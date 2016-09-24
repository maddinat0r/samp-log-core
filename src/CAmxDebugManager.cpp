#include "CAmxDebugManager.hpp"
#include "CSampConfigReader.hpp"

#include <cassert>
#include <tinydir/tinydir.h>


CAmxDebugManager::CAmxDebugManager()
{
	string use_debuginfo;
	if (CSampConfigReader::Get()->GetVar("logcore_debuginfo", use_debuginfo)
		&& use_debuginfo.empty() == false && use_debuginfo.at(0) == '0')
	{
		// server.cfg var "logcore_debuginfo" is set to '0', 
		// disable whole debug info functionality
		m_DisableDebugInfo = true;
		return;
	}

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

CAmxDebugManager::~CAmxDebugManager()
{
	for (auto &a : m_AvailableDebugInfo)
	{
		delete a.first;
		delete a.second;
	}
}

void CAmxDebugManager::InitDebugDataDir(string directory)
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

bool CAmxDebugManager::InitDebugData(string filepath)
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

void CAmxDebugManager::RegisterAmx(AMX *amx)
{
	if (m_DisableDebugInfo)
		return;

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

void CAmxDebugManager::EraseAmx(AMX *amx)
{
	if (m_DisableDebugInfo)
		return;

	m_AmxDebugMap.erase(amx);
}


bool CAmxDebugManager::GetLastAmxLine(AMX * const amx, int &line)
{
	if (m_DisableDebugInfo)
		return false;

	auto it = m_AmxDebugMap.find(amx);
	if (it != m_AmxDebugMap.end())
		return dbg_LookupLine(it->second, amx->cip, &line) == AMX_ERR_NONE && line++;

	return false;
}

bool CAmxDebugManager::GetLastAmxFile(AMX * const amx, const char **file)
{
	if (m_DisableDebugInfo)
		return false;

	auto it = m_AmxDebugMap.find(amx);
	if (it == m_AmxDebugMap.end())
		return false;

	return dbg_LookupFile(it->second, amx->cip, file) == AMX_ERR_NONE;
}

bool CAmxDebugManager::GetLastAmxFunction(AMX * const amx, const char **function)
{
	if (m_DisableDebugInfo)
		return false;

	auto it = m_AmxDebugMap.find(amx);
	if (it == m_AmxDebugMap.end())
		return false;

	return dbg_LookupFunction(it->second, amx->cip, function) == AMX_ERR_NONE;
}

const cell *CAmxDebugManager::GetNativeParamsPtr(AMX * const amx)
{
	unsigned char *amx_data = amx->data;
	if (amx_data == NULL)
		amx_data = amx->base + reinterpret_cast<AMX_HEADER *>(amx->base)->dat;

	cell arg_offset = reinterpret_cast<cell>(amx_data)+amx->stk;
	return reinterpret_cast<cell *>(arg_offset);
}


void samplog_RegisterAmx(AMX *amx)
{
	CAmxDebugManager::Get()->RegisterAmx(amx);
}

void samplog_EraseAmx(AMX *amx)
{
	CAmxDebugManager::Get()->EraseAmx(amx);
}

bool samplog_GetLastAmxLine(AMX * const amx, int *line)
{
	return CAmxDebugManager::Get()->GetLastAmxLine(amx, *line);
}

bool samplog_GetLastAmxFile(AMX * const amx, const char **file)
{
	return CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
}

bool samplog_GetLastAmxFunction(AMX * const amx, const char **function)
{
	return CAmxDebugManager::Get()->GetLastAmxFunction(amx, function);
}
