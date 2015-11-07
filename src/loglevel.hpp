#pragma once


enum class LogLevel : unsigned int
{
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 4,
	ERROR = 8,
};

using LogLevel_ut = std::underlying_type<LogLevel>::type;

inline bool operator&(const LogLevel lhs, const LogLevel rhs)
{
	return (static_cast<LogLevel_ut>(lhs) & static_cast<LogLevel_ut>(rhs))
		== static_cast<LogLevel_ut>(rhs);
}

inline LogLevel operator|=(const LogLevel lhs, const LogLevel rhs)
{
	return static_cast<LogLevel>(
		static_cast<LogLevel_ut>(lhs) | static_cast<LogLevel_ut>(rhs));
}
