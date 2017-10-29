#pragma once

#include "CSingleton.hpp"
#include "samplog/LogLevel.hpp"

#include <chrono>
#include <string>
#include <unordered_map>


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
	union
	{
		unsigned int FileSize; // in megabytes
		std::chrono::hours DateHours; // in hours
	} LogRotationValue;
};

class LogConfigReader : public CSingleton<LogConfigReader>
{
	friend CSingleton<LogConfigReader>;
private:
	LogConfigReader() = default;
	~LogConfigReader() = default;

private: // variables
	std::unordered_map<std::string, LogConfig> _logger_configs;

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
};
