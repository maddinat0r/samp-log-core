#include "CLogger.hpp"

#include <g2logworker.hpp>
#include <std2_make_unique.hpp>


CLogSink::CLogSink(std::string filename)
	: m_Logfile(filename)
{

}

void CLogSink::OnReceive(g2::LogMessageMover m_msg)
{
	g2::LogMessage &msg = m_msg.get();
	m_Logfile << 
		"[" << msg.timestamp() << "] " <<
		"[" << msg.level() << "] " << 
		msg.message() << " " <<
		"(" << msg.file() << ":" << msg.line() << ")\n";
	m_Logfile.flush();
}


CLogger::CLogger(std::string filename)
{
	g2::installCrashHandler();
	m_LogWorker = g2::LogWorkerManager::Get()->CreateLogWorker();
	m_LogWorker->addSink(std2::make_unique<CLogSink>("logs/" + filename + ".log"), &CLogSink::OnReceive);
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

void CLogger::Log(const char *msg, const LOGLEVEL& level, long line/* = 0*/, const char *file/* = ""*/,
	const char *function/* = ""*/)
{
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	g2::installSignalHandlerForThread();
#endif
	LOGLEVEL msgLevel{ level };
	g2::LogMessagePtr message{ std2::make_unique<g2::LogMessage>(file, line, function, msgLevel) };
	message.get()->write().append(msg);


	if (g2::wasFatal(level)) {
		g2::FatalMessagePtr fatal_message{ std2::make_unique<g2::FatalMessage>(*(message._move_only.get()), SIGABRT) };
		g2::internal::fatalCall(m_LogWorker.get(), fatal_message);
	}
	else {
		m_LogWorker->save(message);
	}
}
