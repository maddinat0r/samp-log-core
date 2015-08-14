#include "CAmxManager.hpp"
#include "CSampConfigReader.hpp"

#include <cassert>
#include <tinydir.h>


CAmxManager::CAmxManager()
{
	vector<string> gamemodes;
	if (!CSampConfigReader::Get()->GetGamemodeList(gamemodes))
		return;

	for (auto &g : gamemodes)
	{
		string amx_filepath = "gamemodes/" + g + ".amx";
		InitDebugData(amx_filepath);
	}
	

	//load ALL filterscripts (there's no other way since filterscripts can be dynamically (un)loaded
	InitDebugDataDir("filterscripts");
}

CAmxManager::~CAmxManager()
{
	for (auto &a : m_AvailableDebugInfo)
	{
		delete a.first;
		delete a.second;
	}
}

void CAmxManager::InitDebugDataDir(string directory)
{
	tinydir_dir dir;
	tinydir_open(&dir, directory.c_str());

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		
		if (file.is_dir && file.name[0] != '.')
			InitDebugDataDir(file.path);
		else if (!strcmp(file.extension, "amx"))
			InitDebugData(file.path);

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

bool CAmxManager::InitDebugData(string filepath)
{
	FILE* amx_file = fopen(filepath.c_str(), "rb");
	if (amx_file == nullptr)
		return false;

	/*
	  The following two lines are stripped from AMX helper function "aux_LoadProgram".
	  There are some additional endianess checks and alignments, but these are only
	  important if the system is using big endian. We assume that this library always runs on
	  litte-endian machines, since the SA-MP server only runs on x86(-64) architecture.
	*/
	AMX_HEADER hdr;
	fread(&hdr, sizeof hdr, 1, amx_file);

	/*if (hdr.magic != AMX_MAGIC) {
		fclose(fp);
		return AMX_ERR_FORMAT;
	}*/

	AMX_DBG amxdbg;
	//dbg_LoadInfo already seeks to the beginning of the file
	int error = dbg_LoadInfo(&amxdbg, amx_file);

	fclose(amx_file);

	if (error == AMX_ERR_NONE)
		m_AvailableDebugInfo.emplace(new AMX_HEADER(hdr), new AMX_DBG(amxdbg));

	return (error == AMX_ERR_NONE);
}

void CAmxManager::RegisterAmx(AMX *amx)
{
	if (m_AmxDebugMap.find(amx) != m_AmxDebugMap.end()) //amx already registered
		return;

	for (auto &d : m_AvailableDebugInfo)
	{
		if (memcmp(d.first, amx->base, sizeof(AMX_HEADER)) == 0)
		{
			m_AmxDebugMap.emplace(amx, d.second);
			break;
		}
	}
}

void CAmxManager::EraseAmx(AMX *amx)
{
	m_AmxDebugMap.erase(amx);
}


bool CAmxManager::GetLastAmxLine(AMX * const amx, long &line)
{
	auto it = m_AmxDebugMap.find(amx);
	if (it != m_AmxDebugMap.end())
		return dbg_LookupLine(it->second, amx->cip, &line) == AMX_ERR_NONE;

	return false;
}

bool CAmxManager::GetLastAmxFile(AMX * const amx, string &file)
{
	auto it = m_AmxDebugMap.find(amx);
	if (it != m_AmxDebugMap.end())
	{
		const char *file_buf = nullptr;
		int error = dbg_LookupFile(it->second, amx->cip, &file_buf);
		if (error == AMX_ERR_NONE)
		{
			file.assign(file_buf);
			return true;
		}
	}

	return false;
}

bool CAmxManager::GetLastAmxFunction(AMX * const amx, string &file)
{
	auto it = m_AmxDebugMap.find(amx);
	if (it != m_AmxDebugMap.end())
	{
		const char *file_buf = nullptr;
		int error = dbg_LookupFunction(it->second, amx->cip, &file_buf);
		if (error == AMX_ERR_NONE)
		{
			file.assign(file_buf);
			return true;
		}
	}

	return false;
}
