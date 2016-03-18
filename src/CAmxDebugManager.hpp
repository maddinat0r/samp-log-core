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

	bool GetLastAmxLine(AMX * const amx, long &line);
	bool GetLastAmxFile(AMX * const amx, const char * &file);
	bool GetLastAmxFunction(AMX * const amx, const char * &function);

	const cell *GetNativeParamsPtr(AMX * const amx);

private:
	unordered_map<AMX_HEADER *, AMX_DBG *> m_AvailableDebugInfo;
	unordered_map<AMX *, AMX_DBG *> m_AmxDebugMap;
};

namespace samplog
{
	extern "C" DLL_PUBLIC void RegisterAmx(AMX *amx);
	extern "C" DLL_PUBLIC void EraseAmx(AMX *amx);
	
	extern "C" DLL_PUBLIC bool GetLastAmxLine(AMX * const amx, long &line);
	extern "C" DLL_PUBLIC bool GetLastAmxFile(AMX * const amx, const char * &file);
	extern "C" DLL_PUBLIC bool GetLastAmxFunction(AMX * const amx, const char * &function);
}
