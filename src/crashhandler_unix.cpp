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


namespace {

	static const std::map<g3::SignalType, std::string> chSignals = {
	   {SIGABRT, "SIGABRT"},
	   {SIGFPE, "SIGFPE"},
	   {SIGILL, "SIGILL"},
	   {SIGSEGV, "SIGSEGV"},
	   {SIGTERM, "SIGTERM"},
	};

	//std::map<int, std::string> gSignals = kSignals;


	bool shouldDoExit() 
	{
		static std::atomic<uint64_t> firstExit{ 0 };
		auto const count = firstExit.fetch_add(1, std::memory_order_relaxed);
		return (0 == count);
	}

	void restoreSignalHandler(int signal_number) 
	{
		struct sigaction action;
		memset(&action, 0, sizeof(action)); //
		sigemptyset(&action.sa_mask);
		action.sa_handler = SIG_DFL; // take default action for the signal
		sigaction(signal_number, &action, NULL);
	}


	// Dump of stack,. then exit through g3log background worker
	// ALL thanks to this thread at StackOverflow. Pretty much borrowed from:
	// Ref: http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
	void signalHandler(int signal_number, siginfo_t* info, void* unused_context) 
	{
		// Only one signal will be allowed past this point
		if (false == shouldDoExit()) 
		{
			while (true) 
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}

		const std::string err_msg = fmt::format(
			"signal {:d} ({:s}) catched; shutting log-core down (errno: {}, signal code: {}, exit status: {})",
			signal_number, chSignals.at(signal_number), info->si_errno, info->si_code, info->si_status);

		CLogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
			"logs/log-core.log", "log-core", LogLevel::ERROR,
			err_msg, 0, "", "")));
		CLogManager::Get()->Destroy();

		exitWithDefaultSignalHandler(signal_number, info->si_pid);
	}

} // end anonymous namespace






// Redirecting and using signals. In case of fatal signals g3log should log the fatal signal
// and flush the log queue and then "rethrow" the signal to exit
namespace g3 {
	// References:
	// sigaction : change the default action if a specific signal is received
	//             http://linux.die.net/man/2/sigaction
	//             http://publib.boulder.ibm.com/infocenter/aix/v6r1/index.jsp?topic=%2Fcom.ibm.aix.basetechref%2Fdoc%2Fbasetrf2%2Fsigaction.html
	//
	// signal: http://linux.die.net/man/7/signal and
	//         http://msdn.microsoft.com/en-us/library/xdkz3x12%28vs.71%29.asp
	//
	// memset +  sigemptyset: Maybe unnecessary to do both but there seems to be some confusion here
	//          ,plenty of examples when both or either are used
	//          http://stackoverflow.com/questions/6878546/why-doesnt-parent-process-return-to-the-exact-location-after-handling-signal_number
	namespace internal {

		//bool shouldBlockForFatalHandling() {
		//   return true;  // For windows we will after fatal processing change it to false
		//}


		


		/// string representation of signal ID
		/*std::string exitReasonName(g3::SignalType fatal_id) {

		   int signal_number = static_cast<int>(fatal_id);
		   switch (signal_number) {
			  case SIGABRT: return "SIGABRT";
				 break;
			  case SIGFPE: return "SIGFPE";
				 break;
			  case SIGSEGV: return "SIGSEGV";
				 break;
			  case SIGILL: return "SIGILL";
				 break;
			  case SIGTERM: return "SIGTERM";
				 break;
			  default:
				 std::ostringstream oss;
				 oss << "UNKNOWN SIGNAL(" << signal_number << ")";
				 return oss.str();
		   }
		}*/



		// Triggered by g3log->g3LogWorker after receiving a FATAL trigger
		// which is LOG(FATAL), CHECK(false) or a fatal signal our signalhandler caught.
		// --- If LOG(FATAL) or CHECK(false) the signal_number will be SIGABRT
		void exitWithDefaultSignalHandler(g3::SignalType fatal_signal_id, pid_t process_id) 
		{
			const int signal_number = static_cast<int>(fatal_signal_id);
			restoreSignalHandler(signal_number);
			std::cerr << "\n\n" << __FUNCTION__ << ":" << __LINE__ << ". Signal ID: " << signal_number << "   \n\n" << std::flush;


			kill(process_id, signal_number);
			exit(signal_number);
		}
	} // end g3::internal


	// This will override the default signal handler setup and instead
	// install a custom set of signals to handle
	/*void overrideSetupSignals(const std::map<int, std::string> overrideSignals) {
	   static std::mutex signalLock;
	   std::lock_guard<std::mutex> guard(signalLock);
	   for (const auto& sig : gSignals) {
		  restoreSignalHandler(sig.first);
	   }

	   gSignals = overrideSignals;
	   installCrashHandler(); // installs all the signal handling for gSignals
	}*/

	// restores the signal handler back to default
	//void restoreSignalHandlerToDefault() {
	//   overrideSetupSignals(kSignals);
	//}


	// installs the signal handling for whatever signal set that is currently active
	// If you want to setup your own signal handling then
	// You should instead call overrideSetupSignals()
	void installCrashHandler() 
	{
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		sigemptyset(&action.sa_mask);
		action.sa_sigaction = &signalHandler; // callback to crashHandler for fatal signals
											  // sigaction to use sa_sigaction file. ref: http://www.linuxprogrammingblog.com/code-examples/sigaction
		action.sa_flags = SA_SIGINFO;

		// do it verbose style - install all signal actions
		for (const auto &sig_pair : chSignals) {
			if (sigaction(sig_pair.first, &action, nullptr) < 0) {
				const std::string error = "sigaction - " + sig_pair.second;
				perror(error.c_str());
			}
		}
	}
} // end namespace g3

