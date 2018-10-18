#include <algorithm>
#include <fstream>
#include <iostream>
#include <ctime>
#include <unordered_set>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "LogManager.hpp"
#include "Logger.hpp"
#include "SampConfigReader.hpp"
#include "LogConfigReader.hpp"
#include "LogRotationManager.hpp"
#include "crashhandler.hpp"
#include "amx/amx2.h"

#include <fmt/format.h>
#include <fmt/time.h>
#include <fmt/color.h>

using samplog::LogLevel;


const char *GetLogLevelAsString(LogLevel level)
{
	switch (level)
	{
		case LogLevel::DEBUG:
			return "DEBUG";
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::WARNING:
			return "WARNING";
		case LogLevel::ERROR:
			return "ERROR";
		case LogLevel::FATAL:
			return "FATAL";
		case LogLevel::VERBOSE:
			return "VERBOSE";
	}
	return "<unknown>";
}

fmt::rgb GetLogLevelColor(LogLevel level)
{
	switch (level)
	{
	case LogLevel::DEBUG:
		return fmt::color::green;
	case LogLevel::INFO:
		return fmt::color::royal_blue;
	case LogLevel::WARNING:
		return fmt::color::orange;
	case LogLevel::ERROR:
		return fmt::color::red;
	case LogLevel::FATAL:
		return fmt::color::red;
	case LogLevel::VERBOSE:
		return fmt::color::white_smoke;
	}
	return fmt::color::white;
}

void WriteCallInfoString(Message_t const &msg, fmt::memory_buffer &log_string)
{
	if (!msg->call_info.empty())
	{
		fmt::format_to(log_string, " (");
		bool first = true;
		for (auto const &ci : msg->call_info)
		{
			if (!first)
				fmt::format_to(log_string, " -> ");
			fmt::format_to(log_string, "{:s}:{:d}", ci.file, ci.line);
			first = false;
		}
		fmt::format_to(log_string, ")");
	}
}

void CreateFolder(std::string foldername)
{
#ifdef WIN32
	std::replace(foldername.begin(), foldername.end(), '/', '\\');
	CreateDirectoryA(foldername.c_str(), NULL);
#else
	std::replace(foldername.begin(), foldername.end(), '\\', '/');
	mkdir(foldername.c_str(), ACCESSPERMS);
#endif
}

