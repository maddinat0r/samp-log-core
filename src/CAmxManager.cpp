#include "CAmxManager.hpp"
#include "CSampConfigReader.hpp"

#include <vector>
#include <string>
#include <cassert>

using std::vector;
using std::string;


CAmxManager::CAmxManager()
{
	vector<string> gamemodes;
	if (!CSampConfigReader::Get()->GetGamemodeList(gamemodes))
		return;

	for (auto &g : gamemodes)
	{
		string amx_filepath = "gamemodes/" + g + ".amx";
		FILE* amx_file = fopen(amx_filepath.c_str(), "rb");
		if (amx_file == nullptr)
			continue;
			
		/*
		 The following two lines are stripped from AMX helper function "aux_LoadProgram".
		 There are some additional endianess checks and alignments, but these are only 
		 important if the system is using big endian. We assume that this library always runs on 
		 litte-endian machines, since the SA-MP server only runs on x86 and x86-64 architecture.
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
	}
	
}

CAmxManager::~CAmxManager()
{
	for (auto &a : m_AvailableDebugInfo)
	{
		delete a.first;
		delete a.second;
	}
}

void CAmxManager::RegisterAmx(AMX *amx)
{
	//TODO: filterscripts?! (can be dynamically loaded/unloaded)
	auto it = m_AmxDebugMap.find(amx);
	if (it != m_AmxDebugMap.end()) //amx already registered
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
