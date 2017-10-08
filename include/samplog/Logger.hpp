#pragma once
#ifndef INC_SAMPLOG_LOGGER_HPP
#define INC_SAMPLOG_LOGGER_HPP

#include "ILogger.hpp"

#include <memory>
#include <functional>


namespace samplog
{
	using Logger_t = std::unique_ptr<ILogger, std::function<void(ILogger*)>>;

	inline Logger_t CreateLogger(const char *module_name)
	{
		return Logger_t(samplog_CreateLogger(module_name), std::mem_fn(&ILogger::Destroy));
	}
}


#endif /* INC_SAMPLOG_LOGGER_HPP */
