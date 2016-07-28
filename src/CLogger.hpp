#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <map>
#include <condition_variable>
#include <functional>
#include <fstream>

#include "CSingleton.hpp"
#include "loglevel.hpp"
#include "CMessage.hpp"
#include "CAmxDebugManager.hpp"
#include "export.h"


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
	void CreateFolder(std::string foldername);

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


extern "C" DLL_PUBLIC bool samplog_LogMessage(
	const char *module, LogLevel level, const char *msg,
	int line = 0, const char *file = "", const char *func = "");
extern "C" DLL_PUBLIC bool samplog_LogNativeCall(
	const char *module, AMX * const amx,
	const char *name, const char *params_format);
