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
#endif

#include <vector>
#include <string>
#include <ostream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <iomanip>

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
        std::ostringstream& getStream(LogLevel level = LogLevel::INFO, const std::string &file = "", const std::string &function = "", uint32_t line = 0);

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

#define __FILE_NAME__ (strrchr(__FILE__, '/') ? std::string(__FILE__).substr(std::string(__FILE__).find_last_of("/\\") + 1) : std::string(__FILE__))

// execute if the log level is high enough
#define IFLOG(level) \
    if (allpix::LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty()) ; \
    else

// log to streams if level is high enough
#define LOG(level) \
    if (allpix::LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty()) ; \
    else allpix::Log().getStream(allpix::LogLevel::level, __FILE_NAME__, std::string(std::move(static_cast<const char*>(__func__))), __LINE__)

    // FIXME: this also be defined in a separate file
    // suppress a (logging) stream
    inline void SUPPRESS_STREAM(std::ostream &stream) {
        stream.setstate(std::ios::failbit);
    }

    // release a suppressed stream
    inline void RELEASE_STREAM(std::ostream &stream) {
        stream.clear();
    }
}

#endif /* ALLPIX_LOG_H */
