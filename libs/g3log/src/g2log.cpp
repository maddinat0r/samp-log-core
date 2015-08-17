/** ==========================================================================
 * 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 *
 * Filename:g2log.cpp  Framework for Logging and Design By Contract
 * Created: 2011 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited since it was built on public-domain software and at least in "spirit" influenced
 * from the following sources
 * 1. kjellkod.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:  http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/cpp/
 * 3. Dr.Dobbs, Michael Schulze: http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
 * ********************************************* */

#include "g2log.hpp"
#include <cstdio>    // vsnprintf
#include <mutex>
#include <csignal>
#include <memory>
#include <iostream>
#include <thread>
#include <atomic>
#include <assert.h>

#include "std2_make_unique.hpp"
#include "g2logworker.hpp"
#include "crashhandler.hpp"
#include "g2logmessage.hpp"

namespace {
std::once_flag g_initialize_flag;
std::mutex g_logging_init_mutex;

std::unique_ptr<g2::LogMessage> g_first_unintialized_msg = {nullptr};
std::once_flag g_set_first_uninitialized_flag;
std::once_flag g_save_first_unintialized_flag;
const std::function<void(void)> g_pre_fatal_hook_that_does_nothing = []{ /*does nothing */};
std::function<void(void)> g_fatal_pre_logging_hook;


std::atomic<size_t> g_fatal_hook_recursive_counter = {0};
}





namespace g2 {
// signalhandler and internal clock is only needed to install once
// for unit testing purposes the initializeLogging might be called
// several times...
//                    for all other practical use, it shouldn't!

void initializeLogging(/*LogWorker* bgworker*/) {
   std::call_once(g_initialize_flag, [] {
      installCrashHandler();
   });
   std::lock_guard<std::mutex> lock(g_logging_init_mutex);
   //CHECK(bgworker != nullptr);

   // Save the first uninitialized message, if any
   /*
   std::call_once(g_save_first_unintialized_flag, [&bgworker] {
      if (g_first_unintialized_msg) {
         bgworker->save(LogMessagePtr {std::move(g_first_unintialized_msg)});
      }
   });
   */
   // by default the pre fatal logging hook does nothing
   // if it WOULD do something it would happen in 
  // setFatalPreLoggingHook(g_pre_fatal_hook_that_does_nothing); 
   // recurvise crash counter re-set to zero
   g_fatal_hook_recursive_counter.store(0);
}


/**
*  default does nothing, @ref ::g_pre_fatal_hook_that_does_nothing
*  It will be called just before sending the fatal message, @ref pushFatalmessageToLogger
*  It will be reset to do nothing in ::initializeLogging(...)
*     so please call this function, if you ever need to, after initializeLogging(...)
*/
void setFatalPreLoggingHook(std::function<void(void)>  pre_fatal_hook) {
   static std::mutex m;
   std::lock_guard<std::mutex> lock(m);
   g_fatal_pre_logging_hook = pre_fatal_hook;
}






namespace internal {

/** The default, initial, handling to send a 'fatal' event to g2logworker
 *  the caller will stay here, eternally, until the software is aborted
 * ... in the case of unit testing it is the given "Mock" fatalCall that will
 * define the behaviour.
 */
void fatalCall(g2::LogWorker *logworker, FatalMessagePtr message) {
	assert(logworker != nullptr);
	logworker->fatal(message);
	while (blockForFatalHandling()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}


} // internal
} // g2
