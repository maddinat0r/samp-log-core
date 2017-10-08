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
#include <samplog/LogLevel.hpp>
#include "CMessage.hpp"
#include "CAmxDebugManager.hpp"


class LogManager : public CSingleton<LogManager>
{
	friend class CSingleton<LogManager>;
private:
	LogManager();
	~LogManager();
	LogManager(const LogManager &rhs) = delete;
	LogManager(const LogManager &&rhs) = delete;

public:
	inline void IncreasePluginCounter()
	{
		++m_PluginCounter;
	}
	inline void DecreasePluginCounter()
	{
		if (--m_PluginCounter == 0) //last plugin
			CSingleton::Destroy();
	}
	void QueueLogMessage(Message_t &&msg);

private:
	void Process();
	void CreateFolder(std::string foldername);

private:
	std::ofstream
		m_WarningLog,
		m_ErrorLog;

	std::atomic<bool> m_ThreadRunning;
	std::thread *m_Thread = nullptr;

	std::mutex m_QueueMtx;
	std::condition_variable m_QueueNotifier;
	std::queue<Message_t> m_LogMsgQueue;

	std::string m_DateTimeFormat;

	std::atomic<int> m_PluginCounter{ 0 };
};
