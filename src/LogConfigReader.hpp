#pragma once

#include "Singleton.hpp"
#include "samplog/LogLevel.hpp"
#include "FileChangeDetector.hpp"
#include "LogRotationManager.hpp"

#include <chrono>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>


using samplog::LogLevel;


struct LoggerConfig
{
	LogLevel Level = LogLevel::ERROR | LogLevel::WARNING | LogLevel::FATAL;
	bool PrintToConsole = false;
	bool Append = true;
	LogRotationConfig Rotation;
};

struct LogLevelConfig
{
	bool PrintToConsole = false;
};

struct GlobalConfig
{
	std::string LogTimeFormat = "%x %X";
	bool DisableDebugInfo = false;
	bool EnableColors = false;
};

class LogConfig : public Singleton<LogConfig>
{
	friend Singleton<LogConfig>;
private:
	LogConfig() = default;
	~LogConfig() = default;

private: // variables
	std::mutex _configLock;
	std::unordered_map<std::string, LoggerConfig> _loggerConfigs;
	std::map<LogLevel, LogLevelConfig> _levelConfigs;
	GlobalConfig _globalConfig;
	std::unique_ptr<FileChangeDetector> _fileWatcher;

private: // functions
	void ParseConfigFile();

public: // functions
	void Initialize();
	bool GetLoggerConfig(std::string const &module_name, LoggerConfig &dest)
	{
		std::lock_guard<std::mutex> lock(_configLock);
		auto it = _loggerConfigs.find(module_name);
		if (it != _loggerConfigs.end())
		{
			dest = it->second;
			return true;
		}
		return false;
	}
	LogLevelConfig const &GetLogLevelConfig(LogLevel level)
	{
		std::lock_guard<std::mutex> lock(_configLock);
		return _levelConfigs[level];
	}
	GlobalConfig const &GetGlobalConfig()
	{
		std::lock_guard<std::mutex> lock(_configLock);
		return _globalConfig;
	}
};
