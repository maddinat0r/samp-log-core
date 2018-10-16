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

class LogRotationManager : public CSingleton<LogRotationManager>
{
	friend class CSingleton<LogRotationManager>;
private:
	LogRotationManager() = default;
	~LogRotationManager() = default;

public:
	void Check(std::string const &filename, LogRotationConfig const &config);
};
