#pragma once

#include "CSingleton.hpp"
#include "samplog/LogLevel.hpp"
#include "FileChangeDetector.hpp"

#include <chrono>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>


using samplog::LogLevel;


struct LogConfig
{
	enum class LogRotationType
	{
		NONE,
		DATE,
		SIZE
	};

	LogLevel Level = LogLevel::ERROR | LogLevel::WARNING | LogLevel::FATAL;
	LogRotationType Rotation = LogRotationType::NONE;
	bool PrintToConsole = false;
	union
	{
		unsigned int FileSize; // in megabytes
		std::chrono::hours DateHours; // in hours
	} LogRotationValue;
};

struct LogLevelConfig
{
	bool PrintToConsole = false;
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
	LogLevelConfig GetLogLevelConfig(LogLevel level)
	{
		return _level_configs[level];
	}
};
