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

#include "Singleton.hpp"
#include "Message.hpp"

#include <samplog/LogLevel.hpp>

class Logger;


class LogManager : public Singleton<LogManager>
{
	friend class Singleton<LogManager>;
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
		static std::string const module_name("log-core");
		QueueLogMessage(std::unique_ptr<Message>(new Message(
			module_name, level, std::move(msg), { })));
	}

private:
	void Process();
	inline std::string GetLogFilePath(std::string const &modulename)
	{
		return "logs/" + modulename + ".log";
	}

private:
	std::ofstream
		_warningLog,
		_errorLog,
		_fatalLog;

	std::mutex _loggersMutex;
	std::unordered_map<std::string, Logger *> _loggers;

	std::atomic<bool> _threadRunning;
	std::thread *_thread;

	std::mutex _queueMtx;
	std::condition_variable _queueNotifier;
	std::queue<Message_t> _messageQueue;
};
