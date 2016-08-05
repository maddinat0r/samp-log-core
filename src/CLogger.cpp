#include <algorithm>
#include <fstream>
#include <ctime>
#include <set>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#include "CLogger.hpp"
#include "CSampConfigReader.hpp"
#include "crashhandler.hpp"
#include "amx/amx2.h"

#include <fmt/format.h>


CLogManager::CLogManager() :
	m_ThreadRunning(true)
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

	CreateFolder("logs");

	m_WarningLog.open("logs/warnings.log");
	m_ErrorLog.open("logs/errors.log");

	m_Thread = new std::thread(std::bind(&CLogManager::Process, this));
}

CLogManager::~CLogManager()
{
	{
		std::lock_guard<std::mutex> lg(m_QueueMtx);
		m_ThreadRunning = false;
	}
	m_QueueNotifier.notify_one();
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
	std::set<size_t> HashedModules;
	std::hash<std::string> StringHash;

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

			char timestamp[64];
			std::time_t now_c = std::chrono::system_clock::to_time_t(msg->timestamp);
			std::strftime(timestamp, sizeof(timestamp) / sizeof(char),
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

			const string &modulename = msg->log_module;
			size_t module_hash = StringHash(modulename);
			if (HashedModules.find(module_hash) == HashedModules.end())
			{
				//create possibly non-existing folders before opening log file
				size_t pos = 0;
				while ((pos = modulename.find('/', pos)) != std::string::npos)
				{
					CreateFolder("logs/" + modulename.substr(0, pos++));
				}

				HashedModules.insert(module_hash);
			}

			//default logging
			std::ofstream logfile("logs/" + modulename + ".log",
				std::ofstream::out | std::ofstream::app);

			logfile <<
				"[" << timestamp << "] " <<
				"[" << loglevel_str << "] " <<
				msg->text;

			if (msg->line != 0)
			{
				logfile << " (" << msg->file << ":" << msg->line << ")";
			}
			logfile << '\n' << std::flush;


			//per-log-level logging
			std::ofstream *loglevel_file = nullptr;
			if (msg->loglevel & LogLevel::WARNING)
				loglevel_file = &m_WarningLog;
			else if (msg->loglevel & LogLevel::ERROR)
				loglevel_file = &m_ErrorLog;

			if (loglevel_file != nullptr)
			{
				(*loglevel_file) <<
					"[" << timestamp << "] " <<
					"[" << modulename << "] " <<
					msg->text;
				if (msg->line != 0)
				{
					(*loglevel_file) << " (" << msg->file << ":" << msg->line << ")";
				}
				(*loglevel_file) << '\n' << std::flush;
			}

			//lock the log message queue again (because while-condition and cv.wait)
			lk.lock();
		}
	} while (m_ThreadRunning);
}

void CLogManager::CreateFolder(std::string foldername)
{
#ifdef WIN32
	std::replace(foldername.begin(), foldername.end(), '/', '\\');
	CreateDirectoryA(foldername.c_str(), NULL);
#else
	std::replace(foldername.begin(), foldername.end(), '\\', '/');
	mkdir(foldername.c_str(), ACCESSPERMS);
#endif
}


void samplog_Init()
{
	CLogManager::Get()->IncreasePluginCounter();
}

void samplog_Exit()
{
	CLogManager::Get()->DecreasePluginCounter();
}

bool samplog_LogMessage(const char *module, LogLevel level, const char *msg,
	int line /*= 0*/, const char *file /*= ""*/, const char *func /*= ""*/)
{
	if (module == nullptr || strlen(module) == 0)
		return false;

	CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		module, level, msg ? msg : "", line, file ? file : "", func ? func : "")));
	return true;
}

bool samplog_LogNativeCall(const char *module,
	AMX * const amx, const char *name, const char *params_format)
{
	if (module == nullptr || strlen(module) == 0)
		return false;

	if (amx == nullptr)
		return false;

	if (name == nullptr || strlen(name) == 0)
		return false;

	if (params_format == nullptr) // params_format == "" is valid (no parameters)
		return false;

	const cell *params = CAmxDebugManager::Get()->GetNativeParamsPtr(amx);
	if (params == nullptr)
		return false;

	size_t format_len = strlen(params_format);

	fmt::MemoryWriter fmt_msg;
	fmt_msg << name << '(';

	for (int i = 0; i != format_len; ++i)
	{
		if (i != 0)
			fmt_msg << ", ";

		cell current_param = params[i + 1];
		switch (params_format[i])
		{
		case 'd': //decimal
		case 'i': //integer
			fmt_msg << static_cast<int>(current_param);
			break;
		case 'f': //float
			fmt_msg << amx_ctof(current_param);
			break;
		case 'h': //hexadecimal
		case 'x': //
			fmt_msg << fmt::hex(current_param);
			break;
		case 'b': //binary
			fmt_msg << fmt::bin(current_param);
			break;
		case 's': //string
			fmt_msg << '"' << amx_GetCppString(amx, current_param) << '"';
			break;
		case '*': //censored output
			fmt_msg << "\"*****\"";
			break;
		case 'r': //reference
		{
			cell *addr_dest = nullptr;
			amx_GetAddr(amx, current_param, &addr_dest);
			fmt_msg << "0x" << fmt::pad(fmt::hexu(reinterpret_cast<unsigned int>(addr_dest)), 8, '0');
		}	break;
		case 'p': //pointer-value
			fmt_msg << "0x" << fmt::pad(fmt::hexu(current_param), 8, '0');
			break;
		default:
			return false; //unrecognized format specifier
		}
	}
	fmt_msg << ')';

	int line = 0;
	const char
		*file = "",
		*func = "";

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, &file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, &func);

	CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		module, LogLevel::DEBUG, fmt_msg.str(), line, file, func)));

	return true;
}
