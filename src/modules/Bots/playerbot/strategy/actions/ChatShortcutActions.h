#pragma once

#include "../Action.h"
#include "InventoryAction.h"

namespace ai
{
    class ReturnPositionResetAction : public Action
    {
    public:
        ReturnPositionResetAction(PlayerbotAI* ai, string name) : Action(ai, name) {}
        void ResetReturnPosition();
        void SetReturnPosition(float x, float y, float z);
    };

    class FollowChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        FollowChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "follow chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class StayChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        StayChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "stay chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class FleeChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        FleeChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "flee chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class GoawayChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        GoawayChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "runaway chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class GrindChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        GrindChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "grind chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class TankAttackChatShortcutAction : public ReturnPositionResetAction
    {
    public:
        TankAttackChatShortcutAction(PlayerbotAI* ai) : ReturnPositionResetAction(ai, "tank attack chat shortcut") {}
        virtual bool Execute(Event event);
    };

    class MaxDpsChatShortcutAction : public Action
    {
    public:
        MaxDpsChatShortcutAction(PlayerbotAI* ai) : Action(ai, "max dps chat shortcut") {}
        virtual bool Execute(Event event);
    };

}
