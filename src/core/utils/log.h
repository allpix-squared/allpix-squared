/**
 * AllPix logger class
 * 
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_LOG_H
#define ALLPIX_LOG_H

#ifdef WIN32
#define __func__ __FUNCTION__
#endif // WIN32

#include <vector>
#include <string>
#include <ostream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstdio>

namespace allpix {
    enum class LogLevel {
        QUIET = 0,
        CRITICAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
    };
    
    class DefaultLogger {
    public:
        // constructors
        DefaultLogger();
        ~DefaultLogger();
        
        // disallow copy
        DefaultLogger(const DefaultLogger &) = delete;
        DefaultLogger &operator=(const DefaultLogger &) = delete;
        
        // get a stream for logging
        std::ostringstream& getStream(LogLevel level = LogLevel::INFO, std::string file = "", std::string function = "", uint32_t line = 0);
        
        // set reporting options
        static LogLevel getReportingLevel();
        static void setReportingLevel(LogLevel);

        // set streams (std::cerr is default)
        // NOTE: cannot remove a stream yet 
        // WARNING: caller has to make sure that ostream exist while the logger is available
        static void addStream(std::ostream&);
        static void clearStreams();
        static const std::vector<std::ostream*>& getStreams();
        
        // convert log level to string
        static LogLevel getLevelFromString(const std::string &level);
        static std::string getStringFromLevel(LogLevel level);
    protected:
        std::ostringstream os;
    private:
        int exception_count_;
        
        int get_uncaught_exceptions(bool);
        std::string get_current_date();
        static LogLevel &get_reporting_level();
        static std::vector<std::ostream*> &get_streams();
    };
    
    using Log = DefaultLogger;
            
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    
#define IFLOG(level) \
    if (LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty()) ; \
    else 
            
#define LOG(level) \
    if (LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty()) ; \
    else allpix::Log().getStream(LogLevel::level,__FILE_NAME__,__func__,__LINE__)
                    
} //namespace allpix

#endif /* ALLPIX_LOG_H */
