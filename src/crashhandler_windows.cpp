#if !(defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
#error "crashhandler_windows.cpp is used on a non-Windows platform"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fmt/format.h>
#include <map>
#include "crashhandler.hpp"

 // thread_local only exists since VS2015
 //__declspec(thread) does not support non-POD types
 // it's not really a problem though since we only use it on a bool variable
//#if !(defined(thread_local)) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
//#  define thread_local __declspec(thread)
//#endif

namespace {
	LPTOP_LEVEL_EXCEPTION_FILTER g_previous_unexpected_exception_handler = nullptr;

	//thread_local bool g_installed_thread_signal_handler = false;

	void *g_vector_exception_handler = nullptr;

#define __CH_WIN32_STATUS_PAIR(stat) {STATUS_##stat, #stat}

	static const std::map<g3::SignalType, std::string> KnownExceptionsMap{
		__CH_WIN32_STATUS_PAIR(ACCESS_VIOLATION),
		__CH_WIN32_STATUS_PAIR(DATATYPE_MISALIGNMENT),
		__CH_WIN32_STATUS_PAIR(BREAKPOINT),
		__CH_WIN32_STATUS_PAIR(SINGLE_STEP),
		__CH_WIN32_STATUS_PAIR(ARRAY_BOUNDS_EXCEEDED),
		__CH_WIN32_STATUS_PAIR(FLOAT_DENORMAL_OPERAND),
		__CH_WIN32_STATUS_PAIR(FLOAT_DIVIDE_BY_ZERO),
		__CH_WIN32_STATUS_PAIR(FLOAT_INEXACT_RESULT),
		__CH_WIN32_STATUS_PAIR(FLOAT_INVALID_OPERATION),
		__CH_WIN32_STATUS_PAIR(FLOAT_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(FLOAT_STACK_CHECK),
		__CH_WIN32_STATUS_PAIR(FLOAT_UNDERFLOW),
		__CH_WIN32_STATUS_PAIR(INTEGER_DIVIDE_BY_ZERO),
		__CH_WIN32_STATUS_PAIR(INTEGER_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(PRIVILEGED_INSTRUCTION),
		__CH_WIN32_STATUS_PAIR(IN_PAGE_ERROR),
		__CH_WIN32_STATUS_PAIR(ILLEGAL_INSTRUCTION),
		__CH_WIN32_STATUS_PAIR(NONCONTINUABLE_EXCEPTION),
		__CH_WIN32_STATUS_PAIR(STACK_OVERFLOW),
		__CH_WIN32_STATUS_PAIR(INVALID_DISPOSITION)
	};



	// Restore back to default fatal event handling
	void ReverseToOriginalFatalHandling()
	{
		SetUnhandledExceptionFilter(g_previous_unexpected_exception_handler);

		RemoveVectoredExceptionHandler(g_vector_exception_handler);
		/* ---STANDARD SIGNAL STUFF---
		if (SIG_ERR == signal(SIGABRT, SIG_DFL))
			perror("signal - SIGABRT");

		if (SIG_ERR == signal(SIGFPE, SIG_DFL))
			perror("signal - SIGABRT");

		if (SIG_ERR == signal(SIGSEGV, SIG_DFL))
			perror("signal - SIGABRT");

		if (SIG_ERR == signal(SIGILL, SIG_DFL))
			perror("signal - SIGABRT");

		if (SIG_ERR == signal(SIGTERM, SIG_DFL))
			perror("signal - SIGABRT");
			*/
	}



	// called for fatal signals SIGABRT, SIGFPE, SIGSEGV, SIGILL, SIGTERM
	/* ---STANDARD SIGNAL STUFF---
	void signalHandler(int signal_number)
	{
		//TODO: log fatal signal; available info: 
	   //       `signal_number`, `exitReasonName(signal_number)`
		CLogManager::Get()->Destroy();
		exitWithDefaultSignalHandler(false, signal_number);
	}*/



	// general exception handler
	LONG WINAPI exceptionHandling(LPEXCEPTION_POINTERS info, const char *handler)
	{
		//TODO: log fatal signal; available info: `handler`, `info->ExceptionRecord->ExceptionCode;`
		const g3::SignalType fatal_signal = info->ExceptionRecord->ExceptionCode;
		CLogManager::Get()->QueueLogMessage(std::make_unique<CMessage>(
			"logs/log-core.log", "log-core", LogLevel::ERROR, 
			fmt::format("exception {:X} ({:s}) from {:s} catched; shutting log-core down", fatal_signal, KnownExceptionsMap.at(fatal_signal), handler ? handler : "invalid"),
			0, "", ""));
		CLogManager::Get()->Destroy();

		// FATAL Exception: It doesn't necessarily stop here we pass on continue search
		// if no one else will catch that then it's goodbye anyhow.
		// The RISK here is if someone is cathing this and returning "EXCEPTION_EXECUTE_HANDLER"
		// but does not shutdown then the software will be running with g3log shutdown.
		// .... However... this must be seen as a bug from standard handling of fatal exceptions
		// https://msdn.microsoft.com/en-us/library/6wxdsc38.aspx
		return EXCEPTION_CONTINUE_SEARCH;
	}


	// Unhandled exception catching
	LONG WINAPI unexpectedExceptionHandling(LPEXCEPTION_POINTERS info)
	{
		ReverseToOriginalFatalHandling();
		return exceptionHandling(info, "Unhandled Exception Handler");
	}


	/// Setup through (Windows API) AddVectoredExceptionHandler
	/// Ref: http://blogs.msdn.com/b/zhanli/archive/2010/06/25/c-tips-addvectoredexceptionhandler-addvectoredcontinuehandler-and-setunhandledexceptionfilter.aspx
	LONG WINAPI vectorExceptionHandling(PEXCEPTION_POINTERS p)
	{
		const g3::SignalType exc_code = p->ExceptionRecord->ExceptionCode;
		if (KnownExceptionsMap.find(exc_code) == KnownExceptionsMap.end())
			return EXCEPTION_CONTINUE_SEARCH; //not an exception we care for

		ReverseToOriginalFatalHandling();
		return exceptionHandling(p, "Vectored Exception Handler");
	}


} // end anonymous namespace


namespace g3 {
	namespace internal {


		/// string representation of signal ID or Windows exception id
		/*std::string exitReasonName(g3::SignalType fatal_id)
		{
			switch (fatal_id)
			{
			case SIGABRT: return "SIGABRT"; break;
			case SIGFPE: return "SIGFPE"; break;
			case SIGSEGV: return "SIGSEGV"; break;
			case SIGILL: return "SIGILL"; break;
			case SIGTERM: return "SIGTERM"; break;
			default:
				return fmt::format("UNKNOWN SIGNAL({})", fatal_id);
			}
		}*/


		// Triggered by g3log::LogWorker after receiving a FATAL trigger
		// which is LOG(FATAL), CHECK(false) or a fatal signal our signalhandler caught.
		// --- If LOG(FATAL) or CHECK(false) the signal_number will be SIGABRT
		void exitWithDefaultSignalHandler(bool fatal_exception, g3::SignalType fatal_signal_id)
		{
			ReverseToOriginalFatalHandling();
			// For windows exceptions we want to continue the possibility of
			// exception handling now when the log and stacktrace are flushed
			// to sinks. We therefore avoid to kill the process here. Instead
			// it will be the exceptionHandling functions above that
			// will let exception handling continue with: EXCEPTION_CONTINUE_SEARCH
			//if (fatal_exception) {
			//   gBlockForFatal = false;
			//   return;
			//}

			// for a signal however, we exit through that fatal signal
			const int signal_number = static_cast<int>(fatal_signal_id);
			raise(signal_number);
		}




	} // end g3::internal


	///  SIGFPE, SIGILL, and SIGSEGV handling must be installed per thread
	/// on Windows. This is automatically done if you do at least one LOG(...) call
	/// you can also use this function call, per thread so make sure these three
	/// fatal signals are covered in your thread (even if you don't do a LOG(...) call
	/* ---STANDARD SIGNAL STUFF---
	void installSignalHandlerForThread()
	{
		if (!g_installed_thread_signal_handler)
		{
			g_installed_thread_signal_handler = true;
			if (SIG_ERR == signal(SIGTERM, signalHandler))
				perror("signal - SIGTERM");
			if (SIG_ERR == signal(SIGABRT, signalHandler))
				perror("signal - SIGABRT");
			if (SIG_ERR == signal(SIGFPE, signalHandler))
				perror("signal - SIGFPE");
			if (SIG_ERR == signal(SIGSEGV, signalHandler))
				perror("signal - SIGSEGV");
			if (SIG_ERR == signal(SIGILL, signalHandler))
				perror("signal - SIGILL");
		}
	}*/

	void installCrashHandler()
	{
		//g3::installSignalHandlerForThread();
		g_previous_unexpected_exception_handler = SetUnhandledExceptionFilter(unexpectedExceptionHandling);

		// const size_t kFirstExceptionHandler = 1;
		// kFirstExeptionsHandler is kept here for documentational purposes.
		// The last exception seems more what we want
		const size_t kLastExceptionHandler = 0;
		g_vector_exception_handler = AddVectoredExceptionHandler(kLastExceptionHandler, vectorExceptionHandling);
	}
} // end namespace g3
