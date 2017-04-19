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

#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace allpix {
    enum class LogLevel { QUIET = 0, FATAL, ERROR, WARNING, INFO, DEBUG };
    enum class LogFormat { SHORT = 0, DEFAULT, DEBUG };

    class DefaultLogger {
    public:
        // constructors
        DefaultLogger();
        ~DefaultLogger();

        // disallow copy
        DefaultLogger(const DefaultLogger&) = delete;
        DefaultLogger& operator=(const DefaultLogger&) = delete;

        // get a stream for logging
        std::ostringstream& getStream(LogLevel level = LogLevel::INFO,
                                      const std::string& file = "",
                                      const std::string& function = "",
                                      uint32_t line = 0);

        // set reporting options
        static LogLevel getReportingLevel();
        static void setReportingLevel(LogLevel);

        // convert log level from/to string
        static LogLevel getLevelFromString(const std::string& level);
        static std::string getStringFromLevel(LogLevel level);

        // set format
        static LogFormat getFormat();
        static void setFormat(LogFormat);

        // convert log format from/to string
        static LogFormat getFormatFromString(const std::string& format);
        static std::string getStringFromFormat(LogFormat format);

        // set streams (std::cerr is default)
        // NOTE: cannot remove a stream yet
        // WARNING: caller has to make sure that ostream exist while the logger is available
        static void addStream(std::ostream&);
        static void clearStreams();
        static const std::vector<std::ostream*>& getStreams();

        // set section names
        static void setSection(std::string);
        static std::string getSection();

    private:
        // output stream
        std::ostringstream os;

        // amount of exceptions to prevent abort
        int exception_count_;
        // saved value of the indented header
        unsigned int indent_count_;

        // internal methods
        int get_uncaught_exceptions(bool);
        std::string get_current_date();

        // static save methods
        static std::string& get_section();
        static LogLevel& get_reporting_level();
        static LogFormat& get_format();
        static std::vector<std::ostream*>& get_streams();
    };

    using Log = DefaultLogger;

#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// execute if the log level is high enough
#define IFLOG(level)                                                                                                        \
    if(allpix::LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty())                     \
        ;                                                                                                                   \
    else

// log to streams if level is high enough
#define LOG(level)                                                                                                          \
    if(allpix::LogLevel::level > allpix::Log::getReportingLevel() || allpix::Log::getStreams().empty())                     \
        ;                                                                                                                   \
    else                                                                                                                    \
        allpix::Log().getStream(                                                                                            \
            allpix::LogLevel::level, __FILE_NAME__, std::string(static_cast<const char*>(__func__)), __LINE__)

    // FIXME: this also be defined in a separate file
    // suppress a (logging) stream
    inline void SUPPRESS_STREAM(std::ostream& stream) { stream.setstate(std::ios::failbit); }

    // release a suppressed stream
    inline void RELEASE_STREAM(std::ostream& stream) { stream.clear(); }
} // namespace allpix

#endif /* ALLPIX_LOG_H */
