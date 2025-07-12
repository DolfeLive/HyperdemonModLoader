#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"
#include "logger.h"
#include "settings.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <windows.h>

std::mutex Logger::s_mutex;
std::ofstream Logger::s_logFile;
bool Logger::s_initialized = false;

void Logger::Initialize() {
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_initialized) return;

    if (Settings::GetInstance().GetBool("enable_file_logging")) {
        std::string logFileName = Settings::GetInstance().GetString("log_file_name");

        char dllPath[MAX_PATH];
        HMODULE hModule = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)Initialize,
            &hModule);
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);

        std::string path(dllPath);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            path = path.substr(0, lastSlash + 1);
        }

        std::string fullLogPath = path + logFileName;
        s_logFile.open(fullLogPath, std::ios::app);
    }

    s_initialized = true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_logFile.is_open()) {
        s_logFile.close();
    }

    s_initialized = false;
}

void Logger::Log(const std::string& message, LogLevel level) {
    if (!s_initialized) return;

    std::lock_guard<std::mutex> lock(s_mutex);

    if (Settings::GetInstance().GetBool("enable_console")) {
        WriteToConsole(message, level);
    }

    if (Settings::GetInstance().GetBool("enable_file_logging")) {
        WriteToFile(message, level);
        CheckLogFileSize();
    }
}

void Logger::LogInfo(const std::string& message) {
    Log(message, LogLevel::INFO);
}

void Logger::LogWarning(const std::string& message) {
    Log(message, LogLevel::WARNING);
}

void Logger::LogError(const std::string& message) {
    Log(message, LogLevel::ERROR_LEVEL);
}

void Logger::LogDebug(const std::string& message) {
    if (Settings::GetInstance().GetBool("detailed_logging")) {
        Log(message, LogLevel::DEBUG_LEVEL);
    }
}

void Logger::WriteToConsole(const std::string& message, LogLevel level) {
    SetConsoleColor(level);
    std::cout << "[" << GetTimestamp() << "] [" << GetLogLevelString(level) << "] " << message << std::endl;
    ResetConsoleColor();
}

void Logger::WriteToFile(const std::string& message, LogLevel level) {
    if (s_logFile.is_open()) {
        s_logFile << "[" << GetTimestamp() << "] [" << GetLogLevelString(level) << "] " << message << std::endl;
        s_logFile.flush();
    }
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string Logger::GetLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:          return "INFO";
        case LogLevel::WARNING:       return "WARN";
        case LogLevel::ERROR_LEVEL:   return "ERROR";
        case LogLevel::DEBUG_LEVEL:   return "DEBUG";
        default:                      return "UNKNOWN";
    }
}

void Logger::SetConsoleColor(LogLevel level) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    switch (level) {
    case LogLevel::INFO:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        break;
    case LogLevel::WARNING:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        break;
    case LogLevel::ERROR_LEVEL:
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        break;
    case LogLevel::DEBUG_LEVEL:
        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        break;
    }
}

void Logger::ResetConsoleColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void Logger::CheckLogFileSize() {
    if (!s_logFile.is_open()) return;

    s_logFile.seekp(0, std::ios::end);
    size_t fileSize = s_logFile.tellp();
    s_logFile.seekp(0, std::ios::beg);

    size_t maxSize = Settings::GetInstance().GetInt("max_log_file_size_mb") * 1024 * 1024;

    if (fileSize > maxSize) {
        s_logFile.close();

        std::string logFileName = Settings::GetInstance().GetString("log_file_name");
        std::string backupName = logFileName + ".backup";

        MoveFileA(logFileName.c_str(), backupName.c_str());

        s_logFile.open(logFileName, std::ios::app);
    }
}