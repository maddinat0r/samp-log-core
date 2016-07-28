#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "amx/amx.h"
#include "amx/amxdbg.h"
#include "CSingleton.hpp"
#include "export.h"

using std::string;
using std::unordered_map;


class CAmxDebugManager : public CSingleton<CAmxDebugManager>
{
	friend class CSingleton<CAmxDebugManager>;
private:
	CAmxDebugManager();
	~CAmxDebugManager();

private:
	bool InitDebugData(string filepath);
	void InitDebugDataDir(string directory);

public:
	void RegisterAmx(AMX *amx);
	void EraseAmx(AMX *amx);

	bool GetLastAmxLine(AMX * const amx, int &line);
	bool GetLastAmxFile(AMX * const amx, const char **file);
	bool GetLastAmxFunction(AMX * const amx, const char **function);

	const cell *GetNativeParamsPtr(AMX * const amx);

private:
	bool m_DisableDebugInfo = false;
	unordered_map<AMX_HEADER *, AMX_DBG *> m_AvailableDebugInfo;
	unordered_map<AMX *, AMX_DBG *> m_AmxDebugMap;
};

extern "C" DLL_PUBLIC void samplog_RegisterAmx(AMX *amx);
extern "C" DLL_PUBLIC void samplog_EraseAmx(AMX *amx);

extern "C" DLL_PUBLIC bool samplog_GetLastAmxLine(AMX * const amx, int *line);
extern "C" DLL_PUBLIC bool samplog_GetLastAmxFile(AMX * const amx, const char **file);
extern "C" DLL_PUBLIC bool samplog_GetLastAmxFunction(AMX * const amx, const char **function);
