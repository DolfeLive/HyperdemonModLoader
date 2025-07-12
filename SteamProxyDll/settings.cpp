#include "pch.h"
#include "settings.h"
#include <fstream>
#include <sstream>
#include <windows.h>

namespace SimpleJson {
    std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    std::map<std::string, std::string> Parse(const std::string& json) {
        std::map<std::string, std::string> result;
        std::istringstream stream(json);
        std::string line;

        while (std::getline(stream, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '{' || line[0] == '}') continue;

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string key = Trim(line.substr(0, colonPos));
            std::string value = Trim(line.substr(colonPos + 1));

            if (key.front() == '"' && key.back() == '"') {
                key = key.substr(1, key.length() - 2);
            }
            if (value.back() == ',') {
                value = value.substr(0, value.length() - 1);
            }
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            result[key] = value;
        }

        return result;
    }

    std::string Generate(const std::map<std::string, std::string>& data) {
        std::ostringstream json;
        json << "{\n";

        auto it = data.begin();
        while (it != data.end()) {
            json << "  \"" << it->first << "\": ";

            if (it->second == "true" || it->second == "false" ||
                (it->second.find_first_not_of("0123456789") == std::string::npos && !it->second.empty())) {
                json << it->second;
            }
            else {
                json << "\"" << it->second << "\"";
            }

            ++it;
            if (it != data.end()) {
                json << ",";
            }
            json << "\n";
        }

        json << "}";
        return json.str();
    }
}

static HMODULE GetCurrentModule() {
    HMODULE hModule = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&GetCurrentModule),
        &hModule);
    return hModule;
}

Settings& Settings::GetInstance() {
    static Settings instance;
    return instance;
}

void Settings::LoadSettings() {
    if (m_loaded) return;

    std::string settingsPath = GetSettingsPath();
    std::ifstream file(settingsPath);

    if (!file.is_open()) {
        CreateDefaultSettings();
        SaveSettings();
        m_loaded = true;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    try {
        m_settings = SimpleJson::Parse(content);
        m_loaded = true;
    }
    catch (...) {
        CreateDefaultSettings();
        SaveSettings();
        m_loaded = true;
    }
}

void Settings::SaveSettings() {
    std::string settingsPath = GetSettingsPath();
    std::ofstream file(settingsPath);

    if (file.is_open()) {
        file << SimpleJson::Generate(m_settings);
        file.close();
    }
}

bool Settings::GetBool(const std::string& key) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        return it->second == "true";
    }
    return false;
}

int Settings::GetInt(const std::string& key) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        try {
            return std::stoi(it->second);
        }
        catch (...) {
            return 0;
        }
    }
    return 0;
}

std::string Settings::GetString(const std::string& key) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        return it->second;
    }
    return "";
}

void Settings::SetBool(const std::string& key, bool value) {
    m_settings[key] = value ? "true" : "false";
}

void Settings::SetInt(const std::string& key, int value) {
    m_settings[key] = std::to_string(value);
}

void Settings::SetString(const std::string& key, const std::string& value) {
    m_settings[key] = value;
}

void Settings::CreateDefaultSettings() {
    m_settings.clear();

    SetBool("enable_console", true);
    SetBool("enable_file_logging", true);
    SetBool("detailed_logging", false);
    SetString("original_dll_name", "steam_api64_original.dll");
    SetString("log_file_name", "steam_proxy_log.txt");
    SetInt("max_log_file_size_mb", 10);
}

std::string Settings::GetSettingsPath() const {
    char dllPath[MAX_PATH];
    HMODULE hModule = GetCurrentModule();

    if (hModule) {
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    }
    else {
        hModule = GetModuleHandleA(nullptr);
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    }

    std::string path(dllPath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        path = path.substr(0, lastSlash + 1);
    }

    return path + "steam_proxy_settings.json";
}