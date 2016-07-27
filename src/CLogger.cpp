#include <algorithm>
#include <fstream>
#include <ctime>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "CLogger.hpp"
#include "CSampConfigReader.hpp"
#include "CMessage.hpp"
#include "crashhandler.hpp"



CLogger::CLogger(std::string module) : 
	m_ModuleName(module),
	m_FileName("logs/" + module + ".log")
{
	//create possibly non-existing folders before opening log file
	size_t pos = 0;
	while ((pos = m_FileName.find('/', pos)) != std::string::npos)
	{
		auto dir = m_FileName.substr(0, pos++);
#ifdef WIN32
		std::replace(dir.begin(), dir.end(), '/', '\\');
		CreateDirectoryA(dir.c_str(), NULL);
#else
		mkdir(dir.c_str(), ACCESSPERMS);
#endif
	}

	//set default log level
	m_LogLevel.store(LogLevel::NONE, std::memory_order_release);
	SetLogLevel(LogLevel::ERROR, true);
	SetLogLevel(LogLevel::WARNING, true);
}

void CLogger::SetLogLevel(const LogLevel level, bool enabled)
{
	if (level == LogLevel::NONE)
	{
		m_LogLevel.store(LogLevel::NONE, std::memory_order_release);
	}
	else
	{
		LogLevel current_loglevel = m_LogLevel.load(std::memory_order_acquire);
		if (enabled)
		{
			current_loglevel = static_cast<LogLevel>(
				static_cast<LogLevel_ut>(current_loglevel) | static_cast<LogLevel_ut>(level));
		}
		else
		{
			current_loglevel = static_cast<LogLevel>(
				static_cast<LogLevel_ut>(current_loglevel) & ~static_cast<LogLevel_ut>(level));
		}
		m_LogLevel.store(current_loglevel, std::memory_order_release);
	}
}

bool CLogger::IsLogLevel(const LogLevel level)
{
	LogLevel current_loglevel = m_LogLevel.load(std::memory_order_acquire);
	return current_loglevel & level;
}

void CLogger::Log(const char *msg, 
	const LogLevel level, int line/* = 0*/, const char *file/* = ""*/,
	const char *function/* = ""*/)
{
/*#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	g3::installSignalHandlerForThread();
#endif*/

	if (IsLogLevel(level) == false)
		return;

	Message_t message(new CMessage(
		m_FileName, m_ModuleName, level, msg ? msg : "", 
		line, file ? file : "", function? function : ""));
	CLogManager::Get()->QueueLogMessage(std::move(message));
}

void CLogger::Destroy()
{
	delete this;
}

ILogger *CreateLoggerPtr(const char *modulename)
{
	return new CLogger(modulename);
}


CLogManager::CLogManager() :
	m_ThreadRunning(true),
	m_WarningLog("logs/warnings.log"),
	m_ErrorLog("logs/errors.log")
{
	crashhandler::Install();

	if (CSampConfigReader::Get()->GetVar("logtimeformat", m_DateTimeFormat))
	{
		//delete brackets
		size_t pos = 0;
		while ((pos = m_DateTimeFormat.find_first_of("[]()")) != std::string::npos)
			m_DateTimeFormat.erase(pos, 1);
	}
	else
	{
		m_DateTimeFormat = "%x %X";
	}


	m_Thread = new std::thread(std::bind(&CLogManager::Process, this));
}

CLogManager::~CLogManager()
{
	m_ThreadRunning = false;
	m_Thread->join();
	delete m_Thread;
}

void CLogManager::QueueLogMessage(Message_t &&msg)
{
	{
		std::lock_guard<std::mutex> lg(m_QueueMtx);
		m_LogMsgQueue.push(std::move(msg));
	}
	m_QueueNotifier.notify_one();
}

void CLogManager::Process()
{
	std::unique_lock<std::mutex> lk(m_QueueMtx);
	std::function<bool()> condition = [this]() { return !m_LogMsgQueue.empty(); };

	do
	{
		m_QueueNotifier.wait(lk, condition);
		Message_t &msg = m_LogMsgQueue.front();

		char timestamp[64];
		std::time_t now_c = std::chrono::system_clock::to_time_t(msg->timestamp);
		std::strftime(timestamp, sizeof(timestamp)/sizeof(char), 
			m_DateTimeFormat.c_str(), std::localtime(&now_c));
		
		const char *loglevel_str = "<unknown>";
		switch (msg->loglevel)
		{
		case LogLevel::DEBUG:
			loglevel_str = "DEBUG";
			break;
		case LogLevel::INFO:
			loglevel_str = "INFO";
			break;
		case LogLevel::WARNING:
			loglevel_str = "WARNING";
			break;
		case LogLevel::ERROR:
			loglevel_str = "ERROR";
			break;
		}


		//default logging
		std::ofstream logfile(msg->log_filename, std::ofstream::out | std::ofstream::app);
		logfile <<
			"[" << timestamp << "] " <<
			"[" << loglevel_str << "] " <<
			msg->text;
		if (msg->line != 0)
		{
			logfile << " (" << msg->file << ":" << msg->line << ")";
		}
		logfile << '\n' << std::flush;


		//log-level logging
		std::ofstream *loglevel_file = nullptr;
		if (msg->loglevel & LogLevel::WARNING)
			loglevel_file = &m_WarningLog;
		else if (msg->loglevel & LogLevel::ERROR)
			loglevel_file = &m_ErrorLog;

		if(loglevel_file != nullptr)
		{
			(*loglevel_file) <<
				"[" << timestamp << "] " <<
				"[" << msg->log_module << "] " <<
				msg->text;
			if (msg->line != 0)
			{
				(*loglevel_file) << " (" << msg->file << ":" << msg->line << ")";
			}
			(*loglevel_file) << '\n' << std::flush;
		}

		m_LogMsgQueue.pop();
	} while (m_ThreadRunning || (m_ThreadRunning == false && condition()) );
}
