#pragma once

#include "CSingleton.hpp"

#include <string>
#include <chrono>


enum class LogRotationType
{
	NONE,
	DATE,
	SIZE
};

struct LogRotationConfig
{
	LogRotationType Type = LogRotationType::NONE;
	union
	{
		unsigned int FileSize; // in kilobytes
		std::chrono::minutes Date; // in minutes
	} Value;
	int BackupCount = 10;
};

class LogRotationManager : public CSingleton<LogRotationManager>
{
	friend class CSingleton<LogRotationManager>;
private:
	LogRotationManager() = default;
	~LogRotationManager() = default;

public:
	void Check(std::string const &filename, LogRotationConfig const &config);
};
