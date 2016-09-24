#pragma once

#include <string>
#include <memory>
#include <chrono>

using std::string;

#include "loglevel.hpp"
#include "CAmxDebugManager.hpp"


class CMessage
{
public:
	CMessage(string module,
		LogLevel level, string msg,
		AmxFuncCallInfo *info) :

		timestamp(std::chrono::system_clock::now()),
		log_module(std::move(module)),
		loglevel(level),
		text(std::move(msg)),
		call_info(info)
	{ }
	~CMessage()
	{
		if (call_info)
			free(const_cast<AmxFuncCallInfo *>(call_info));
	}

	CMessage(const CMessage &rhs) = delete;
	CMessage operator=(const CMessage &rhs) = delete;

	CMessage(const CMessage &&rhs) = delete;
	CMessage operator=(const CMessage &&rhs) = delete;

public:
	const string text;
	const std::chrono::system_clock::time_point timestamp;

	const AmxFuncCallInfo * const call_info;

	LogLevel const loglevel;
	const string log_module;

};

using Message_t = std::unique_ptr<CMessage>;
