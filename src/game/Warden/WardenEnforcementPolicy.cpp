/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "WardenEnforcementPolicy.h"

#include "WardenServer.h"

namespace warden
{
WardenEnforcementPolicy::WardenEnforcementPolicy(
    WardenEnforcementMode mode) : m_mode(mode)
{
}

std::vector<WardenPolicyDecision> WardenEnforcementPolicy::EvaluateBatch(
    WardenEvidenceBatch const& batch)
{
    if (batch.purpose == CheckPlanPurpose::Confirmation)
    {
        if (batch.evidence.size() != 1u)
            return ConfirmationContractViolation();

        WardenEvidence const& evidence = batch.evidence.front();
        auto const pending = m_pendingConfirmations.find(evidence.checkId);
        if (pending == m_pendingConfirmations.end() ||
            evidence.checkType == WardenCheckType::Timing ||
            pending->second.checkType != evidence.checkType ||
            pending->second.evidenceClass != evidence.evidenceClass)
            return ConfirmationContractViolation();

        // Classify before erasing. If the returned outcome violates the exact
        // confirmation contract, the matched pending identity must still be
        // drained as an Unavailable audit before Warden disengages.
        WardenConfirmedDisposition const disposition =
            ClassifyConfirmedEvidence(m_mode, evidence);
        if (disposition == WardenConfirmedDisposition::Invalid)
            return ConfirmationContractViolation();

        m_pendingConfirmations.erase(pending);

        WardenPolicyDecision decision;
        decision.checkId = evidence.checkId;
        decision.checkType = evidence.checkType;
        decision.evidenceClass = evidence.evidenceClass;
        decision.outcome = evidence.outcome;
        switch (disposition)
        {
            case WardenConfirmedDisposition::Cleared:
                decision.action = WardenPolicyAction::ConfirmationCleared;
                break;
            case WardenConfirmedDisposition::Audit:
                decision.action = m_confirmedAudits.insert(
                    AuditKey(evidence.checkId, evidence.outcome)).second ?
                    WardenPolicyAction::PersistAudit :
                    WardenPolicyAction::None;
                break;
            case WardenConfirmedDisposition::Incident:
                decision.action = WardenPolicyAction::PersistAndKick;
                break;
            case WardenConfirmedDisposition::Invalid:
                return ConfirmationContractViolation();
        }
        return {decision};
    }

    std::vector<WardenPolicyDecision> decisions;
    for (WardenEvidence const& evidence : batch.evidence)
    {
        if (!NeedsConfirmation(evidence) ||
            m_confirmedAudits.find(AuditKey(evidence.checkId,
                evidence.outcome)) != m_confirmedAudits.end() ||
            m_pendingConfirmations.find(evidence.checkId) !=
                m_pendingConfirmations.end())
            continue;

        m_pendingConfirmations.emplace(evidence.checkId,
            PendingConfirmation{evidence.checkType, evidence.evidenceClass});
        decisions.push_back({WardenPolicyAction::QueueConfirmation,
            evidence.checkId, evidence.checkType, evidence.evidenceClass,
            evidence.outcome});
    }
    return decisions;
}

std::vector<WardenPolicyDecision>
WardenEnforcementPolicy::AbortPendingConfirmations()
{
    std::vector<WardenPolicyDecision> decisions;
    decisions.reserve(m_pendingConfirmations.size());
    for (auto const& pending : m_pendingConfirmations)
    {
        decisions.push_back({WardenPolicyAction::PersistAudit,
            pending.first, pending.second.checkType,
            pending.second.evidenceClass, WardenCheckOutcome::Unavailable});
    }
    m_pendingConfirmations.clear();
    return decisions;
}

WardenPolicyDecision WardenEnforcementPolicy::EvaluateLifecycle(
    WardenLifecycleEvent const& event) const
{
    if (event.state != WardenState::Failed ||
        m_mode == WardenEnforcementMode::Observe)
        return {};

    // Closing the connection preserves enforcement availability without
    // treating a protocol or infrastructure failure as cheating evidence.
    return {WardenPolicyAction::Kick};
}

std::vector<WardenPolicyDecision>
WardenEnforcementPolicy::ConfirmationContractViolation()
{
    std::vector<WardenPolicyDecision> decisions =
        AbortPendingConfirmations();
    decisions.push_back({m_mode == WardenEnforcementMode::Observe ?
        WardenPolicyAction::Disengage : WardenPolicyAction::Kick});
    return decisions;
}

uint64 WardenEnforcementPolicy::AuditKey(uint32 checkId,
    WardenCheckOutcome outcome)
{
    return (uint64(checkId) << 8u) | uint8(outcome);
}
}
