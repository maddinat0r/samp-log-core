#include "AmxDebugManager.hpp"
#include "SampConfigReader.hpp"
#include "LogConfig.hpp"

#include <cassert>
#include <tinydir.h>
#include <algorithm>
#include <vector>

using samplog::AmxFuncCallInfo;


AmxDebugManager::AmxDebugManager()
{
	if (LogConfig::Get()->GetGlobalConfig().DisableDebugInfo)
	{
		// disable whole debug info functionality
		_disableDebugInfo = true;
		return;
	}

	std::vector<std::string> gamemodes;
	if (!SampConfigReader::Get()->GetGamemodeList(gamemodes))
		return;

	for (auto &g : gamemodes)
	{
		std::string amx_filepath = "gamemodes/" + g + ".amx";
		InitDebugData(amx_filepath.c_str());
	}

	//load ALL filterscripts (there's no other way since filterscripts can be dynamically (un)loaded
	InitDebugDataDir("filterscripts");
}

AmxDebugManager::~AmxDebugManager()
{
	for (auto &a : _availableDebugInfo)
	{
		delete a.first;
		delete a.second;
	}
}

void AmxDebugManager::InitDebugDataDir(const char *directory)
{
	tinydir_dir dir;
	tinydir_open(&dir, directory);

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

bool AmxDebugManager::InitDebugData(const char *filepath)
{
	FILE* amx_file = fopen(filepath, "rb");
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
		_availableDebugInfo.emplace(new AMX_HEADER(hdr), new AMX_DBG(amxdbg));

	return (error == AMX_ERR_NONE);
}

void AmxDebugManager::RegisterAmx(AMX *amx)
{
	if (_disableDebugInfo)
		return;

	if (_amxDebugMap.find(amx) != _amxDebugMap.end()) //amx already registered
		return;

	for (auto &d : _availableDebugInfo)
	{
		if (memcmp(d.first, amx->base, sizeof(AMX_HEADER)) == 0)
		{
			_amxDebugMap.emplace(amx, d.second);
			break;
		}
	}
}

void AmxDebugManager::EraseAmx(AMX *amx)
{
	if (_disableDebugInfo)
		return;

	_amxDebugMap.erase(amx);
}

bool AmxDebugManager::GetFunctionCall(AMX * const amx, ucell address, AmxFuncCallInfo &dest)
{
	if (_disableDebugInfo)
		return false;

	auto it = _amxDebugMap.find(amx);
	if (it == _amxDebugMap.end())
		return false;

	AMX_DBG *amx_dbg = it->second;

	{
		// workaround for possible overflow of amx_dbg->hdr->lines
		// taken from Zeex' crashdetect plugin code
		int num_lines = (
			reinterpret_cast<unsigned char*>(amx_dbg->symboltbl[0]) -
			reinterpret_cast<unsigned char*>(amx_dbg->linetbl)
			) / sizeof(AMX_DBG_LINE);
		int index = 0;
		while (index < num_lines && amx_dbg->linetbl[index].address <= address)
			index++;

		if (index >= num_lines)
			return false; // invalid address

		if (--index < 0)
			return false; // not found

		dest.line = amx_dbg->linetbl[index].line + 1;
	}

	if (dbg_LookupFile(amx_dbg, address, &(dest.file)) != AMX_ERR_NONE)
		return false;

	if (dbg_LookupFunction(amx_dbg, address, &(dest.function)) != AMX_ERR_NONE)
		return false;

	return true;
}

bool AmxDebugManager::GetFunctionCallTrace(AMX * const amx, std::vector<AmxFuncCallInfo> &dest)
{
	if (_disableDebugInfo)
		return false;

	if (_amxDebugMap.find(amx) != _amxDebugMap.end())
		return false;

	AmxFuncCallInfo call_info;

	if (!GetFunctionCall(amx, amx->cip, call_info))
		return false;

	dest.push_back(call_info);

	AMX_HEADER *base = reinterpret_cast<AMX_HEADER *>(amx->base);
	cell dat = reinterpret_cast<cell>(amx->base + base->dat);

	cell frm_addr = amx->frm;

	while (true)
	{
		cell ret_addr = *(reinterpret_cast<cell *>(dat + frm_addr + sizeof(cell)));

		if (ret_addr == 0)
			break;

		if (GetFunctionCall(amx, ret_addr, call_info))
			dest.push_back(call_info);
		else
			dest.push_back({ 0, "<unknown>", "<unknown>" });

		frm_addr = *(reinterpret_cast<cell *>(dat + frm_addr));
		if (frm_addr == 0)
			break;
	}

	//HACK: for some reason the oldest/highest call (not cip though) 
	//      has a slightly incorrect ret_addr
	if (dest.size() > 1)
		dest.back().line--;

	return true;
}
