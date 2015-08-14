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
	m_SinkHandle = m_LogWorker->addSink(std2::make_unique<CLogSink>("logs/" + filename + ".log"), &CLogSink::OnReceive);
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
	const char *function/* = ""*/, g2::SignalType fatal_signal/* = SIGABRT*/)
{
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	g2::installSignalHandlerForThread();
#endif
	LOGLEVEL msgLevel{ level };
	g2::LogMessagePtr message{ std2::make_unique<g2::LogMessage>(file, line, function, msgLevel) };
	message.get()->write().append(msg);


	if (g2::wasFatal(level)) {
		//auto fatalhook = g_fatal_pre_logging_hook;
		//std::string stack_trace;

		//stack_trace = { "\n*******\tSTACKDUMP *******\n" };
		//stack_trace.append(stacktrace::stackdump());

		// In case the fatal_pre logging actually will cause a crash in its turn
		// let's not do recursive crashing!
		//g2::setFatalPreLoggingHook(g_pre_fatal_hook_that_does_nothing);
		//++g_fatal_hook_recursive_counter; // thread safe counter
		// "begin" race here. If two threads crash with recursive crashes
		// then it's possible that the "other" fatal stack trace will be shown
		// that's OK since it was anyhow the first crash detected
		//static const std::string first_stack_trace = stack_trace;
		//fatalhook();
		//message.get()->write().append(stack_trace);

		/*if (g_fatal_hook_recursive_counter.load() > 1) {
			message.get()->write()
				.append("\n\n\nWARNING\n"
				"A recursive crash detected. It is likely the hook set with 'setFatalPreLoggingHook(...)' is responsible\n\n")
				.append("---First crash stacktrace: ").append(first_stack_trace).append("\n---End of first stacktrace\n");
		}*/
		g2::FatalMessagePtr fatal_message{ std2::make_unique<g2::FatalMessage>(*(message._move_only.get()), fatal_signal) };
		// At destruction, flushes fatal message to g2LogWorker
		// either we will stay here until the background worker has received the fatal
		// message, flushed the crash message to the sinks and exits with the same fatal signal
		//..... OR it's in unit-test mode then we throw a std::runtime_error (and never hit sleep)
		g2::internal::fatalCall(m_LogWorker.get(), fatal_message);
	}
	else {
		m_LogWorker->save(message);
	}
}
