#include "ExtractorConsole.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

#include <chrono>
#include <cstdio>
#include <thread>

namespace world::terrain
{
    namespace
    {
        using MaNGOS::Console::ConsoleUI;
        using MaNGOS::Console::Style;

        ConsoleUI& Ui() { return ConsoleUI::Instance(); }

        // Status header slots, so a later call cannot quietly take another's row.
        enum StatusSlot
        {
            SLOT_STAGE = 0,
            SLOT_PROGRESS = 1,
            SLOT_ELAPSED = 2,
            SLOT_SOURCE = 3,
            SLOT_DEST = 4
        };

        std::string Trim(const std::string& in)
        {
            const size_t first = in.find_first_not_of(" \t\r\n\"'");
            if (first == std::string::npos)
            {
                return std::string();
            }
            const size_t last = in.find_last_not_of(" \t\r\n\"'");
            return in.substr(first, last - first + 1);
        }

        std::string Shorten(const std::string& path, size_t width)
        {
            if (path.size() <= width)
            {
                return path;
            }
            return "..." + path.substr(path.size() - (width - 3));
        }
    }

    std::string ExtractorConsole::ToUnixPath(std::string path)
    {
        for (char& c : path)
        {
            if (c == '\\')
            {
                c = '/';
            }
        }
        return path;
    }

#if defined(_WIN32)
    bool ExtractorConsole::BrowseForFolder(const std::string& title, std::string& path)
    {
        // The console owns the alternate screen, but the picker is its own window and
        // draws nowhere near it, so nothing needs saving and restoring here.
        const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool weInitialised = SUCCEEDED(init);

        BROWSEINFOA info{};
        char display[MAX_PATH] = {0};
        info.hwndOwner = GetConsoleWindow();
        info.pszDisplayName = display;
        info.lpszTitle = title.c_str();
        info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

        bool picked = false;
        if (LPITEMIDLIST idl = SHBrowseForFolderA(&info))
        {
            char chosen[MAX_PATH] = {0};
            if (SHGetPathFromIDListA(idl, chosen))
            {
                path = ToUnixPath(chosen);
                picked = true;
            }
            CoTaskMemFree(idl);
        }

        if (weInitialised)
        {
            CoUninitialize();
        }
        return picked;
    }
#else
    bool ExtractorConsole::BrowseForFolder(const std::string&, std::string&)
    {
        return false;
    }
#endif

    bool ExtractorConsole::Start(const std::string& src, const std::string& dest)
    {
        m_active = Ui().Start("MaNGOS Extractor", "3.3.5a client data");
        if (!m_active)
        {
            return false;
        }

        Ui().SetStatus(SLOT_SOURCE, "client", Shorten(src, 48), Style::STYLE_DETAIL);
        Ui().SetStatus(SLOT_DEST, "output", Shorten(dest, 48), Style::STYLE_DETAIL);
        Ui().SetStatus(SLOT_STAGE, "stage", "idle", Style::STYLE_ACCENT);
        Ui().SetProgress(-1);
        Draw();
        return true;
    }

    void ExtractorConsole::Stop()
    {
        if (m_active)
        {
            Ui().Stop();
            m_active = false;
        }
    }

    bool ExtractorConsole::Active() const { return m_active; }

    void ExtractorConsole::Draw()
    {
        if (m_active)
        {
            Ui().Render();
        }
    }

    void ExtractorConsole::Log(const std::string& text)
    {
        if (m_active)
        {
            Ui().PushLog(text, Style::STYLE_NORMAL);
            Draw();
        }
        else
        {
            std::printf("%s\n", text.c_str());
        }
    }

    void ExtractorConsole::Detail(const std::string& text)
    {
        if (m_active)
        {
            Ui().PushLog(text, Style::STYLE_DETAIL);
            Draw();
        }
        else
        {
            std::printf("%s\n", text.c_str());
        }
    }

    void ExtractorConsole::Success(const std::string& text)
    {
        if (m_active)
        {
            Ui().PushLog(text, Style::STYLE_SUCCESS);
            Draw();
        }
        else
        {
            std::printf("%s\n", text.c_str());
        }
    }

    void ExtractorConsole::Warn(const std::string& text)
    {
        if (m_active)
        {
            Ui().PushLog(text, Style::STYLE_WARN);
            Draw();
        }
        else
        {
            std::printf("%s\n", text.c_str());
        }
    }

    void ExtractorConsole::Error(const std::string& text)
    {
        if (m_active)
        {
            Ui().PushLog(text, Style::STYLE_ERROR);
            Draw();
        }
        else
        {
            std::fprintf(stderr, "%s\n", text.c_str());
        }
    }

    void ExtractorConsole::Activity(const std::string& text)
    {
        if (m_active)
        {
            Ui().SetActivity(text);
            Draw();
        }
    }

    void ExtractorConsole::Progress(int percent)
    {
        if (m_active)
        {
            Ui().SetProgress(percent);
            Draw();
        }
    }

    void ExtractorConsole::SetStage(const std::string& stage)
    {
        if (m_active)
        {
            Ui().SetStatus(SLOT_STAGE, "stage", stage, Style::STYLE_ACCENT);
            Draw();
        }
    }

