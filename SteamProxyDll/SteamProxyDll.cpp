#include "pch.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <cstdarg>

static HMODULE hOriginalDll = nullptr;

// Hook variables for the first logging function
void* originalLogFunc = nullptr;
void* hookedLogFunc = nullptr;
BYTE originalBytes[14];
bool hookInstalled = false;

// Hook variables for the second logging function (FUN_1401f4b60)
void* originalPrintfLogFunc = nullptr;
void* hookedPrintfLogFunc = nullptr;
BYTE originalPrintfBytes[14];
bool printfHookInstalled = false;

DWORD WINAPI MainThread(LPVOID lpParam);

std::string GetCurrentDllDirectory() {
    char dllPath[MAX_PATH];
    HMODULE hModule = nullptr;

    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)GetCurrentDllDirectory,
        &hModule);

    GetModuleFileNameA(hModule, dllPath, MAX_PATH);

    std::string path(dllPath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

void LogMessage(const std::string& message) {
    try {
        printf("[PROXY] %s\n", message.c_str());

        std::string logPath = GetCurrentDllDirectory() + "steam_log.txt";
        std::ofstream log(logPath, std::ios::app);
        if (log.is_open()) {
            log << "[PROXY] " << message << std::endl;
            log.close();
        }

        std::ofstream log2("E:\\SteamLibrary\\steamapps\\common\\hyperdemon\\steam_log.txt", std::ios::app);
        if (log2.is_open()) {
            log2 << "[PROXY] " << message << std::endl;
            log2.close();
        }
    }
    catch (...) {}
}

// Original hooked function
void* __fastcall HookedInternalLogFunc(void* param_1, char* param_2) {
    if (param_2) {
        std::string logMsg = std::string("[INTERNAL LOG] ") + param_2;

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%s\n", logMsg.c_str());
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        try {
            std::string logPath = GetCurrentDllDirectory() + "steam_internal_log.txt";
            std::ofstream log(logPath, std::ios::app);
            if (log.is_open()) {
                log << logMsg << std::endl;
                log.close();
            }

            std::ofstream log2("E:\\SteamLibrary\\steamapps\\common\\hyperdemon\\steam_internal_log.txt", std::ios::app);
            if (log2.is_open()) {
                log2 << logMsg << std::endl;
                log2.close();
            }
        }
        catch (...) {}
    }

    return param_1;
}

// New hooked function for FUN_1401f4b60 (printf-style logging)
void __fastcall HookedPrintfLogFunc(const char* format, ...) {
    if (format) {
        // Format the string with variable arguments for logging
        va_list args;
        va_start(args, format);

        char buffer[4096];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::string logMsg = std::string("[PRINTF LOG] ") + buffer;

        // Print to console with cyan color
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%s\n", logMsg.c_str());
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        // Also log to file
        try {
            std::string logPath = GetCurrentDllDirectory() + "steam_printf_log.txt";
            std::ofstream log(logPath, std::ios::app);
            if (log.is_open()) {
                log << logMsg << std::endl;
                log.close();
            }

            std::ofstream log2("E:\\SteamLibrary\\steamapps\\common\\hyperdemon\\steam_printf_log.txt", std::ios::app);
            if (log2.is_open()) {
                log2 << logMsg << std::endl;
                log2.close();
            }
        }
        catch (...) {}
    }

    // NOW CALL THE ORIGINAL FUNCTION - CRITICAL!
    // Temporarily restore original bytes
    DWORD oldProtect;
    if (VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(originalPrintfLogFunc, originalPrintfBytes, 14);
        VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);

        // Call the original function with the same arguments
        va_list args;
        va_start(args, format);

        // Create function pointer to original
        typedef void(__fastcall* OriginalPrintfFunc)(const char*, ...);
        OriginalPrintfFunc originalFunc = (OriginalPrintfFunc)originalPrintfLogFunc;

        // Call original function
        originalFunc(format, args);

        va_end(args);

        // Restore our hook
        if (VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            BYTE jumpCode[14] = {
                0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xFF, 0xE0,
                0x90, 0x90
            };

            memcpy(&jumpCode[2], &hookedPrintfLogFunc, 8);
            memcpy(originalPrintfLogFunc, jumpCode, 14);
            VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);
        }
    }
}

