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
/*public:
	inline const string &GetText() const
	{
		return m_Text;
	}
	inline long GetLine() const
	{
		return m_Line;
	}
	inline const string &GetFileName() const
	{
		return m_File;
	}
	inline const string &GetFunctionName() const
	{
		return m_Function;
	}
	inline const std::chrono::steady_clock::time_point &GetTime() const
	{
		return m_Timestamp;
	}
	inline const LogLevel GetLevel() const
	{
		return m_Loglevel;
	}
	
private:
	string
		m_Text,
		m_File,
		m_Function;

	long m_Line;

	std::chrono::steady_clock::time_point m_Timestamp;

	LogLevel m_Loglevel;

	string
		m_ModuleName,
		m_FileName;
	*/
};

using Message_t = std::unique_ptr<CMessage>;
