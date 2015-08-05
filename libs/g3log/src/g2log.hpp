/** ==========================================================================
 * 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 * 
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 *
 * Filename:g2log.hpp  Framework for Logging and Design By Contract
 * Created: 2011 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited since it was built on public-domain software and influenced
 * at least in "spirit" from the following sources
 * 1. kjellkod.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:  http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/cpp/
 * 3. Dr.Dobbs, Michael Schulze: http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
 * ********************************************* */


#pragma once

#include <string>
#include <cstdarg>
#include <functional>

#include "g2loglevels.hpp"
//#include "g2logmessagecapture.hpp"
#include "g2logmessage.hpp"

#if !(defined(__PRETTY_FUNCTION__))
#define __PRETTY_FUNCTION__   __FUNCTION__
#endif


/** namespace for LOG() and CHECK() frameworks
 * History lesson:   Why the names 'g2' and 'g2log'?:
 * The framework was made in my own free time as PUBLIC DOMAIN but the 
 * first commercial project to use it used 'g2' as an internal denominator for 
 * the current project. g2 as in 'generation 2'. I decided to keep the g2 and g2log names
 * to give credit to the people in that project (you know who you are :) and I guess also
 * for 'sentimental' reasons. That a big influence was google's glog is just a happy 
 *  concidence or subconscious choice. Either way g2log became the name for this logger.
 *
 * --- Thanks for a great 2011 and good luck with 'g2' --- KjellKod 
 */
namespace g2 {
   class LogWorker;
   struct LogMessage;
   struct FatalMessage;

   /** Should be called at very first startup of the software with \ref g2LogWorker
    *  pointer. Ownership of the \ref g2LogWorker is the responsibilkity of the caller */
   //void initializeLogging(LogWorker *logger);


   // internal namespace is for completely internal or semi-hidden from the g2 namespace due to that it is unlikely
   // that you will use these
   namespace internal {

      // Save the created LogMessage to any existing sinks
      //void saveMessage(LogWorker *log_worker, const char* message, const char* file, int line, const char* function, const LOGLEVEL& level,
      //        const char* boolean_expression, int fatal_signal, const char* stack_trace);

      // forwards the message to all sinks
      //void pushMessageToLogger(LogMessagePtr log_entry);

      
      // forwards a FATAL message to all sinks,. after which the g2logworker
      // will trigger crashhandler / g2::internal::exitWithDefaultSignalHandler
      // 
      // By default the "fatalCall" will forward a Fatalessageptr to this function
      // this behaviour can be changed if you set a different fatal handler through
      // "setFatalExitHandler"
     // void pushFatalMessageToLogger(FatalMessagePtr message);


      // Save the created FatalMessage to any existing sinks and exit with 
      // the originating fatal signal,. or SIGABRT if it originated from a broken contract
      // By default forwards to: pushFatalMessageToLogger, see "setFatalExitHandler" to override
      //
      // If you override it then you probably want to call "pushFatalMessageToLogger" after your
      // custom fatal handler is done. This will make sure that the fatal message the pushed
      // to sinks as well as shutting down the process
      void fatalCall(g2::LogWorker *logworker, FatalMessagePtr message);

   } // internal
} // g2