// Function to call the original printf-style logging function with custom strings
typedef void(__fastcall* OriginalPrintfLogFunc)(const char*, ...);
OriginalPrintfLogFunc GetOriginalPrintfLogFunc() {
    if (!originalPrintfLogFunc) return nullptr;

    // Temporarily restore original bytes to call the original function
    DWORD oldProtect;
    if (VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(originalPrintfLogFunc, originalPrintfBytes, 14);
        VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);

        return (OriginalPrintfLogFunc)originalPrintfLogFunc;
    }
    return nullptr;
}

void RestorePrintfHook() {
    if (!printfHookInstalled || !originalPrintfLogFunc) return;

    DWORD oldProtect;
    if (VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // Restore hook
        BYTE jumpCode[14] = {
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xE0,
            0x90, 0x90
        };

        memcpy(&jumpCode[2], &hookedPrintfLogFunc, 8);
        memcpy(originalPrintfLogFunc, jumpCode, 14);
        VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);
    }
}

// Public function to call the original logging function with custom strings
extern "C" __declspec(dllexport) void CallCustomLog(const char* format, ...) {
    if (!format) return;

    va_list args;
    va_start(args, format);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    LogMessage(std::string("[CUSTOM CALL] Attempting to call original printf log function: ") + buffer);

    // Get original function pointer
    auto originalFunc = GetOriginalPrintfLogFunc();
    if (originalFunc) {
        // Call the original function
        originalFunc(format, args);

        // Restore our hook
        RestorePrintfHook();

        LogMessage("[CUSTOM CALL] Successfully called original function and restored hook");
    }
    else {
        LogMessage("[CUSTOM CALL] Failed to get original function pointer");
    }
}

bool InstallSimpleHook() {
    try {
        HMODULE mainModule = GetModuleHandle(NULL);
        if (!mainModule) {
            LogMessage("Failed to get main module handle");
            return false;
        }

        uintptr_t baseAddress = (uintptr_t)mainModule;
        uintptr_t logFunctionAddress = baseAddress + 0x4570;

        LogMessage("Attempting to hook internal logging function...");
        LogMessage("Base address: 0x" + std::to_string(baseAddress));
        LogMessage("Log function address: 0x" + std::to_string(logFunctionAddress));

        originalLogFunc = (void*)logFunctionAddress;
        hookedLogFunc = (void*)HookedInternalLogFunc;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(originalLogFunc, &mbi, sizeof(mbi)) == 0) {
            LogMessage("Failed to query memory at log function address");
            return false;
        }

        DWORD oldProtect;
        if (!VirtualProtect(originalLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            LogMessage("Failed to change memory protection");
            return false;
        }

        memcpy(originalBytes, originalLogFunc, 14);

        BYTE jumpCode[14] = {
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xE0,
            0x90, 0x90
        };

        memcpy(&jumpCode[2], &hookedLogFunc, 8);
        memcpy(originalLogFunc, jumpCode, 14);
        VirtualProtect(originalLogFunc, 14, oldProtect, &oldProtect);

        hookInstalled = true;
        LogMessage("Manual hook installed successfully!");
        return true;

    }
    catch (...) {
        LogMessage("Exception occurred while installing hook");
        return false;
    }
}

