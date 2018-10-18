#pragma once

#include "CSingleton.hpp"
#include "samplog/LogLevel.hpp"
#include "FileChangeDetector.hpp"
#include "LogRotationManager.hpp"

#include <chrono>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>


using samplog::LogLevel;


struct LogConfig
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

class LogConfigReader : public CSingleton<LogConfigReader>
{
	friend CSingleton<LogConfigReader>;
private:
	LogConfigReader() = default;
	~LogConfigReader() = default;

private: // variables
	std::unordered_map<std::string, LogConfig> _logger_configs;
	std::map<LogLevel, LogLevelConfig> _level_configs;
	GlobalConfig _globalConfig;
	std::unique_ptr<FileChangeDetector> _fileWatcher;

private: // functions
	void ParseConfigFile();

public: // functions
	void Initialize();
	bool GetLoggerConfig(std::string const &module_name, LogConfig &dest) const
	{
		auto it = _logger_configs.find(module_name);
		if (it != _logger_configs.end())
		{
			dest = it->second;
			return true;
		}
		return false;
	}
	LogLevelConfig const &GetLogLevelConfig(LogLevel level)
	{
		return _level_configs[level];
	}
	GlobalConfig const &GetGlobalConfig() const
	{
		return _globalConfig;
	}
};