    void ExtractorConsole::SetCounts(size_t done, size_t total)
    {
        if (!m_active)
        {
            return;
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%zu / %zu", done, total);
        Ui().SetStatus(SLOT_PROGRESS, "written", buf, Style::STYLE_NORMAL);
        if (total)
        {
            Ui().SetProgress(int(done * 100 / total));
        }
        Draw();
    }

    void ExtractorConsole::SetElapsed(unsigned seconds)
    {
        if (!m_active)
        {
            return;
        }
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u", seconds / 3600,
                      (seconds / 60) % 60, seconds % 60);
        Ui().SetStatus(SLOT_ELAPSED, "elapsed", buf, Style::STYLE_DETAIL);
        Draw();
    }

    bool ExtractorConsole::RunMenu(Choice& out)
    {
        if (!m_active)
        {
            return true;  // no terminal: whatever the command line asked for stands
        }

        auto showMenu = [&]()
        {
            Ui().PushLog("", Style::STYLE_NORMAL);
            Ui().PushLog("  What should be baked?", Style::STYLE_ACCENT);
            Ui().PushLog("", Style::STYLE_NORMAL);
            Ui().PushLog("    1   everything          dbc + tiles + gomodels + nav",
                         Style::STYLE_NORMAL);
            Ui().PushLog("    2   dbc                 the raw DBC set the server loads",
                         Style::STYLE_NORMAL);
            Ui().PushLog("    3   tiles               fused terrain + static collision",
                         Style::STYLE_NORMAL);
            Ui().PushLog("    4   gomodels            door, lift and bridge collision",
                         Style::STYLE_NORMAL);
            Ui().PushLog("    5   nav                 the navmesh, from the baked tiles",
                         Style::STYLE_NORMAL);
            Ui().PushLog("", Style::STYLE_NORMAL);
            Ui().PushLog("    src [path]          client Data dir; no path opens a browser",
                         Style::STYLE_DETAIL);
            Ui().PushLog("    dest [path]         output dir;      no path opens a browser",
                         Style::STYLE_DETAIL);
            Ui().PushLog("    map <id>            restrict the bake to one map",
                         Style::STYLE_DETAIL);
            Ui().PushLog("    q                   quit without baking",
                         Style::STYLE_DETAIL);
            Ui().PushLog("", Style::STYLE_NORMAL);
        };

        auto setPath = [&](const char* what, const std::string& typed, std::string& slot)
        {
            std::string chosen = typed;
            if (chosen.empty())
            {
                if (!BrowseForFolder(std::string("Choose the ") + what + " folder", chosen))
                {
                    Ui().PushLog("  cancelled -- or no folder browser on this platform; "
                                 "type the path instead", Style::STYLE_WARN);
                    Draw();
                    return;
                }
            }
            slot = ToUnixPath(chosen);
            Ui().PushLog(std::string("  ") + what + " = " + slot, Style::STYLE_SUCCESS);
            Ui().SetStatus(std::string("src") == what ? SLOT_SOURCE : SLOT_DEST,
                           std::string("src") == what ? "client" : "output",
                           Shorten(slot, 48), Style::STYLE_DETAIL);
            Draw();
        };

        showMenu();
        Ui().SetPrompt("bake> ");
        Ui().SetHint("pick a number, or q to leave");
        Draw();

        std::string line;
        while (true)
        {
            if (!Ui().PollInput(line))
            {
                Ui().Render();
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                continue;
            }

            const std::string cmd = Trim(line);
            if (cmd.empty())
            {
                continue;
            }

            if (cmd == "q" || cmd == "quit" || cmd == "exit")
            {
                return false;
            }
            if (cmd.rfind("map ", 0) == 0)
            {
                out.mapFilter = std::atoi(cmd.c_str() + 4);
                char buf[64];
                std::snprintf(buf, sizeof(buf), "  restricted to map %d", out.mapFilter);
                Ui().PushLog(buf, Style::STYLE_SUCCESS);
                Draw();
                continue;
            }
            if (cmd == "src" || cmd.rfind("src ", 0) == 0)
            {
                setPath("src", cmd.size() > 4 ? Trim(cmd.substr(4)) : std::string(),
                        out.src);
                continue;
            }
            if (cmd == "dest" || cmd.rfind("dest ", 0) == 0)
            {
                setPath("dest", cmd.size() > 5 ? Trim(cmd.substr(5)) : std::string(),
                        out.dest);
                continue;
            }
            if (cmd == "?" || cmd == "help")
            {
                showMenu();
                Draw();
                continue;
            }

            if (cmd == "1") { out.dbc = out.tiles = out.goModels = out.nav = true; }
            else if (cmd == "2") { out.dbc = true; }
            else if (cmd == "3") { out.tiles = true; }
            else if (cmd == "4") { out.goModels = true; }
            else if (cmd == "5") { out.nav = true; }
            else
            {
                Ui().PushLog("  not one of the choices", Style::STYLE_WARN);
                Draw();
                continue;
            }

            Ui().SetPrompt("");
            Ui().SetHint("baking; this takes a while");
            Draw();
            return true;
        }
    }
}
