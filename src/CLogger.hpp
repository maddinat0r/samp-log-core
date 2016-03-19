#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>

#include "CSingleton.hpp"
#include "loglevel.hpp"
#include "CMessage.hpp"
#include "export.h"


class ILogger
{
public:
	virtual void SetLogLevel(const LogLevel log_level, bool enabled) = 0;
	virtual bool IsLogLevel(const LogLevel log_level) = 0;

	virtual void Log(const char *msg,
		const LogLevel level, int line = 0, const char *file = "",
		const char *function = "") = 0;

	virtual void Destroy() = 0;

};

class CLogger : public ILogger
{
public:
	CLogger(std::string module);
	~CLogger() = default;
	CLogger(const CLogger &rhs) = delete;

public:
	void SetLogLevel(const LogLevel log_level, bool enabled);
	bool IsLogLevel(const LogLevel log_level);

	void Log(const char *msg, 
		const LogLevel level, int line = 0, const char *file = "",
		const char *function = "");
	
	void Destroy();

private:
	const std::string
		m_ModuleName,
		m_FileName;

	std::atomic<LogLevel> m_LogLevel;
};

using Logger_t = std::unique_ptr<CLogger>; 
extern "C" DLL_PUBLIC ILogger *CreateLoggerPtr(const char *modulename);


class CLogManager : public CSingleton<CLogManager>
{
	friend class CSingleton<CLogManager>;
private:
	CLogManager();
	~CLogManager();
	CLogManager(const CLogManager &rhs) = delete;
	CLogManager(const CLogManager &&rhs) = delete;

public:
	void QueueLogMessage(Message_t &&msg);

private:
	void Process();

private:
	std::ofstream
		m_WarningLog,
		m_ErrorLog;

	std::thread *m_Thread = nullptr;
	std::atomic<bool> m_ThreadRunning;

	std::queue<Message_t> m_LogMsgQueue;
	std::mutex m_QueueMtx;
	std::condition_variable m_QueueNotifier;

	std::string m_DateTimeFormat;
};
