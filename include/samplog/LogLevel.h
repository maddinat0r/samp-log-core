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
};

#ifdef __cplusplus
namespace samplog
{
	typedef samplog_LogLevel LogLevel
}
#endif /* __cplusplus */


#endif /* INC_SAMPLOG_LOGLEVEL_H */
