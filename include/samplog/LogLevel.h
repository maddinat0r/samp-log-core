#pragma once
#ifndef INC_SAMPLOG_LOGLEVEL_H
#define INC_SAMPLOG_LOGLEVEL_H

#ifdef ERROR //because Microsoft
#undef ERROR
#endif


enum samplog_LogLevel
{
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 4,
	ERROR = 8,
	FATAL = 16,
	VERBOSE = 32,
};

#ifdef __cplusplus
inline samplog_LogLevel operator|(samplog_LogLevel a, samplog_LogLevel b)
{
	return static_cast<samplog_LogLevel>(static_cast<int>(a) | static_cast<int>(b));
}

namespace samplog
{
	typedef samplog_LogLevel LogLevel;
}
#endif /* __cplusplus */


#endif /* INC_SAMPLOG_LOGLEVEL_H */
