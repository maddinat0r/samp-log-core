#pragma once

#include <string>
#include <memory>
#include <chrono>

using std::string;

#include "loglevel.hpp"


class CMessage
{
public:
	CMessage(string filename, string module,
		LogLevel level, string msg,
		long line, string file, string func) :

		timestamp(std::chrono::system_clock::now()),
		log_filename(std::move(filename)),
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

	const long line;

	const std::chrono::system_clock::time_point timestamp;

	const LogLevel loglevel;

	const string
		log_module,
		log_filename;

};

using Message_t = std::unique_ptr<CMessage>;
