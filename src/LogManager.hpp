#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <fstream>

#include "CSingleton.hpp"
#include "CMessage.hpp"

#include <samplog/LogLevel.hpp>

class Logger;


class LogManager : public CSingleton<LogManager>
{
	friend class CSingleton<LogManager>;
private:
	LogManager();
	~LogManager();
	LogManager(LogManager&&) = delete;
	LogManager& operator=(LogManager&&) = delete;
	LogManager(const LogManager&) = delete;
	LogManager& operator=(const LogManager&) = delete;

public:
	void RegisterLogger(Logger *logger);
	void UnregisterLogger(Logger *logger);
	void QueueLogMessage(Message_t &&msg);

	void LogInternal(samplog::LogLevel level, std::string msg)
	{
		QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
			"log-core", level, msg, { })));
	}

private:
	void Process();

private:
	std::ofstream
		m_WarningLog,
		m_ErrorLog,
		m_FatalLog;

	std::mutex m_LoggersMutex;
	std::unordered_map<std::string, Logger *> m_Loggers;

	std::atomic<bool> m_ThreadRunning;
	std::thread *m_Thread = nullptr;

	std::mutex m_QueueMtx;
	std::condition_variable m_QueueNotifier;
	std::queue<Message_t> m_LogMsgQueue;

	std::string m_DateTimeFormat;
};