void EnsureTerminalColorSupport()
{
	static bool enabled = false;
	if (enabled)
		return;

#ifdef WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console == INVALID_HANDLE_VALUE)
		return;

	DWORD console_opts;
	if (!GetConsoleMode(console, &console_opts))
		return;

	if (!SetConsoleMode(console, console_opts | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
		return;
#endif
	enabled = true;
}


LogManager::LogManager() :
	_threadRunning(true),
	_thread(nullptr)
{
	crashhandler::Install();

	CreateFolder("logs");

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

void LogManager::RegisterLogger(Logger *logger)
{
	std::lock_guard<std::mutex> lg(_loggersMutex);
	_loggers.emplace(logger->GetModuleName(), logger);
}

void LogManager::UnregisterLogger(Logger *logger)
{
	bool is_last = false;
	{
		std::lock_guard<std::mutex> lg(_loggersMutex);
		_loggers.erase(logger->GetModuleName());
		is_last = (_loggers.size() == 0);
	}
	if (is_last) //last logger
		Singleton::Destroy();
}

void LogManager::QueueLogMessage(Message_t &&msg)
{
	{
		std::lock_guard<std::mutex> lg(_queueMtx);
		_messageQueue.push(std::move(msg));
	}
	_queueNotifier.notify_one();
}

void LogManager::Process()
{
	LogConfig::Get()->Initialize();

	std::unique_lock<std::mutex> lk(_queueMtx);
	std::unordered_set<std::string> hashed_modules;

	do
	{
		// this check seems unnecessary at first, however it's needed in
		// case internal log messages are already queued (for example 
		// LogConfig::Initialize logs warnings if there are parsing errors)
		if (_messageQueue.empty())
		{
			// we need to wake up in at least every minute to properly check for
			// date-based log file rotation
			_queueNotifier.wait_for(lk, std::chrono::seconds(45));
		}

		// check for date-based log file rotation
		{
			std::lock_guard<std::mutex> lg(_loggersMutex);
			for (auto const &p : _loggers)
			{
				LoggerConfig log_config;
				if (!LogConfig::Get()->GetLoggerConfig(p.first, log_config))
					continue;

				LogRotationManager::Get()->Check(GetLogFilePath(p.first), log_config.Rotation);
			}
		}

		while (!_messageQueue.empty())
		{
			Message_t msg = std::move(_messageQueue.front());
			_messageQueue.pop();

			//manually unlock mutex
			//the whole write-to-file code below has no need to be locked with the
			//message queue mutex; while writing to the log file, new messages can
			//now be queued
			lk.unlock();

			std::string const &modulename = msg->log_module;
			std::string const module_log_filename = GetLogFilePath(modulename);

			if (hashed_modules.count(modulename) == 0)
			{
				//create possibly non-existing folders before opening log file
				size_t pos = 0;
				while ((pos = modulename.find('/', pos)) != std::string::npos)
				{
					CreateFolder("logs/" + modulename.substr(0, pos++));
				}

				hashed_modules.insert(modulename);
			}


			if (msg->type == Message::Type::MESSAGE)
			{
				std::time_t now_c = std::chrono::system_clock::to_time_t(msg->timestamp);
				auto const &time_format = LogConfig::Get()->GetGlobalConfig().LogTimeFormat;
				std::string timestamp = fmt::format("{:" + time_format + "}", fmt::localtime(now_c));

				const char *loglevel_str = GetLogLevelAsString(msg->loglevel);

				// build log string
				fmt::memory_buffer log_string_buf;

				fmt::format_to(log_string_buf, "{:s}", msg->text);
				WriteCallInfoString(msg, log_string_buf);

				std::string const log_string = fmt::to_string(log_string_buf);

				//default logging
				{
					std::ofstream logfile(module_log_filename,
						std::ofstream::out | std::ofstream::app);
					logfile <<
						"[" << timestamp << "] " <<
						"[" << loglevel_str << "] " <<
						log_string << '\n' << std::flush;
				}
				LoggerConfig log_config;
				LogConfig::Get()->GetLoggerConfig(modulename, log_config);

				LogRotationManager::Get()->Check(module_log_filename, log_config.Rotation);


				//per-log-level logging
				std::ofstream *loglevel_file = nullptr;
				if (msg->loglevel == LogLevel::WARNING)
					loglevel_file = &_warningLog;
				else if (msg->loglevel == LogLevel::ERROR)
					loglevel_file = &_errorLog;
				else if (msg->loglevel == LogLevel::FATAL)
					loglevel_file = &_fatalLog;

				if (loglevel_file != nullptr)
				{
					(*loglevel_file) <<
						"[" << timestamp << "] " <<
						"[" << modulename << "] " <<
						log_string << '\n' << std::flush;
				}

				auto const &level_config = LogConfig::Get()->GetLogLevelConfig(msg->loglevel);

				if (log_config.PrintToConsole || level_config.PrintToConsole)
				{
					if (LogConfig::Get()->GetGlobalConfig().EnableColors)
					{
						EnsureTerminalColorSupport();

						fmt::print("[");
						fmt::print(fmt::rgb(255, 255, 150), timestamp);
						fmt::print("] [");
						fmt::print(fmt::color::sandy_brown, modulename);
						fmt::print("] [");
						auto loglevel_color = GetLogLevelColor(msg->loglevel);
						if (msg->loglevel == LogLevel::FATAL)
							fmt::print(fmt::color::white, loglevel_color, loglevel_str);
						else
							fmt::print(loglevel_color, loglevel_str);
						fmt::print("] {:s}\n", log_string);
					}
					else
					{
						fmt::print("[{:s}] [{:s}] [{:s}] {:s}\n",
							timestamp, modulename, loglevel_str, log_string);
					}
				}
			}
			else if (msg->type == Message::Type::ACTION_CLEAR)
			{
				// create file if it doesn't exist, and truncate whole content
				std::ofstream logfile(module_log_filename, std::ofstream::trunc);
			}

			//lock the log message queue again (because while-condition and cv.wait)
			lk.lock();
		}
	} while (_threadRunning);
}
