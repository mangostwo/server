#pragma once

// The baker's interactive front end, on the same console the server uses.
//
// A bake is a long, opaque, one-shot job that people run once and then wonder about,
// so it is exactly the case a status header and a progress bar are for. The console
// is game-agnostic -- it takes strings and numbers -- so the baker can drive it
// without either side knowing about the other.
//
// Everything here degrades: when there is no terminal to take over (a pipe, a CI log)
// Start() fails, Active() stays false, and every call below falls back to plain lines.

#include "Console/ConsoleUI.h"

#include <string>

namespace world::terrain
{
    class ExtractorConsole
    {
    public:
        bool Start(const std::string& src, const std::string& dest);
        void Stop();
        bool Active() const;

        void Log(const std::string& text);
        void Detail(const std::string& text);
        void Success(const std::string& text);
        void Warn(const std::string& text);
        void Error(const std::string& text);

        void Activity(const std::string& text);
        void Progress(int percent);

        void SetStage(const std::string& stage);
        void SetCounts(size_t done, size_t total);
        void SetElapsed(unsigned seconds);

        // Blocks until the user picks a component set, or types quit. Returns false
        // when the user chose to leave without baking.
        struct Choice
        {
            bool dbc = false;
            bool tiles = false;
            bool goModels = false;
            bool nav = false;
            int mapFilter = -1;
            std::string src;
            std::string dest;
        };
        bool RunMenu(Choice& out);

        // Windows' folder picker. False when the user cancelled, or when the platform
        // has no such dialog -- in which case typing the path is the only way, which is
        // why typing it is always accepted.
        static bool BrowseForFolder(const std::string& title, std::string& path);

        // Backslashes to forward slashes. Every path the baker shows and stores is in
        // one style, whichever half of the program produced it.
        static std::string ToUnixPath(std::string path);

    private:
        void Draw();

        bool m_active = false;
    };
}
