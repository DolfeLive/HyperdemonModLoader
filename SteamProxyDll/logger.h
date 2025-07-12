#pragma once
#include <string>
#include <mutex>
#include <fstream>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR_LEVEL,
    DEBUG_LEVEL
};

class Logger {
public:
    static void Initialize();
    static void Shutdown();

    static void Log(const std::string& message, LogLevel level = LogLevel::INFO);
    static void LogInfo(const std::string& message);
    static void LogWarning(const std::string& message);
    static void LogError(const std::string& message);
    static void LogDebug(const std::string& message);

private:
    static std::mutex s_mutex;
    static std::ofstream s_logFile;
    static bool s_initialized;

    static void WriteToConsole(const std::string& message, LogLevel level);
    static void WriteToFile(const std::string& message, LogLevel level);
    static std::string GetTimestamp();
    static std::string GetLogLevelString(LogLevel level);
    static void SetConsoleColor(LogLevel level);
    static void ResetConsoleColor();
    static void CheckLogFileSize();
};