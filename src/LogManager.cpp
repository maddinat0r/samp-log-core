#include <fstream>

#ifdef WIN32
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "LogManager.hpp"
#include "LogConfig.hpp"
#include "crashhandler.hpp"
#include "utils.hpp"

#include <memory>
#include <map>

using samplog::LogLevel;


LogManager::LogManager() :
	_threadRunning(true),
	_thread(nullptr),
	_internalLogger("log-core")
{
	crashhandler::Install();

	_thread = new std::thread(std::bind(&LogManager::Process, this));
}

LogManager::~LogManager()
{
	{
		std::lock_guard<std::mutex> lg(_queueMtx);
		_threadRunning = false;
	}
	_queueNotifier.notify_one();
	_thread->join();
	delete _thread;
}

void LogManager::Queue(Action_t &&action)
{
	{
		std::lock_guard<std::mutex> lg(_queueMtx);
		_queue.push(std::move(action));
	}
	_queueNotifier.notify_one();
}

void LogManager::WriteLevelLogString(std::string const &time, LogLevel level,
	std::string const &module_name, std::string const &message)
{
	static const std::map<LogLevel, std::string> level_files{
		{ LogLevel::WARNING, "warnings.log" },
		{ LogLevel::ERROR, "errors.log" },
		{ LogLevel::FATAL, "fatals.log" }
	};

	auto it = level_files.find(level);
	if (it != level_files.end())
	{
		auto file_path = LogConfig::Get()->GetGlobalConfig().LogsRootFolder + it->second;
		utils::EnsureFolders(file_path);
		std::ofstream loglevel_file(file_path,
			std::ofstream::out | std::ofstream::app);
		loglevel_file <<
			"[" << time << "] " <<
			"[" << module_name << "] " <<
			message << '\n' << std::flush;
	}
}

void LogManager::Process()
{
	std::unique_lock<std::mutex> lk(_queueMtx);

	do
	{
		_queueNotifier.wait(lk);

		while (!_queue.empty())
		{
			auto action = std::move(_queue.front());
			_queue.pop();

			//manually unlock mutex
			//the whole write-to-file code below has no need to be locked with the
			//message queue mutex; while writing to the log file, new messages can
			//now be queued
			lk.unlock();
			action();
			lk.lock();
		}
	} while (_threadRunning);
}
