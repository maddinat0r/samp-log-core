/** ==========================================================================
 * 2012 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 * Filename:g2loglevels.hpp  Part of Framework for Logging and Design By Contract
 * Created: 2012 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited. First published at KjellKod.cc
 * ********************************************* */

#pragma once

#include <string>
#include <type_traits>

#ifdef ERROR //holy shit Microsoft
#undef ERROR
#endif

enum class LOGLEVEL : unsigned int
{
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 4,
	ERROR = 8,
	FATAL = 16,

	FATAL_SIGNAL = 1024,
	FATAL_EXCEPTION = 1025
};

namespace g2
{
	using LOGLEVEL_type = std::underlying_type<LOGLEVEL>::type;

	inline bool logLevel(const LOGLEVEL &level_src, const LOGLEVEL &level_check)
	{
		return (static_cast<LOGLEVEL_type>(level_src) & static_cast<LOGLEVEL_type>(level_check))
			== static_cast<LOGLEVEL_type>(level_check);
	}

	inline bool wasFatal(const LOGLEVEL &level)
	{
		return g2::logLevel(level, LOGLEVEL::FATAL);
	}

	inline void setLogLevel(LOGLEVEL &level_src, const LOGLEVEL &level_target)
	{
		level_src = static_cast<LOGLEVEL>(
			static_cast<LOGLEVEL_type>(level_src) | static_cast<LOGLEVEL_type>(level_target));
	}

	inline void unsetLogLevel(LOGLEVEL &level_src, const LOGLEVEL &level_target)
	{
		level_src = static_cast<LOGLEVEL>(
			static_cast<LOGLEVEL_type>(level_src) & ~static_cast<LOGLEVEL_type>(level_target));
	}

	std::string getLevelName(const LOGLEVEL &level);

} //namespace g2
