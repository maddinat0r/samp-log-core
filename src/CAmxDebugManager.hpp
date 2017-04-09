#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "amx/amx.h"
#include "amx/amxdbg.h"
#include "CSingleton.hpp"
#include "export.h"

using std::string;
using std::unordered_map;


extern "C" typedef struct
{
	int line;
	const char *file;
	const char *function;
} samplog_AmxFuncCallInfo;

typedef samplog_AmxFuncCallInfo AmxFuncCallInfo;

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

	bool GetFunctionCall(AMX * const amx, ucell address, AmxFuncCallInfo &dest);
	bool GetFunctionCallTrace(AMX * const amx, std::vector<AmxFuncCallInfo> &dest);

private:
	bool m_DisableDebugInfo = false;
	unordered_map<AMX_HEADER *, AMX_DBG *> m_AvailableDebugInfo;
	unordered_map<AMX *, AMX_DBG *> m_AmxDebugMap;
};

extern "C" DLL_PUBLIC void samplog_RegisterAmx(AMX *amx);
extern "C" DLL_PUBLIC void samplog_EraseAmx(AMX *amx);

extern "C" DLL_PUBLIC bool samplog_GetLastAmxFunctionCall(
	AMX * const amx, samplog_AmxFuncCallInfo *destination);
extern "C" DLL_PUBLIC unsigned int samplog_GetAmxFunctionCallTrace(
	AMX * const amx, samplog_AmxFuncCallInfo *destination, unsigned int max_size);
