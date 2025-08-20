#include "pch.h"
#include "steam_proxy.h"
#include "logger.h"
#include "settings.h"

HMODULE SteamProxy::s_hOriginalDll = nullptr;

void SteamProxy::SetOriginalDll(HMODULE hDll) {
    s_hOriginalDll = hDll;
}

HMODULE SteamProxy::GetOriginalDll() {
    return s_hOriginalDll;
}

bool SteamProxy::SteamAPI_Init() {
    Logger::LogInfo(">>> SteamAPI_Init called!");

    if (s_hOriginalDll) {
        auto originalFunc = (bool(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_Init");
        if (originalFunc) {
            bool result = originalFunc();
            Logger::LogInfo(">>> SteamAPI_Init result: " + std::string(result ? "TRUE" : "FALSE"));
            return result;
        }
    }

    Logger::LogError(">>> SteamAPI_Init: Failed to call original function");
    return false;
}

bool SteamProxy::SteamAPI_IsSteamRunning() {
    Logger::LogInfo(">>> INTERCEPTED: SteamAPI_IsSteamRunning called!");

    if (s_hOriginalDll) {
        auto originalFunc = (bool(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_IsSteamRunning");
        if (originalFunc) {
            bool result = originalFunc();
            Logger::LogInfo(">>> SteamAPI_IsSteamRunning result: " + std::string(result ? "TRUE" : "FALSE"));
            return result;
        }
    }

    Logger::LogError(">>> SteamAPI_IsSteamRunning: Failed to call original function");
    return false;
}

int SteamProxy::SteamAPI_GetHSteamUser() {
    Logger::LogInfo(">>> SteamAPI_GetHSteamUser called!");

    if (s_hOriginalDll) {
        auto originalFunc = (int(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_GetHSteamUser");
        if (originalFunc) {
            int result = originalFunc();
            Logger::LogInfo(">>> SteamAPI_GetHSteamUser result: " + std::to_string(result));
            return result;
        }
    }

    Logger::LogError(">>> SteamAPI_GetHSteamUser: Failed to call original function - returning 0");
    return 0;
}

int SteamProxy::SteamAPI_GetHSteamPipe() {
    Logger::LogInfo(">>> SteamAPI_GetHSteamPipe called!");

    if (s_hOriginalDll) {
        auto originalFunc = (int(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_GetHSteamPipe");
        if (originalFunc) {
            int result = originalFunc();
            Logger::LogInfo(">>> SteamAPI_GetHSteamPipe result: " + std::to_string(result));
            return result;
        }
    }

    Logger::LogError(">>> SteamAPI_GetHSteamPipe: Failed to call original function - returning 0");
    return 0;
}

void* SteamProxy::SteamInternal_ContextInit(void* ctx) {
    Logger::LogInfo(">>> SteamInternal_ContextInit called!");

    if (s_hOriginalDll) {
        using Fn = void* (__cdecl*)(void*);
        auto originalFunc = (Fn)GetProcAddress(s_hOriginalDll, "SteamInternal_ContextInit");
        if (originalFunc) {
            void* result = originalFunc(ctx);
            Logger::LogInfo(">>> SteamInternal_ContextInit result: " + std::string(result ? "non-nullptr" : "nullptr"));
            return result;
        }
    }

    Logger::LogError(">>> SteamInternal_ContextInit: Failed to call original function - returning nullptr");
    return nullptr;
}

void* SteamProxy::SteamInternal_CreateInterface(const char* version) {
    Logger::LogInfo(">>> SteamInternal_CreateInterface called!");
    if (version) {
        Logger::LogDebug(">>> Interface version: " + std::string(version));
    }

    if (s_hOriginalDll) {
        auto originalFunc = (void* (__cdecl*)(const char*))GetProcAddress(s_hOriginalDll, "SteamInternal_CreateInterface");
        if (originalFunc) {
            void* result = originalFunc(version);
            Logger::LogInfo(">>> SteamInternal_CreateInterface result: " + std::string(result ? "non-nullptr" : "nullptr"));
            return result;
        }
    }

    Logger::LogError(">>> SteamInternal_CreateInterface: Failed to call original function - returning nullptr");
    return nullptr;
}

void SteamProxy::SteamAPI_Shutdown() {
    Logger::LogInfo(">>> SteamAPI_Shutdown called!");
    
    if (s_hOriginalDll) {
        auto originalFunc = (void(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_Shutdown");
        if (originalFunc) {
            originalFunc();
            Logger::LogInfo(">>> SteamAPI_Shutdown: Called original function");
        }
    }
}

void SteamProxy::SteamAPI_RunCallbacks() {
    if (Settings::GetInstance().GetBool("detailed_logging")) {
        Logger::LogDebug(">>> SteamAPI_RunCallbacks called");
    }

    if (s_hOriginalDll) {
        auto originalFunc = (void(*)())GetProcAddress(s_hOriginalDll, "SteamAPI_RunCallbacks");
        if (originalFunc) {
            originalFunc();
        }
    }
}

extern "C" {
    __declspec(dllexport) bool SteamAPI_Init() {
        return SteamProxy::SteamAPI_Init();
    }

    __declspec(dllexport) bool SteamAPI_IsSteamRunning() {
        return SteamProxy::SteamAPI_IsSteamRunning();
    }

    __declspec(dllexport) int SteamAPI_GetHSteamUser() {
        return SteamProxy::SteamAPI_GetHSteamUser();
    }

    __declspec(dllexport) int SteamAPI_GetHSteamPipe() {
        return SteamProxy::SteamAPI_GetHSteamPipe();
    }

    __declspec(dllexport) void* SteamInternal_ContextInit(void* ctx) {
        return SteamProxy::SteamInternal_ContextInit(ctx);
    }

    __declspec(dllexport) void* SteamInternal_CreateInterface(const char* version) {
        return SteamProxy::SteamInternal_CreateInterface(version);
    }

    __declspec(dllexport) void SteamAPI_Shutdown() {
        SteamProxy::SteamAPI_Shutdown();
    }

    __declspec(dllexport) void SteamAPI_RunCallbacks() {
        SteamProxy::SteamAPI_RunCallbacks();
    }
}
