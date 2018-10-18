#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>

#include <samplog/LogLevel.hpp>
#include "AmxDebugManager.hpp"


class Message
{
public:
	enum class Type
	{
		MESSAGE,
		ACTION_CLEAR
	};

public:
	Message(std::string const &module,
		samplog::LogLevel level, std::string &&msg,
		std::vector<samplog::AmxFuncCallInfo> &&info) :

		type(Type::MESSAGE),
		timestamp(std::chrono::system_clock::now()),
		log_module(std::move(module)),
		loglevel(level),
		text(msg),
		call_info(info)
	{ }
	Message(std::string const &module, Type action) :
		log_module(std::move(module)),
		type(action),
		loglevel(samplog::LogLevel::NONE)
	{ }
	~Message() = default;

	Message(const Message &rhs) = delete;
	Message operator=(const Message &rhs) = delete;

	Message(const Message &&rhs) = delete;
	Message operator=(const Message &&rhs) = delete;

public:
	Type const type;

	std::string const text;
	std::chrono::system_clock::time_point const timestamp;

	std::vector<samplog::AmxFuncCallInfo> const call_info;

	samplog::LogLevel const loglevel;
	std::string const log_module;

};

using Message_t = std::unique_ptr<Message>;
