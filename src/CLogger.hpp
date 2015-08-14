#include "g2log.hpp"
#include "g2loglevels.hpp"
#include "g2sinkhandle.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <fstream>

using std::shared_ptr;

namespace g2 { class LogWorker; }


class CLogSink
{
public:
	CLogSink(std::string filename);
	~CLogSink() = default;

private:
	std::ofstream m_Logfile;

public:
	void OnReceive(g2::LogMessageMover m_msg);
};

class CLogger
{
public:
	CLogger(std::string filename);
	~CLogger() = default;

private:

public:
	void SetLogLevel(const LOGLEVEL &log_level, bool enabled);

	bool LogLevel(const LOGLEVEL &log_level);

	void Log(const char *msg, const LOGLEVEL& level, long line = 0, const char *file = "",
		const char *function = "", g2::SignalType fatal_signal = SIGABRT);
	
private:
	shared_ptr<g2::LogWorker> m_LogWorker;
	std::unique_ptr<g2::SinkHandle<CLogSink>> m_SinkHandle;

	std::atomic<LOGLEVEL> m_LogLevel;
};
