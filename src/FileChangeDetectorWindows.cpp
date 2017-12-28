#include "FileChangeDetector.hpp"

#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


void FileChangeDetector::EventLoop(std::string const file_path)
{
	char
		full_path[MAX_PATH + 1],
		directory_path[_MAX_DIR + 1],
		full_directory_path[MAX_PATH + 1];
	char *filename;

	if (GetFullPathName(file_path.c_str(), sizeof(full_path), full_path, &filename) == 0)
		return; // TODO: error msg: can't resolve file path
	_splitpath_s(full_path, full_directory_path, sizeof(full_directory_path),
		directory_path, sizeof(directory_path), nullptr, 0, nullptr, 0);
	strcat_s(full_directory_path, directory_path);

	HANDLE dir_handle = CreateFile(full_directory_path, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL);

	if (dir_handle == INVALID_HANDLE_VALUE || dir_handle == nullptr)
	{
		return; // TODO: error msg: cannot open folder
	}

	OVERLAPPED polling_overlap;
	polling_overlap.OffsetHigh = 0;
	polling_overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	char buf[2048];
	FILE_NOTIFY_INFORMATION *notify_info;
	auto last_execution_tp = std::chrono::steady_clock::now();
	while (true)
	{
		BOOL result = ReadDirectoryChangesW(dir_handle, &buf, sizeof(buf), FALSE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			nullptr, &polling_overlap, nullptr);

		if (result == FALSE)
			return; // TODO: error msg: can't queue read change operation

		bool end_thread = false;
		do
		{
			DWORD wait_res = WaitForSingleObject(polling_overlap.hEvent, 1000);
			if (!_isThreadRunning)
			{
				end_thread = true;
				break;
			}
			if (wait_res == WAIT_TIMEOUT)
				continue;
			else if (wait_res == WAIT_OBJECT_0)
				break;
			else // error
				return; // TODO: error msg: error while waiting on overlapped i/o event
		} while (true);

		if (end_thread)
			break;

		int offset = 0;
		do
		{
			notify_info = (FILE_NOTIFY_INFORMATION*)((char*)buf + offset);
			offset += notify_info->NextEntryOffset;

			char change_filename[MAX_PATH];
			int filenamelen = WideCharToMultiByte(CP_ACP, 0, notify_info->FileName,
				notify_info->FileNameLength / 2, change_filename, sizeof(change_filename),
				NULL, NULL);
			change_filename[notify_info->FileNameLength / 2] = '\0';

			if (strcmp(filename, change_filename) != 0)
				continue;

			switch (notify_info->Action)
			{
				case FILE_ACTION_ADDED:
				case FILE_ACTION_MODIFIED:
				case FILE_ACTION_RENAMED_NEW_NAME:
				{
					// don't spam with change events
					// also FILE_ACTION_MODIFIED seem to come in twice with a delay of several ms
					auto current_tp = std::chrono::steady_clock::now();
					if (current_tp - last_execution_tp  > std::chrono::milliseconds(100))
					{
						_callback();
						last_execution_tp = current_tp;
					}
				}
					break;
				case FILE_ACTION_REMOVED:
				case FILE_ACTION_RENAMED_OLD_NAME:
					// we only care if the file is still valid and has the name that we want
					break;
				default:
					// TODO: warning message: unknown file action
					break;
			}


		} while (notify_info->NextEntryOffset != 0);
	}

	CloseHandle(dir_handle);
}
