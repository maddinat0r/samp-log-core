#pragma once

#ifdef ERROR //because Microsoft
#undef ERROR
#endif


namespace samplog
{
	enum class LogLevel : unsigned int
	{
		NONE = 0,
		DEBUG = 1,
		INFO = 2,
		WARNING = 4,
		ERROR = 8,
	};
}
