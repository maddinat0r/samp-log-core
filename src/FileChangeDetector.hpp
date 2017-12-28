#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>


class FileChangeDetector
{
public:
	FileChangeDetector(std::string const &file_path, std::function<void()> callback) :
		_isThreadRunning(true),
		_eventThread(&FileChangeDetector::EventLoop, this, file_path),
		_callback(callback)
	{ }
	~FileChangeDetector()
	{
		_isThreadRunning = false;
		_eventThread.join();
	}

private:
	std::atomic<bool> _isThreadRunning;
	std::thread _eventThread;
	std::function<void()> _callback;

private:
	void EventLoop(std::string const file_path);

};
