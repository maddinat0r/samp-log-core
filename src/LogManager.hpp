#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <functional>

#include "Singleton.hpp"
#include "Logger.hpp"

#include <samplog/LogLevel.hpp>


class LogManager : public Singleton<LogManager>
{
	friend class Singleton<LogManager>;
public:
	using Action_t = std::function<void()>;

private:
	LogManager();
	~LogManager();
	LogManager(LogManager&&) = delete;
	LogManager& operator=(LogManager&&) = delete;
	LogManager(const LogManager&) = delete;
	LogManager& operator=(const LogManager&) = delete;

public:
	void Queue(Action_t &&action);

	void WriteLevelLogString(std::string const &time,
		samplog::LogLevel level, std::string const &module_name,
		std::string const &message);

	inline void LogInternal(samplog::LogLevel level, std::string msg)
	{
		_internalLogger.Log(level, std::move(msg));
	}

private:
	void Process();

private:
	std::atomic<bool> _threadRunning;
	std::thread *_thread;

	std::mutex _queueMtx;
	std::condition_variable _queueNotifier;
	std::queue<Action_t> _queue;

	Logger _internalLogger;
};