bool InstallPrintfHook() {
    try {
        HMODULE mainModule = GetModuleHandle(NULL);
        if (!mainModule) {
            LogMessage("Failed to get main module handle for printf hook");
            return false;
        }

        uintptr_t baseAddress = (uintptr_t)mainModule;
        // You need to replace this offset with the actual offset of FUN_1401f4b60
        uintptr_t printfLogFunctionAddress = baseAddress + 0x1f4b60;  // Adjust this offset!

        LogMessage("Attempting to hook printf-style logging function...");
        LogMessage("Printf log function address: 0x" + std::to_string(printfLogFunctionAddress));

        originalPrintfLogFunc = (void*)printfLogFunctionAddress;
        hookedPrintfLogFunc = (void*)HookedPrintfLogFunc;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(originalPrintfLogFunc, &mbi, sizeof(mbi)) == 0) {
            LogMessage("Failed to query memory at printf log function address");
            return false;
        }

        DWORD oldProtect;
        if (!VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            LogMessage("Failed to change memory protection for printf hook");
            return false;
        }

        memcpy(originalPrintfBytes, originalPrintfLogFunc, 14);

        BYTE jumpCode[14] = {
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xE0,
            0x90, 0x90
        };

        memcpy(&jumpCode[2], &hookedPrintfLogFunc, 8);
        memcpy(originalPrintfLogFunc, jumpCode, 14);
        VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);

        printfHookInstalled = true;
        LogMessage("Printf hook installed successfully!");
        return true;

    }
    catch (...) {
        LogMessage("Exception occurred while installing printf hook");
        return false;
    }
}

void RemoveSimpleHook() {
    if (!hookInstalled || !originalLogFunc) return;

    try {
        DWORD oldProtect;
        if (VirtualProtect(originalLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(originalLogFunc, originalBytes, 14);
            VirtualProtect(originalLogFunc, 14, oldProtect, &oldProtect);
            LogMessage("Manual hook removed successfully");
            hookInstalled = false;
        }
    }
    catch (...) {
        LogMessage("Exception occurred while removing hook");
    }
}

void RemovePrintfHook() {
    if (!printfHookInstalled || !originalPrintfLogFunc) return;

    try {
        DWORD oldProtect;
        if (VirtualProtect(originalPrintfLogFunc, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(originalPrintfLogFunc, originalPrintfBytes, 14);
            VirtualProtect(originalPrintfLogFunc, 14, oldProtect, &oldProtect);
            LogMessage("Printf hook removed successfully");
            printfHookInstalled = false;
        }
    }
    catch (...) {
        LogMessage("Exception occurred while removing printf hook");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized) {
            initialized = true;

            CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);

            hOriginalDll = LoadLibraryA("steam_api64_original.dll");

            LogMessage("=== Steam API Proxy DLL Loaded ===");
            LogMessage(std::string("Original DLL loaded: ") + (hOriginalDll ? "SUCCESS" : "FAILED"));

            if (hOriginalDll) {
                char originalPath[MAX_PATH];
                GetModuleFileNameA(hOriginalDll, originalPath, MAX_PATH);
                LogMessage(std::string("Original DLL path: ") + originalPath);
            }

            LogMessage(std::string("DLL Directory: ") + GetCurrentDllDirectory());

            // Install both hooks after a short delay
            CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
                Sleep(2000);
                InstallSimpleHook();
                InstallPrintfHook();
                return 0;
                }, nullptr, 0, nullptr);
        }
        break;
    case DLL_PROCESS_DETACH:
        if (initialized) {
            LogMessage("=== Steam API Proxy DLL Unloaded ===");

            RemoveSimpleHook();
            RemovePrintfHook();

            if (hOriginalDll) {
                FreeLibrary(hOriginalDll);
                hOriginalDll = nullptr;
            }
            initialized = false;
        }
        break;
    }
    return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);

    SetConsoleTitleA("Steam API Proxy - Multi-Function Logging (Manual Hook)");

    std::cout << "[+] DLL Injected Successfully.\n";
    std::cout << "[*] Using manual hooking (no Detours dependency)\n";
    std::cout << "[*] Logging Steam API calls and internal messages...\n";
    std::cout << "[*] Green text = Internal application logs\n";
    std::cout << "[*] Cyan text = Printf-style logs (FUN_1401f4b60)\n";
    std::cout << "[*] White text = Steam API intercepts\n\n";

    // Example of calling custom log after a delay
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        Sleep(20000);
        while (true)
        {
            Sleep(16);
            CallCustomLog("Custom test message: %s %d", "Hello", 42);
        }
        return 0;
        }, nullptr, 0, nullptr);

    while (true) {
        Sleep(1000);
    }

    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

// ... (rest of your existing Steam API functions remain the same)

