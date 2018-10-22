#include <chrono>

#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h> // memset
#include <unistd.h> // read
#include <libgen.h> // dirname, basename

#include "FileChangeDetector.hpp"
#include "LogManager.hpp"

#include "fmt/format.h"


void FileChangeDetector::EventLoop(std::string const file_path)
{
	// can't use realpath, as the file has to exist
	//if (realpath(file_path.c_str(), absolute_path) == nullptr)
	//	return;

	// dirname and basename can modify the string passed,
	// so we make a copy for the second call
	std::string 
		absolute_path(file_path),
		absolute_path_copy(absolute_path);
	const char
		*dir_path = dirname(&absolute_path[0]),
		*file_name = basename(&absolute_path_copy[0]);

	int notifier = inotify_init1(IN_NONBLOCK);
	if (notifier < 0)
	{
		LogManager::Get()->LogInternal(samplog::LogLevel::ERROR, fmt::format(
			"file change detector: can't initialize inotify instance: {:d}",
			errno));
		return;
	}

	int change_notify = inotify_add_watch(notifier, dir_path,
		IN_CREATE | IN_MODIFY | IN_MOVED_TO);
	if (change_notify < 0)
	{
		LogManager::Get()->LogInternal(samplog::LogLevel::ERROR, fmt::format(
			"file change detector: can't add watch for directory \"{:s}\" " \
			"to inotify instance: {:d}",
			dir_path, errno));
		return;
	}

	fd_set notify_set;

	static const int EVENT_SIZE = sizeof(struct inotify_event);
	char buf[1024 * (EVENT_SIZE + NAME_MAX + 1)];
	auto last_execution_tp = std::chrono::steady_clock::now();
	while (_isThreadRunning)
	{
		int status = -1;
		bool exit_thread = false;
		do
		{
			// set gets modified after each select call
			FD_ZERO(&notify_set);
			FD_SET(notifier, &notify_set);

			// timeout gets modified after each select call
			struct timeval timeout_time;
			memset(&timeout_time, 0, sizeof(struct timeval));
			timeout_time.tv_sec = 1;

			status = select(notifier + 1, &notify_set, nullptr, nullptr, &timeout_time);
			if (!_isThreadRunning)
			{
				exit_thread = true;
				break;
			}
		} while (status == 0);

		if (exit_thread)
			break;

		if (status < 0)
		{
			LogManager::Get()->LogInternal(samplog::LogLevel::ERROR, fmt::format(
				"file change detector: can't select inotify instance: {:d}",
				errno));
			return;
		}

		int length = read(notifier, buf, sizeof(buf));
		if (length < 0)
		{
			LogManager::Get()->LogInternal(samplog::LogLevel::ERROR, fmt::format(
				"file change detector: can't read inotify instance: {:d}",
				errno));
			return;
		}

		exit_thread = false;
		for (int i = 0; i != length; )
		{
			struct inotify_event *event = (struct inotify_event *) &buf[i];
			if (event->wd < 0 || event->mask & IN_Q_OVERFLOW)
			{
				LogManager::Get()->LogInternal(samplog::LogLevel::WARNING, fmt::format(
					"file change detector: event queue overflowed"));
				continue;
			}

			if (event->wd != change_notify)
			{
				LogManager::Get()->LogInternal(samplog::LogLevel::WARNING, fmt::format(
					"file change detector: unknown watcher in event"));
				continue;
			}

			if (event->mask & IN_IGNORED || event->mask & IN_UNMOUNT)
			{
				LogManager::Get()->LogInternal(samplog::LogLevel::WARNING, fmt::format(
					"file change detector: inotify watch was removed by the system"));
				exit_thread = true;
				break;
			}

			if (event->mask & IN_ISDIR)
				continue; // we don't care about directory events

			if (event->name && event->len > 0 && strcmp(file_name, event->name) == 0)
			{
				// don't spam with change events
				auto current_tp = std::chrono::steady_clock::now();
				if (current_tp - last_execution_tp > std::chrono::milliseconds(1000))
				{
					_callback();
					last_execution_tp = current_tp;
				}
			}

			i += EVENT_SIZE + event->len;
		}

		if (exit_thread)
			break;
	}

	inotify_rm_watch(notifier, change_notify);
	close(notifier);
}
