[![Build Status](https://travis-ci.org/maddinat0r/samp-log-core.svg?branch=master)](https://travis-ci.org/maddinat0r/samp-log-core)
What is this?  
-----
This is a library which provides a full logging-system to be used in any SA-MP plugins. It can also use debug information in an AMX file to provide more information about a native-call (like in which PAWN file and on which line number it was called).

I'm just a normal SA-MP scripter; what do I have to do with this?
-----
You just have to download the runtime package containing the library and put it in the root folder of your SA-MP server (where the SA-MP server executable is).

I'm a plugin developer; what do I need to use this library?
-----
You have to download the normal package, use the include files in your project and link the library with your plugin.
Here is a small example on how you could use this library in your plugin:
```cpp
#include <samplog/PluginLogger.h>

using samplog::LogLevel;
samplog::CPluginLogger logger("my-plugin-name");

// native MyNativeFunction(int_param, Float:float_param, const string_param[]);
cell AMX_NATIVE_CALL MyNativeFunction(AMX *amx, cell *params)
{
	logger.Log(LogLevel::WARNING, "simple warning message");

	logger.Log(amx, LogLevel::ERROR, "error message with debug info (line number, file) appended");

	//logs the native call with the actual passed values as debug message
	logger.LogNativeCall(amx, "MyNativeFunction", "dfs");
	//possible log message: 
	//    "[<datetime>] [DEBUG] MyNativeFunction(123, 45.6789, "mystring") (my-script.pwn:43)"

	return 1;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
    samplog::Init();
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	samplog::Exit();
}
```
----
###used server configuration variables
- `logtimeformat` (using the same variable as the SA-MP server): uses the specified formatting for the date/time string of a log message  
- `logplugin_debuginfo`: when set to `0`, disables all additional debug info functionality, even if a AMX file is compiled with debug informations (basically renders all functions in header `DebugInfo.hpp` useless, they always return `false`)  

###Thanks to:
- [Zeex' crashdetect](https://github.com/Zeex/samp-plugin-crashdetect) (many useful things about AMX structure and debug info there!)
- [KjellKod's crash-handler code (taken from g3log)](https://github.com/KjellKod/g3log) (heavily modified by now)
- [vitaut's fmt](https://github.com/fmtlib/fmt)