extern "C" {
    __declspec(dllexport) bool SteamAPI_Init() {
        LogMessage(">>> INTERCEPTED: SteamAPI_Init called!");

        if (hOriginalDll) {
            auto originalFunc = (bool(*)())GetProcAddress(hOriginalDll, "SteamAPI_Init");
            if (originalFunc) {
                bool result = originalFunc();
                LogMessage(std::string(">>> SteamAPI_Init result: ") + (result ? "TRUE" : "FALSE"));
                return result;
            }
        }

        LogMessage(">>> SteamAPI_Init: Failed to call original function");
        return false;
    }

    __declspec(dllexport) bool SteamAPI_IsSteamRunning() {
        LogMessage(">>> INTERCEPTED: SteamAPI_IsSteamRunning called!");

        if (hOriginalDll) {
            auto originalFunc = (bool(*)())GetProcAddress(hOriginalDll, "SteamAPI_IsSteamRunning");
            if (originalFunc) {
                bool result = originalFunc();
                LogMessage(std::string(">>> SteamAPI_IsSteamRunning result: ") + (result ? "TRUE" : "FALSE"));
                return result;
            }
        }

        LogMessage(">>> SteamAPI_IsSteamRunning: Failed to call original function");
        return false;
    }

    __declspec(dllexport) int SteamAPI_GetHSteamUser() {
        LogMessage(">>> INTERCEPTED: SteamAPI_GetHSteamUser called!");

        if (hOriginalDll) {
            auto originalFunc = (int(*)())GetProcAddress(hOriginalDll, "SteamAPI_GetHSteamUser");
            if (originalFunc) {
                int result = originalFunc();
                LogMessage(std::string(">>> SteamAPI_GetHSteamUser result: ") + std::to_string(result));
                return result;
            }
        }

        LogMessage(">>> SteamAPI_GetHSteamUser: Failed to call original function - returning 0");
        return 0;
    }

    __declspec(dllexport) int SteamAPI_GetHSteamPipe() {
        LogMessage(">>> INTERCEPTED: SteamAPI_GetHSteamPipe called!");

        if (hOriginalDll) {
            auto originalFunc = (int(*)())GetProcAddress(hOriginalDll, "SteamAPI_GetHSteamPipe");
            if (originalFunc) {
                int result = originalFunc();
                LogMessage(std::string(">>> SteamAPI_GetHSteamPipe result: ") + std::to_string(result));
                return result;
            }
        }

        LogMessage(">>> SteamAPI_GetHSteamPipe: Failed to call original function - returning 0");
        return 0;
    }

    __declspec(dllexport) void* SteamInternal_ContextInit(void* ctx) {
        LogMessage(">>> INTERCEPTED: SteamInternal_ContextInit called!");

        static HMODULE realSteam = nullptr;
        if (!realSteam)
            realSteam = LoadLibraryA("steam_api64_original.dll");
        if (!realSteam)
            return nullptr;

        using Fn = void* (__cdecl*)(void*);
        static Fn fn = (Fn)GetProcAddress(realSteam, "SteamInternal_ContextInit");
        if (!fn)
            return nullptr;

        void* result = fn(ctx);
        LogMessage(std::string(">>> SteamInternal_ContextInit result: ") + (result ? "non-nullptr" : "nullptr"));
        return result;
    }

    __declspec(dllexport) void* SteamInternal_CreateInterface(const char* version) {
        LogMessage(">>> INTERCEPTED: SteamInternal_CreateInterface called!");
        if (version) {
            LogMessage(std::string(">>> Interface version: ") + version);
        }

        if (hOriginalDll) {
            auto originalFunc = (void* (__cdecl*)(const char*))GetProcAddress(hOriginalDll, "SteamInternal_CreateInterface");
            if (originalFunc) {
                void* result = originalFunc(version);
                LogMessage(std::string(">>> SteamInternal_CreateInterface result: ") + (result ? "non-nullptr" : "nullptr"));
                return result;
            }
        }

        LogMessage(">>> SteamInternal_CreateInterface: Failed to call original function - returning nullptr");
        return nullptr;
    }
}