#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "CSingleton.hpp"
#include "amx/amx.h"
#include "amx/amxdbg.h"
#include <samplog/export.h>
#include <samplog/DebugInfo.hpp>

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

	bool GetFunctionCall(AMX * const amx, ucell address, samplog::AmxFuncCallInfo &dest);
	bool GetFunctionCallTrace(AMX * const amx, std::vector<samplog::AmxFuncCallInfo> &dest);

private:
	bool m_DisableDebugInfo = false;
	unordered_map<AMX_HEADER *, AMX_DBG *> m_AvailableDebugInfo;
	unordered_map<AMX *, AMX_DBG *> m_AmxDebugMap;
};
