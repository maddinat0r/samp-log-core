#include <algorithm>

#ifdef WIN32
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "utils.hpp"

using samplog::LogLevel;


const char *utils::GetLogLevelAsString(samplog::LogLevel level)
{
	switch (level)
	{
	case LogLevel::DEBUG:
		return "DEBUG";
	case LogLevel::INFO:
		return "INFO";
	case LogLevel::WARNING:
		return "WARNING";
	case LogLevel::ERROR:
		return "ERROR";
	case LogLevel::FATAL:
		return "FATAL";
	case LogLevel::VERBOSE:
		return "VERBOSE";
	}
	return "<unknown>";
}

fmt::rgb utils::GetLogLevelColor(samplog::LogLevel level)
{
	switch (level)
	{
	case LogLevel::DEBUG:
		return fmt::color::green;
	case LogLevel::INFO:
		return fmt::color::royal_blue;
	case LogLevel::WARNING:
		return fmt::color::orange;
	case LogLevel::ERROR:
		return fmt::color::red;
	case LogLevel::FATAL:
		return fmt::color::red;
	case LogLevel::VERBOSE:
		return fmt::color::white_smoke;
	}
	return fmt::color::white;
}

void utils::CreateFolder(std::string foldername)
{
#ifdef WIN32
	std::replace(foldername.begin(), foldername.end(), '/', '\\');
	CreateDirectoryA(foldername.c_str(), NULL);
#else
	std::replace(foldername.begin(), foldername.end(), '\\', '/');
	mkdir(foldername.c_str(), ACCESSPERMS);
#endif
}

void utils::EnsureTerminalColorSupport()
{
	static bool enabled = false;
	if (enabled)
		return;

#ifdef WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console == INVALID_HANDLE_VALUE)
		return;

	DWORD console_opts;
	if (!GetConsoleMode(console, &console_opts))
		return;

	if (!SetConsoleMode(console, console_opts | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
		return;
#endif
	enabled = true;
}
