#pragma once

#include "Singleton.hpp"

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>


enum class LogRotationType
{
	NONE,
	DATE,
	SIZE
};

enum class LogRotationTimeType
{
	DAILY,
	WEEKLY,
	MONTHLY
};

struct LogRotationConfig
{
	LogRotationType Type = LogRotationType::NONE;
	union
	{
		unsigned int FileSize; // in kilobytes
		LogRotationTimeType Date;
	} Value;
	int BackupCount = 10;
};

class LogRotationManager : public Singleton<LogRotationManager>
{
	friend class Singleton<LogRotationManager>;
private:
	LogRotationManager();
	~LogRotationManager();

private:
	std::mutex _rotationEntriesLock;
	std::unordered_map<std::string, LogRotationConfig> _rotationEntries;

	std::atomic<bool> _threadRunning;
	std::thread _thread;

private:
	void Process();

	void CheckDateRotation(std::string const &file_path,
		LogRotationTimeType const type, int const backup_count);
	void CheckSizeRotation(std::string const &file_path,
		unsigned int const max_size, int const backup_count);

public:
	inline void RegisterLogFile(std::string const &file_path,
		LogRotationConfig const &config)
	{
		std::lock_guard<std::mutex> lock(_rotationEntriesLock);
		auto it = _rotationEntries.find(file_path);
		if (it != _rotationEntries.end())
			_rotationEntries.erase(file_path);

		if (config.Type != LogRotationType::NONE)
			_rotationEntries.emplace(file_path, config);
	}
	inline void UnregisterLogFile(std::string const &file_path)
	{
		std::lock_guard<std::mutex> lock(_rotationEntriesLock);
		_rotationEntries.erase(file_path);
	}
};
