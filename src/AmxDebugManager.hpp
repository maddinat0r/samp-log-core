#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Singleton.hpp"
#include "amx/amx.h"
#include "amx/amxdbg.h"
#include <samplog/export.h>
#include <samplog/DebugInfo.hpp>


class AmxDebugManager : public Singleton<AmxDebugManager>
{
	friend class Singleton<AmxDebugManager>;
private:
	AmxDebugManager();
	~AmxDebugManager();

private:
	bool InitDebugData(const char *filepath);
	void InitDebugDataDir(const char *directory);

public:
	void RegisterAmx(AMX *amx);
	void EraseAmx(AMX *amx);

	bool GetFunctionCall(AMX * const amx, ucell address, samplog::AmxFuncCallInfo &dest);
	bool GetFunctionCallTrace(AMX * const amx, std::vector<samplog::AmxFuncCallInfo> &dest);

private:
	bool _disableDebugInfo = false;
	std::unordered_map<AMX_HEADER *, AMX_DBG *> _availableDebugInfo;
	std::unordered_map<AMX *, AMX_DBG *> _amxDebugMap;
};
