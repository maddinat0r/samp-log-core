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
	CMessage(string module,
		samplog::LogLevel level, string msg,
		std::vector<AmxFuncCallInfo> &&info) :

		timestamp(std::chrono::system_clock::now()),
		log_module(std::move(module)),
		loglevel(level),
		text(std::move(msg)),
		call_info(std::move(info))
	{ }
	~CMessage() = default;

	CMessage(const CMessage &rhs) = delete;
	CMessage operator=(const CMessage &rhs) = delete;

	CMessage(const CMessage &&rhs) = delete;
	CMessage operator=(const CMessage &&rhs) = delete;

public:
	const string text;
	const std::chrono::system_clock::time_point timestamp;

	const std::vector<AmxFuncCallInfo> call_info;

	samplog::LogLevel const loglevel;
	const string log_module;

};

using Message_t = std::unique_ptr<CMessage>;
