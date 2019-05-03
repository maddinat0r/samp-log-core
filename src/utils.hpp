#pragma once

#include <string>
#include <fmt/color.h>

#include "samplog/LogLevel.hpp"


namespace utils
{
	const char *GetLogLevelAsString(samplog::LogLevel level);
	fmt::rgb GetLogLevelColor(samplog::LogLevel level);

	void CreateFolder(std::string foldername);
	void EnsureFolders(std::string const &path);
	void EnsureTerminalColorSupport();
}
