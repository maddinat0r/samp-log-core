/** ==========================================================================
 * 2012 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 * Filename:g2loglevels.cpp  Part of Framework for Logging and Design By Contract
 * Created: 2012 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited. First published at KjellKod.cc
 * ********************************************* */

#include "g2loglevels.hpp"

namespace g2
{
	std::string getLevelName(const LOGLEVEL &level)
	{
		switch (level)
		{
		case LOGLEVEL::DEBUG:
			return "DEBUG";
		case LOGLEVEL::INFO:
			return "INFO";
		case LOGLEVEL::WARNING:
			return "WARNING";
		case LOGLEVEL::ERROR:
			return "ERROR";
		case LOGLEVEL::FATAL:
			return "FATAL";
		default:
			return std::string();
		}
	}
}
