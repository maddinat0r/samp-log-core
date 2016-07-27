/** ==========================================================================
 * 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================*/

#include "crashhandler.hpp"

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) && !defined(__GNUC__))
#error "crashhandler_unix.cpp used but it's a windows system"
#endif


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
#include <fmt/format.h>

 // Linux/Clang, OSX/Clang, OSX/gcc
#if (defined(__clang__) || defined(__APPLE__))
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#include "CLogger.hpp"


namespace 
{
	static const std::map<g3::SignalType, std::string> gSignals = {
	   {SIGABRT, "SIGABRT"},
	   {SIGFPE, "SIGFPE"},
	   {SIGILL, "SIGILL"},
	   {SIGSEGV, "SIGSEGV"},
	   {SIGTERM, "SIGTERM"},
	};


	bool IsFirstSignal() 
	{
		static std::atomic<int> firstExit{ 0 };
		auto const count = firstExit.fetch_add(1, std::memory_order_relaxed);
		return (count == 0);
	}

	void RestoreSignalHandler(int signal_number) 
	{
		struct sigaction action;
		memset(&action, 0, sizeof(action)); //
		sigemptyset(&action.sa_mask);
		action.sa_handler = SIG_DFL; // take default action for the signal
		sigaction(signal_number, &action, NULL);
	}

	void ExitWithDefaultSignalHandler(crashhandler::Signal fatal_signal_id, pid_t process_id)
	{
		const int signal_number = static_cast<int>(fatal_signal_id);
		RestoreSignalHandler(signal_number);
		std::cerr << "\n\n" << __FUNCTION__ << ":" << __LINE__ << ". Signal ID: " << signal_number << "   \n\n" << std::flush;


		kill(process_id, signal_number);
		exit(signal_number);
	}

	void SignalHandler(int signal_number, siginfo_t* info, void* unused_context) 
	{
		// Only one signal will be allowed past this point
		if (!IsFirstSignal())
		{
			while (true)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		const std::string err_msg = fmt::format(
			"signal {:d} ({:s}) catched; shutting log-core down (errno: {}, signal code: {}, exit status: {})",
			signal_number, gSignals.at(signal_number), info->si_errno, info->si_code, info->si_status);

		CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
			"logs/log-core.log", "log-core", LogLevel::ERROR,
			err_msg, 0, "", "")));
		CLogManager::Get()->Destroy();

		ExitWithDefaultSignalHandler(signal_number, info->si_pid);
	}
}


namespace crashhandler
{
	void Install() 
	{
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		sigemptyset(&action.sa_mask);
		action.sa_sigaction = &SignalHandler;
		action.sa_flags = SA_SIGINFO;

		for (const auto &sig_pair : gSignals) 
		{
			if (sigaction(sig_pair.first, &action, nullptr) < 0) 
			{
				const std::string error = "sigaction - " + sig_pair.second;
				perror(error.c_str());
			}
		}
	}
}

