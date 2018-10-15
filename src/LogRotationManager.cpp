#include "LogRotationManager.hpp"

#include <cstdio>
#include <fstream>
#include <vector>
#include <fmt/format.h>
#include <tinydir.h>


void LogRotationManager::Check(std::string const &filepath, LogRotationConfig const &config)
{
	if (config.Type == LogRotationType::NONE)
		return;

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

		auto const filepath_offset = filepath.find_last_of('/');
		std::string const
			file_dir = filepath.substr(0, filepath_offset),
			file_name = filepath.substr(filepath_offset + 1);
		std::vector<int> moved_nums;

		if (config.BackupCount != 0)
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
		}

		std::sort(moved_nums.begin(), moved_nums.end(), std::greater<int>());

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

	}
}
