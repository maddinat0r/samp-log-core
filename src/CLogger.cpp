#include <algorithm>
#include <g2logworker.hpp>
#include <std2_make_unique.hpp>

#include "CLogger.hpp"
#include "CSampConfigReader.hpp"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/stat.h>
#endif


CLogSink::CLogSink(std::string filename)
{
	//create possibly non-existing folders before opening log file
	size_t pos = 0;
	while ((pos = filename.find('/', pos)) != std::string::npos)
	{
		auto dir = filename.substr(0, pos++);
#ifdef WIN32
		std::replace(dir.begin(), dir.end(), '/', '\\');
		CreateDirectoryA(dir.c_str(), NULL);
#else
		mkdir(dir.c_str(), ACCESSPERMS);
#endif
	}

	m_Logfile.open(filename);
}

void CLogSink::OnReceive(g2::LogMessageMover m_msg)
{
	g2::LogMessage &msg = m_msg.get();
	m_Logfile <<
		"[" << msg.timestamp() << "] " <<
		"[" << msg.level() << "] " <<
		msg.message();
	if (msg._line != 0)
	{
		m_Logfile << " (" << msg.file() << ":" << msg.line() << ")";
	}
	m_Logfile << '\n';
	m_Logfile.flush();
}


CLogger::CLogger(std::string module)
	: m_ModuleName(module)
{
	g2::installCrashHandler();
	m_LogWorker = g2::LogWorkerManager::Get()->CreateLogWorker();
	/*m_SinkHandle = */m_LogWorker->addSink(std2::make_unique<CLogSink>("logs/" + module + ".log"), &CLogSink::OnReceive);
}
void CLogger::SetLogLevel(const LOGLEVEL &log_level, bool enabled)
{
	LOGLEVEL current_loglevel = m_LogLevel.load(std::memory_order_acquire);
	g2::setLogLevel(current_loglevel, log_level);
	m_LogLevel.store(current_loglevel, std::memory_order_release);
}

bool CLogger::LogLevel(const LOGLEVEL &log_level)
{
	LOGLEVEL current_loglevel = m_LogLevel.load(std::memory_order_acquire);
	return g2::logLevel(current_loglevel, log_level);
}

void CLogger::Log(const char *msg, 
	const LOGLEVEL& level, long line/* = 0*/, const char *file/* = ""*/,
	const char *function/* = ""*/)
{
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	g2::installSignalHandlerForThread();
#endif
	LOGLEVEL msgLevel{ level };
	g2::LogMessagePtr message{ 
		std2::make_unique<g2::LogMessage>(m_ModuleName.c_str(), file, line, function, msgLevel) };
	message.get()->write().append(msg);

	static string datetime_format;
	if (datetime_format.empty())
	{
		if (CSampConfigReader::Get()->GetVar("logtimeformat", datetime_format))
		{
			//delete brackets
			size_t pos = 0;
			while ((pos = datetime_format.find_first_of("[]")) != std::string::npos)
				datetime_format.erase(pos, 1);
		}
		else
		{
			datetime_format = g2::internal::datetime_formatted;
		}
	}
	message.get()->set_datetime_format(datetime_format);

	if (g2::wasFatal(level)) {
		g2::FatalMessagePtr fatal_message{ std2::make_unique<g2::FatalMessage>(*(message._move_only.get()), SIGABRT) };
		g2::internal::fatalCall(m_LogWorker.get(), fatal_message);
	}
	else {
		m_LogWorker->save(message);
	}
}
