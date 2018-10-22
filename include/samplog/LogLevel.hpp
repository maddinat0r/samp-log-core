#pragma once
#ifndef INC_SAMPLOG_LOGLEVEL_HPP
#define INC_SAMPLOG_LOGLEVEL_HPP

#ifdef ERROR //because Microsoft
#undef ERROR
#endif


namespace samplog
{
	enum LogLevel
	{
		NONE = 0,
		DEBUG = 1,
		INFO = 2,
		WARNING = 4,
		ERROR = 8,
		FATAL = 16,
		VERBOSE = 32
	};

	inline LogLevel operator|(LogLevel a, LogLevel b)
	{
		return static_cast<LogLevel>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline LogLevel &operator|=(LogLevel &in, LogLevel val)
	{
		return in = in | val;
	}
}


#endif /* INC_SAMPLOG_LOGLEVEL_HPP */
