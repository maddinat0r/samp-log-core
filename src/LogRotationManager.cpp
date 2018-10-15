#include "LogRotationManager.hpp"

#include <cstdio>
#include <fstream>
#include <vector>
#include <fmt/format.h>
#include <tinydir.h>


void LogRotationManager::Check(std::string const &filepath, LogRotationConfig const &config)
{
	if (config.Type == LogRotationType::NONE)
		return;

	if (config.Type == LogRotationType::SIZE)
	{
	}
	else if (config.Type == LogRotationType::DATE)
	{

	}
}
