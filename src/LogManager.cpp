#include <fstream>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "LogManager.hpp"
#include "crashhandler.hpp"
#include "utils.hpp"

using samplog::LogLevel;


LogManager::LogManager() :
	_threadRunning(true),
	_thread(nullptr),
	_internalLogger("log-core")
{
	crashhandler::Install();

	utils::CreateFolder("logs");

	_warningLog.open("logs/warnings.log");
	_errorLog.open("logs/errors.log");
	_fatalLog.open("logs/fatals.log");

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

	_warningLog.close();
	_errorLog.close();
	_fatalLog.close();
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
	std::ofstream *loglevel_file = nullptr;
	if (level == LogLevel::WARNING)
		loglevel_file = &_warningLog;
	else if (level == LogLevel::ERROR)
		loglevel_file = &_errorLog;
	else if (level == LogLevel::FATAL)
		loglevel_file = &_fatalLog;

	if (loglevel_file != nullptr)
	{
		(*loglevel_file) <<
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
