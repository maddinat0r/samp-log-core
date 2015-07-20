#include <cstdio>

#include "amx\amx.h"
#include "plugincommon.h"
#include "amxdbg.h"

#include "CSampConfigReader.hpp"
#include "CAmxManager.hpp"


typedef void(*logprintf_t)(const char* format, ...);

extern void *pAMXFunctions;
logprintf_t logprintf;


PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];


	logprintf(" >> plugin.log: successfully loaded.");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("plugin.log: Unloading plugin...");


	logprintf("plugin.log: Plugin unloaded.");
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
}


extern "C" const AMX_NATIVE_INFO native_list[] =
{
	{NULL, NULL}
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	CAmxManager::Get()->RegisterAmx(amx);
	return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	CAmxManager::Get()->EraseAmx(amx);
	return AMX_ERR_NONE;
}
