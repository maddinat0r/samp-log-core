#pragma once

#include "Singleton.hpp"
#include "samplog/LogLevel.hpp"
#include "FileChangeDetector.hpp"
#include "Logger.hpp"

#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>


using samplog::LogLevel;


struct LogLevelConfig
{
	bool PrintToConsole = false;
};

struct GlobalConfig
{
	std::string LogTimeFormat = "%x %X";
	bool DisableDebugInfo = false;
	bool EnableColors = false;
	std::string LogsRootFolder = "logs/";
};

class LogConfig : public Singleton<LogConfig>
{
	friend Singleton<LogConfig>;
public:
	using ConfigUpdateEvent_t = std::function<void(Logger::Config const &)>;

private:
	LogConfig() = default;
	~LogConfig() = default;

private: // variables
	std::mutex _configLock;
	std::unordered_map<std::string, Logger::Config> _loggerConfigs;
	std::unordered_map<std::string, ConfigUpdateEvent_t> _loggerConfigEvents;
	std::map<LogLevel, LogLevelConfig> _levelConfigs;
	GlobalConfig _globalConfig;
	std::unique_ptr<FileChangeDetector> _fileWatcher;

private: // functions
	void ParseConfigFile();
	void AddLoggerConfig(std::string const &module_name, Logger::Config &&config)
	{
		auto entry = _loggerConfigs.emplace(module_name, std::move(config));

		// trigger config refresh for logger
		auto it = _loggerConfigEvents.find(module_name);
		if (it != _loggerConfigEvents.end())
			it->second(entry.first->second);
	}

public: // functions
	void Initialize();

	inline void SubscribeLogger(Logger *logger, ConfigUpdateEvent_t &&cb)
	{
		auto e_it = _loggerConfigEvents.emplace(logger->GetModuleName(),
			std::forward<ConfigUpdateEvent_t>(cb));

		std::lock_guard<std::mutex> lock(_configLock);
		auto it = _loggerConfigs.find(logger->GetModuleName());
		if (it != _loggerConfigs.end())
			e_it.first->second(it->second);
	}
	inline void UnsubscribeLogger(Logger *logger)
	{
		_loggerConfigEvents.erase(logger->GetModuleName());
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
