#include <memory>
#include <string>
#include <mutex>
#include "terrain/GoModelStore.hpp"
#include "terrain/TileSerializer.hpp"

namespace world::terrain
{
    GoModelStore& GoModelStore::Instance()
    {
        static GoModelStore store;
        return store;
    }

    void GoModelStore::SetDirectory(const std::string& dir)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_dir = dir;
        m_models.clear();
    }

    void GoModelStore::Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_models.clear();
    }

    std::shared_ptr<const ICollisionModel> GoModelStore::Get(uint32_t displayId)
    {
        std::string dir;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto found = m_models.find(displayId);
            if (found != m_models.end())
            {
                return found->second;
            }
            dir = m_dir;
        }

        // The file is read OUTSIDE the lock. Held across the open, one disk read
        // serialises every game-object spawn in the world behind it -- unnoticeable at
        // boot, where the same thread does them one after another anyway, and a stall
        // on every map-update thread at once when a fight pulls in a display id nobody
        // has touched yet.
        std::shared_ptr<const ICollisionModel> model;
        if (!dir.empty())
        {
            if (auto tile = ReadTile(dir + "/" + GoModelFileName(displayId)))
            {
                if (!tile->instances.empty())
                {
                    model = tile->instances.front().model;
                }
            }
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        if (dir != m_dir)
        {
            return model;               // SetDirectory ran under us; do not cache it
        }

        // A null is cached too: it records that this display id has no collision, which
        // is true of most of them, and spares a failed open per spawn. Two threads that
        // raced on the same id both read the file; emplace keeps whichever arrived
        // first, so every caller still shares one model.
        return m_models.emplace(displayId, model).first->second;
    }
}
