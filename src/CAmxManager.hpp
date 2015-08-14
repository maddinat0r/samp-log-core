#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <amx/amx.h>
#include "amxdbg.h"
#include "CSingleton.hpp"

using std::string;
using std::unordered_map;


class CAmxManager : public CSingleton<CAmxManager>
{
	friend class CSingleton<CAmxManager>;
private:
	CAmxManager();
	~CAmxManager();

private:
	bool InitDebugData(string filepath);
	void InitDebugDataDir(string directory);

public:
	void RegisterAmx(AMX *amx);
	void EraseAmx(AMX *amx);

	bool GetLastAmxLine(AMX * const amx, long &line);
	bool GetLastAmxFile(AMX * const amx, string &file);
	bool GetLastAmxFunction(AMX * const amx, string &file);

private:
	unordered_map<AMX_HEADER *, AMX_DBG *> m_AvailableDebugInfo;
	unordered_map<AMX *, AMX_DBG *> m_AmxDebugMap;
};
