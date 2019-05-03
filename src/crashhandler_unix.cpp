#include "crashhandler.hpp"

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) && !defined(__GNUC__))
#	error "crashhandler_unix.cpp is used on a non-UNIX platform"
#endif

#include "LogManager.hpp"

#include <fmt/format.h>

#include <csignal>
#include <cstring>
#include <unistd.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>

 // Linux/Clang, OSX/Clang, OSX/gcc
#if (defined(__clang__) || defined(__APPLE__))
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif


using samplog::LogLevel;


namespace 
{
	const std::map<crashhandler::Signal, std::string> Signals = {
	   {SIGABRT, "SIGABRT"},
	   {SIGFPE, "SIGFPE"},
	   {SIGILL, "SIGILL"},
	   {SIGSEGV, "SIGSEGV"},
	   {SIGINT, "SIGINT"},
	};

	std::map<crashhandler::Signal, struct sigaction> OldSignalActions;

	
	bool IsFirstSignal() 
	{
		static std::atomic<int> first_exit{ 0 };
		int const count = first_exit.fetch_add(1, std::memory_order_relaxed);
		return (count == 0);
	}

	void RestoreSignalHandler(int signal_number) 
	{
		//try restoring old action
		auto it = OldSignalActions.find(signal_number);
		if (it != OldSignalActions.end())
			sigaction(signal_number, &(it->second), nullptr);
	}

	void ExitWithDefaultSignalHandler(crashhandler::Signal fatal_signal_id)
	{
		const int signal_number = static_cast<int>(fatal_signal_id);
		RestoreSignalHandler(signal_number);
		std::cerr << "\n\n" << "[log-core] fatal signal '" << signal_number
			<< "' (" << Signals.at(fatal_signal_id) << ") caught   \n\n" << std::flush;

		raise(signal_number);
	}

	void SignalHandler(int signal_number, siginfo_t* info, void* unused_context) 
	{
		//only one signal will be allowed past this point
		if (!IsFirstSignal())
		{
			while (true)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		const std::string err_msg = fmt::format(
			"caught signal {:d} ({:s}) (errno: {}, signal code: {}, exit status: {})",
			signal_number, Signals.at(signal_number), info->si_errno, info->si_code, info->si_status);

		LogManager::Get()->LogInternal(LogLevel::INFO, err_msg);
		LogManager::Get()->LogInternal(LogLevel::INFO, "log-core will now safely shut itself down");
		LogManager::Get()->Destroy();

		ExitWithDefaultSignalHandler(signal_number);
	}
}


namespace crashhandler
{
	void Install() 
	{
		struct sigaction action, old_action;
		memset(&action, 0, sizeof(action));
		memset(&old_action, 0, sizeof(old_action));
		sigemptyset(&action.sa_mask);
		action.sa_sigaction = &SignalHandler;
		action.sa_flags = SA_SIGINFO;

		for (const auto &signal : Signals) 
		{
			if (sigaction(signal.first, &action, &old_action) < 0)
			{
				const std::string error = "sigaction - " + signal.second;
				perror(error.c_str());
			}
			else
			{
				OldSignalActions.emplace(signal.first, old_action);
			}
		}
	}
}

