#include "pch.h"
#include "steam_proxy.h"
#include "logger.h"
#include "settings.h"
#include <thread>
#include <chrono>

static HMODULE hOriginalDll = nullptr;
static bool initialized = false;

DWORD WINAPI MainThread(LPVOID lpParam) {
    if (Settings::GetInstance().GetBool("enable_console")) {
        AllocConsole(); 
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);

        SetConsoleTitleA("Steam API Proxy");
    }

    Logger::Log("=== Steam API Proxy Started ===");
    Logger::Log("Console enabled: " + std::string(Settings::GetInstance().GetBool("enable_console") ? "YES" : "NO"));
    Logger::Log("Detailed logging: " + std::string(Settings::GetInstance().GetBool("detailed_logging") ? "YES" : "NO"));

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (Settings::GetInstance().GetBool("enable_console")) {
        FreeConsole();
    }

    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!initialized) {
            initialized = true;

            Settings::GetInstance().LoadSettings();

            Logger::Initialize();

            CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);

            std::string originalDllName = Settings::GetInstance().GetString("original_dll_name");
            hOriginalDll = LoadLibraryA(originalDllName.c_str());

            if (hOriginalDll) {
                Logger::Log("Successfully loaded original Steam API: " + originalDllName);
                SteamProxy::SetOriginalDll(hOriginalDll);
            }
            else {
                Logger::Log("Failed to load original Steam API: " + originalDllName);
            }
        }
        break;

    case DLL_PROCESS_DETACH:
        if (initialized) {
            Logger::Log("=== Steam API Proxy Shutting Down ===");

            if (hOriginalDll) {
                FreeLibrary(hOriginalDll);
                hOriginalDll = nullptr;
            }

            Logger::Shutdown();
            initialized = false;
        }
        break;
    }
    return TRUE;
}