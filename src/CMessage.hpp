#pragma once

#include <string>
#include <memory>

using std::string;
using std::unique_ptr;

#include "loglevel.hpp"


class CMessage
{
public:
	CMessage(string msg, LogLevel level, 
		long line, string file, string func) :
		m_Text(std::move(msg)),
		m_Loglevel(level),
		m_Line(line),
		m_File(std::move(file)),
		m_Function(std::move(func))
	{ }
	~CMessage() = default;

	CMessage(const CMessage &rhs) = delete;
	CMessage operator=(const CMessage &rhs) = delete;

	CMessage(const CMessage &&rhs) = delete;
	CMessage operator=(const CMessage &&rhs) = delete;

public:
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



private:
	string
		m_Text,
		m_File,
		m_Function;

	long m_Line;

	LogLevel m_Loglevel;

};

using Message_t = unique_ptr<CMessage>;
