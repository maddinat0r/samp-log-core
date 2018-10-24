#include <fstream>
#include <vector>
#include <functional>
#include <fmt/format.h>
#include <fmt/time.h>
#include <tinydir.h>

#include "LogRotationManager.hpp"
#include "LogManager.hpp"


LogRotationManager::LogRotationManager() :
	_threadRunning(true),
	_thread(std::bind(&LogRotationManager::Process, this))
{

}

LogRotationManager::~LogRotationManager()
{
	_threadRunning = false;
	_thread.join();
}

void LogRotationManager::Process()
{
	bool skip_date_check = false;
	auto date_tp = Logger::Clock::now();
	while (_threadRunning)
	{
		{
			auto date_tp_now = Logger::Clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::minutes>(date_tp_now - date_tp);
			if (dur > std::chrono::minutes(0))
			{
				date_tp = date_tp_now;
				skip_date_check = false;
			}
			else
			{
				skip_date_check = true;
			}
		}

		{
			std::lock_guard<std::mutex> lock(_rotationEntriesLock);
			for (auto const &e : _rotationEntries)
			{
				auto const &file_path = e.first;
				auto const &config = e.second;

				switch (config.Type)
				{
				case LogRotationType::DATE:
					// skip date check if it already happened within the same minute
					if (skip_date_check)
						continue;

					CheckDateRotation(file_path, config.Value.Date, config.BackupCount);
					break;

				case LogRotationType::SIZE:
					CheckSizeRotation(file_path, config.Value.FileSize, config.BackupCount);
					break;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void SplitFilePath(std::string const &file_path, std::string &file_dir, std::string &file_name)
{
	auto const filepath_offset = file_path.find_last_of('/');
	file_dir = file_path.substr(0, filepath_offset);
	file_name = file_path.substr(filepath_offset + 1);
}

void LogRotationManager::CheckDateRotation(std::string const &file_path,
	LogRotationTimeType const time_type, int const backup_count)
{
	std::string file_dir, file_name;
	SplitFilePath(file_path, file_dir, file_name);

	auto day_check_func = [](std::tm &tm)
	{
		// every night at 00:00 o'clock
		return tm.tm_hour == 0 && tm.tm_min == 0;
	};

	auto tt = Logger::Clock::to_time_t(Logger::Clock::now());
	auto tm = fmt::localtime(tt);
	switch (time_type)
	{
	case LogRotationTimeType::DAILY:
		if (!day_check_func(tm))
			return;
		break;
	case LogRotationTimeType::WEEKLY:
		// every monday night
		if (tm.tm_wday != 1 || !day_check_func(tm))
			return;
		break;
	case LogRotationTimeType::MONTHLY:
		// every first of the month at night
		if (tm.tm_mday != 1 || !day_check_func(tm))
			return;
		break;
	}

	auto const new_filename = fmt::format("{:s}.{:%Y%m%d-%H%M}", file_path, tm);
	// check if file already exists
	if (std::ifstream(new_filename))
		return;

	std::vector<std::string> logfile_dates;
	if (backup_count > 0)
	{
		tinydir_dir dir;
		tinydir_open(&dir, file_dir.c_str());

		while (dir.has_next)
		{
			tinydir_file file;
			tinydir_readfile(&dir, &file);
			tinydir_next(&dir);

			if (!file.is_reg)
				continue;

			std::string const fn(file.name);
			if (fn.find(file_name) != 0)
				continue;

			if (fn == file_name)
				continue;

			logfile_dates.push_back(file.extension);
		}

		tinydir_close(&dir);

		std::sort(logfile_dates.begin(), logfile_dates.end());
	}

	int count = logfile_dates.size();
	for (auto const &d : logfile_dates)
	{
		if (count-- < backup_count)
			break;

		// unnecessary file, delete it
		std::remove(fmt::format("{:s}.{:s}", file_path, d).c_str());
	}

	if (backup_count != 0)
	{
		std::rename(file_path.c_str(), new_filename.c_str());
	}
	else
	{
		// clear original file, because the number of backups to keep is zero
		std::ofstream logfile(file_path, std::ofstream::trunc);
	}
}

void LogRotationManager::CheckSizeRotation(std::string const &file_path,
	unsigned int const max_size, int const backup_count)
{
	std::string file_dir, file_name;
	SplitFilePath(file_path, file_dir, file_name);

	std::streamoff size = -1;
	{
		std::ifstream file(file_path, std::ifstream::in | std::ifstream::ate);
		size = file.tellg();
	}

	if (size == -1)
	{
		LogManager::Get()->LogInternal(samplog::LogLevel::ERROR,
			fmt::format("log rotation: invalid size for file \"{:s}\"", file_path));
		return;
	}

	if (size < (max_size * 1000))
		return; // file not large enough

	std::vector<int> moved_nums;

	if (backup_count > 0)
	{
		tinydir_dir dir;
		tinydir_open(&dir, file_dir.c_str());

		while (dir.has_next)
		{
			tinydir_file file;
			tinydir_readfile(&dir, &file);
			tinydir_next(&dir);

			if (!file.is_reg)
				continue;

			std::string const fn(file.name);
			if (fn.find(file_name) != 0)
				continue;

			char *end;
			int num = strtol(file.extension, &end, 10);
			// end points to the char past the last char interpreted,
			// so '\0' if successfully parsed
			if (*end)
				continue; // extension is not a number

			moved_nums.push_back(num);
		}

		tinydir_close(&dir);

		std::sort(moved_nums.begin(), moved_nums.end(), std::greater<int>());
	}

	auto filename_count = [&file_path](int count)
	{
		return fmt::format("{:s}.{:d}", file_path, count);
	};

	int count = moved_nums.size();
	for (auto const n : moved_nums)
	{
		auto const
			fn_old = filename_count(n),
			fn_new = filename_count(n + 1);

		if (count-- >= backup_count)
		{
			// unnecessary file, delete it
			std::remove(fn_old.c_str());
		}
		else
		{
			std::rename(fn_old.c_str(), fn_new.c_str());
		}
	}

	if (backup_count != 0)
	{
		std::rename(file_path.c_str(), filename_count(1).c_str());
	}
	else
	{
		// clear original file, because the number of backups to keep is zero
		std::ofstream logfile(file_path, std::ofstream::trunc);
	}
}
