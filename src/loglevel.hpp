#pragma once

#ifdef ERROR //great job M$
#  undef ERROR
#endif


enum LogLevel
{
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 4,
	ERROR = 8,
};
