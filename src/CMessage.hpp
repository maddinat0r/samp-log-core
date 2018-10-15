#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>

using std::string;

#include <samplog/LogLevel.hpp>
#include "CAmxDebugManager.hpp"


class CMessage
{
public:
	enum class Type
	{
		MESSAGE,
		ACTION_CLEAR
	};
public:
	CMessage(string module,
		samplog::LogLevel level, string msg,
		std::vector<samplog::AmxFuncCallInfo> &&info) :

		type(Type::MESSAGE),
		timestamp(std::chrono::system_clock::now()),
		log_module(std::move(module)),
		loglevel(level),
		text(std::move(msg)),
		call_info(std::move(info))
	{ }
	CMessage(string module, Type action) :
		log_module(std::move(module)),
		type(action),
		loglevel(samplog::LogLevel::NONE)
	{ }
	~CMessage() = default;

	CMessage(const CMessage &rhs) = delete;
	CMessage operator=(const CMessage &rhs) = delete;

	CMessage(const CMessage &&rhs) = delete;
	CMessage operator=(const CMessage &&rhs) = delete;

public:
	const Type type;

	const string text;
	const std::chrono::system_clock::time_point timestamp;

	const std::vector<samplog::AmxFuncCallInfo> call_info;

	samplog::LogLevel const loglevel;
	const string log_module;

};

using Message_t = std::unique_ptr<CMessage>;
