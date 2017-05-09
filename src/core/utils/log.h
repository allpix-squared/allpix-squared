/**
 * @file
 * @brief Provides a logger and macros for convenient access
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
    /**
     * @brief Logging detail level
     */
    enum class LogLevel {
        QUIET = 0, ///< Do not log at all
        FATAL,     ///< Fatal problems that should terminate the framework (typically exceptions)
        ERROR,     ///< Critical problems that usually lead to fatal errors
        WARNING,   ///< Possible issue that could lead to unexpected results
        INFO,      ///< General informational summaries of process
        DEBUG      ///< Detailed information about process and execution
    };
    /**
     * @brief Format of the logger
     */
    enum class LogFormat {
        SHORT = 0, ///< Only include a single character for the log level, the section header and the message
        DEFAULT,   ///< Also include the time and a full logging level description
        DEBUG      ///< All of the above and also information about the file and line where the message was defined
    };

    /**
     * @brief Logger of the framework to inform the user
     *
     * Should almost never be instantiated directly. The \ref LOG macro should be used instead to pass all the information.
     * This leads to a cleaner interface for sending log messages.
     */
    // TODO [DOC] This just be renamed to Log?
    class DefaultLogger {
    public:
        DefaultLogger(const DefaultLogger&) = delete;
        DefaultLogger& operator=(const DefaultLogger&) = delete;

        /**
         * @brief Gives a stream to write to using the C++ stream syntax
         * @param level Logging level
         * @param file The file name of the file containing the log message
         * @param function The function containing the log message
         * @param line The line number of the log message
         * @return A C++ stream to write to
         */
        std::ostringstream& getStream(LogLevel level = LogLevel::INFO,
                                      const std::string& file = "",
                                      const std::string& function = "",
                                      uint32_t line = 0);

        /**
         * @brief Get the reporting level for logging
         * @return The current log level
         */
        static LogLevel getReportingLevel();
        /**
         * @brief Set a new reporting level to use for logging
         * @param level The new log level
         */
        static void setReportingLevel(LogLevel level);

        /**
         * @brief Convert a string to a LogLevel
         * @param level The name of the level
         * @return The corresponding log level
         */
        static LogLevel getLevelFromString(const std::string& level);
        /**
         * @brief Convert a LogLevel to a string
         * @param level The log level
         * @return The corresponding name
         */
        static std::string getStringFromLevel(LogLevel level);

        /**
         * @brief Get the logging format
         * @return The current log format
         */
        static LogFormat getFormat();
        /**
         * @brief Set a new logging format
         * @param format The new log log format
         */
        static void setFormat(LogFormat format);

        /**
         * @brief Convert a string to a LogFormat
         * @param format The name of the format
         * @return The corresponding log format
         */
        static LogFormat getFormatFromString(const std::string& format);
        /**
         * @brief Convert a LogFormat to a string
         * @param format The log format
         * @return The corresponding name
         */
        static std::string getStringFromFormat(LogFormat format);

        /**
         * @brief Add a stream to write the log message
         * @param stream A stream to write to
         */
        // NOTE: cannot remove a stream yet
        // WARNING: caller has to make sure that ostream exist while the logger is available
        static void addStream(std::ostream& stream);
        /**
         * @brief Clear and delete all streams that the logger writes to
         */
        static void clearStreams();
        /**
         * @brief Return all the streams the logger writes to
         * @return A list of all the streams used
         */
        static const std::vector<std::ostream*>& getStreams();

        /**
         * @brief Set the section header to use from now on
         * @param header The header to use
         */
        static void setSection(std::string header);
        /**
         * @brief Get the current section header
         * @return The header used
         */
        static std::string getSection();

    private:
        /**
         * @brief The number of exceptions that are uncaught
         * @param cons Return zero if it cannot be properly determined (pre-C++17)
         * @return The number of uncaught exceptions
         */
        int get_uncaught_exceptions(bool cons);
        /**
         * @brief Get the current date as a printable string
         * @return The current date
         */
        std::string get_current_date();

        // Output stream
        std::ostringstream os;

        // Number of exceptions to prevent abort
        int exception_count_;
        // Saved value of the length of the header indent
        unsigned int indent_count_;

        // Internal methods to store static values
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
