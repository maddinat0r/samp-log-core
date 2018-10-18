#include "LogRotationManager.hpp"

#include <cstdio>
#include <fstream>
#include <vector>
#include <functional>
#include <fmt/format.h>
#include <fmt/time.h>
#include <tinydir.h>


void LogRotationManager::Check(std::string const &filepath, LogRotationConfig const &config)
{
	if (config.Type == LogRotationType::NONE)
		return;

	auto const filepath_offset = filepath.find_last_of('/');
	std::string const
		file_dir = filepath.substr(0, filepath_offset),
		file_name = filepath.substr(filepath_offset + 1);

	if (config.Type == LogRotationType::SIZE)
	{
		std::streamoff size = -1;
		{
			std::ifstream file(filepath, std::ifstream::in | std::ifstream::ate);
			size = file.tellg();
		}

		if (size == -1)
			return; //TODO error

		if (size < (config.Value.FileSize * 1000))
			return; // file not large enough

		std::vector<int> moved_nums;

		if (config.BackupCount > 0)
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

		auto filename_count = [&filepath](int count)
		{
			return fmt::format("{:s}.{:d}", filepath, count);
		};

		int count = moved_nums.size();
		for (auto const n : moved_nums)
		{
			auto const
				fn_old = filename_count(n),
				fn_new = filename_count(n + 1);

			if (count-- >= config.BackupCount)
			{
				// unnecessary file, delete it
				std::remove(fn_old.c_str());
			}
			else
			{
				std::rename(fn_old.c_str(), fn_new.c_str());
			}
		}

		if (config.BackupCount != 0)
		{
			std::rename(filepath.c_str(), filename_count(1).c_str());
		}
		else
		{
			// clear original file, because the number of backups to keep is zero
			std::ofstream logfile(filepath, std::ofstream::trunc);
		}
	}
	else if (config.Type == LogRotationType::DATE)
	{
		using Clock = std::chrono::system_clock;
		auto day_check_func = [](std::tm &tm)
		{
			// every night at 00:00 o'clock
			return tm.tm_hour == 0 && tm.tm_min == 0;
		};

		auto tt = Clock::to_time_t(Clock::now());
		auto tm = fmt::localtime(tt);
		switch (config.Value.Date)
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

		auto const new_filename = fmt::format("{:s}.{:%Y%m%d-%H%M}", filepath, tm);
		// check if file already exists
		if (std::ifstream(new_filename))
			return;

		std::vector<std::string> logfile_dates;
		if (config.BackupCount > 0)
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
			if (count-- < config.BackupCount)
				break;
			
			// unnecessary file, delete it
			std::remove(fmt::format("{:s}.{:s}", filepath, d).c_str());
		}

		if (config.BackupCount != 0)
		{
			std::rename(filepath.c_str(), new_filename.c_str());
		}
		else
		{
			// clear original file, because the number of backups to keep is zero
			std::ofstream logfile(filepath, std::ofstream::trunc);
		}
	}
}
