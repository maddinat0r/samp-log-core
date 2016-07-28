#pragma once

#include <string>
#include <memory>
#include <chrono>

using std::string;

#include "loglevel.hpp"


class CMessage
{
public:
	CMessage(string module,
		LogLevel level, string msg,
		int line, string file, string func) :

		timestamp(std::chrono::system_clock::now()),
		log_module(std::move(module)),
		loglevel(level),
		text(std::move(msg)),
		line(line),
		file(std::move(file)),
		function(std::move(func))
	{ }
	~CMessage() = default;

	CMessage(const CMessage &rhs) = delete;
	CMessage operator=(const CMessage &rhs) = delete;

	CMessage(const CMessage &&rhs) = delete;
	CMessage operator=(const CMessage &&rhs) = delete;

public:
	const string
		text,
		file,
		function;

	int const line;

	const std::chrono::system_clock::time_point timestamp;

	LogLevel const loglevel;

	const string log_module;

};

using Message_t = std::unique_ptr<CMessage>;
