#pragma once
#include <string>
#include <map>
#include <memory>

class Settings {
public:
    static Settings& GetInstance();

    void LoadSettings();
    void SaveSettings();

    bool GetBool(const std::string& key) const;
    int GetInt(const std::string& key) const;
    std::string GetString(const std::string& key) const;

    void SetBool(const std::string& key, bool value);
    void SetInt(const std::string& key, int value);
    void SetString(const std::string& key, const std::string& value);

private:
    Settings() = default;
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    void CreateDefaultSettings();
    std::string GetSettingsPath() const;

    std::map<std::string, std::string> m_settings;
    bool m_loaded = false;
};