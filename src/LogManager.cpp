#include <algorithm>
#include <fstream>
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
#include "crashhandler.hpp"
#include "amx/amx2.h"

#include <fmt/format.h>
#include <fmt/time.h>

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

void WriteCallInfoString(Message_t const &msg, fmt::MemoryWriter &log_string)
{
	if (!msg->call_info.empty())
	{
		log_string << " (";
		bool first = true;
		for (auto const &ci : msg->call_info)
		{
			if (!first)
				log_string << " -> ";
			log_string << ci.file << ":" << ci.line;
			first = false;
		}
		log_string << ")";
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


LogManager::LogManager() :
	m_ThreadRunning(true),
	m_DateTimeFormat("{:%x %X}")
{
	crashhandler::Install();

	std::string cfg_time_format;
	if (SampConfigReader::Get()->GetVar("logtimeformat", cfg_time_format))
	{
		//delete brackets
		size_t pos = 0;
		while ((pos = cfg_time_format.find_first_of("[]()")) != std::string::npos)
			cfg_time_format.erase(pos, 1);

		m_DateTimeFormat = "{:" + cfg_time_format + "}";
		// quickly test out the format string
		// will assert if invalid and on Windows
		fmt::format(m_DateTimeFormat, fmt::localtime(std::time(nullptr)));
	}

	LogConfigReader::Get()->Initialize();

	CreateFolder("logs");

	m_WarningLog.open("logs/warnings.log");
	m_ErrorLog.open("logs/errors.log");

	m_Thread = new std::thread(std::bind(&LogManager::Process, this));
}

LogManager::~LogManager()
{
	{
		std::lock_guard<std::mutex> lg(m_QueueMtx);
		m_ThreadRunning = false;
	}
	m_QueueNotifier.notify_one();
	m_Thread->join();
	delete m_Thread;
}

void LogManager::RegisterLogger(Logger *logger)
{
	std::lock_guard<std::mutex> lg(m_LoggersMutex);
	m_Loggers.emplace(logger->GetModuleName(), logger);
}

void LogManager::UnregisterLogger(Logger *logger)
{
	std::lock_guard<std::mutex> lg(m_LoggersMutex);
	m_Loggers.erase(logger->GetModuleName());
	if (m_Loggers.size() == 0) //last logger
		CSingleton::Destroy();
	// TODO we're destroying our class while the mutex is still locked...
}

void LogManager::QueueLogMessage(Message_t &&msg)
{
	{
		std::lock_guard<std::mutex> lg(m_QueueMtx);
		m_LogMsgQueue.push(std::move(msg));
	}
	m_QueueNotifier.notify_one();
}

void LogManager::Process()
{
	std::unique_lock<std::mutex> lk(m_QueueMtx);
	std::unordered_set<std::string> hashed_modules;

	do
	{
		m_QueueNotifier.wait(lk);
		while (!m_LogMsgQueue.empty())
		{
			Message_t msg = std::move(m_LogMsgQueue.front());
			m_LogMsgQueue.pop();

			//manually unlock mutex
			//the whole write-to-file code below has no need to be locked with the
			//message queue mutex; while writing to the log file, new messages can
			//now be queued
			lk.unlock();

			const string &modulename = msg->log_module;
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

			std::string timestamp;
			std::time_t now_c = std::chrono::system_clock::to_time_t(msg->timestamp);
			timestamp = fmt::format(m_DateTimeFormat, fmt::localtime(now_c));

			const char *loglevel_str = GetLogLevelAsString(msg->loglevel);

			// build log string
			fmt::MemoryWriter log_string;

			log_string << msg->text;
			WriteCallInfoString(msg, log_string);

			//default logging
			std::ofstream logfile("logs/" + modulename + ".log",
				std::ofstream::out | std::ofstream::app);
			logfile <<
				"[" << timestamp << "] " <<
				"[" << loglevel_str << "] " <<
				log_string.str() << '\n' << std::flush;


			//per-log-level logging
			std::ofstream *loglevel_file = nullptr;
			if (msg->loglevel == LogLevel::WARNING)
				loglevel_file = &m_WarningLog;
			else if (msg->loglevel == LogLevel::ERROR)
				loglevel_file = &m_ErrorLog;

			if (loglevel_file != nullptr)
			{
				(*loglevel_file) <<
					"[" << timestamp << "] " <<
					"[" << modulename << "] " <<
					log_string.str() << '\n' << std::flush;
			}

			//lock the log message queue again (because while-condition and cv.wait)
			lk.lock();
		}
	} while (m_ThreadRunning);
}
