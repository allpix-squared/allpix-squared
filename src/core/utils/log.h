/**
 * @file
 * @brief Provides a logger and macros for convenient access
 *
 * @copyright Copyright (c) 2016-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_H
#define ALLPIX_LOG_H

#ifdef WIN32
#define __func__ __FUNCTION__
#endif

#include <cstring>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace allpix {
    /**
     * @brief Logging detail level
     */
    enum class LogLevel : std::uint8_t {
        FATAL = 0, ///< Fatal problems that terminate the framework (typically exceptions)
        STATUS,    ///< Only critical progress information
        ERROR,     ///< Critical problems that usually lead to fatal errors
        WARNING,   ///< Possible issue that could lead to unexpected results
        INFO,      ///< General information about processes (should not be called in run function)
        DEBUG,     ///< Detailed information about physics process
        NONE,      ///< Indicates the log level has not been set (cannot be selected by the user)
        TRACE,     ///< Software debugging information about what part is currently running
        PRNG,      ///< Logging level printing every pseudo-random number requested
    };
    /**
     * @brief Format of the logger
     */
    enum class LogFormat : std::uint8_t {
        SHORT = 0, ///< Only include a single character for the log level, the section header and the message
        DEFAULT,   ///< Also include the time and a full logging level description
        LONG       ///< All of the above and also information about the file and line where the message was defined
    };

    /**
     * @brief Logger of the framework to inform the user of process
     *
     * Should almost never be instantiated directly. The \ref LOG macro should be used instead to pass all the information.
     * This leads to a cleaner interface for sending log messages.
     */
    // TODO [DOC] This just be renamed to Log?
    class DefaultLogger {
    public:
        /**
         * @brief Construct a logger
         */
        DefaultLogger();
        /**
         * @brief Write the output to the streams and destruct the logger
         */
        ~DefaultLogger();

        /// @{
        /**
         * @brief Disable copying
         */
        DefaultLogger(const DefaultLogger&) = delete;
        DefaultLogger& operator=(const DefaultLogger&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        DefaultLogger(DefaultLogger&&) noexcept(false) = default;
        DefaultLogger& operator=(DefaultLogger&&) noexcept(false) = default;
        /// @}

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
         * @brief Gives a process stream which updates the same line as long as it is the same
         * @param identifier Name to indicate the line, used to distinguish when to update or write new line
         * @param level Logging level
         * @param file The file name of the file containing the log message
         * @param function The function containing the log message
         * @param line The line number of the log message
         * @return A C++ stream to write to
         */
        std::ostringstream& getProcessStream(std::string identifier,
                                             LogLevel level = LogLevel::INFO,
                                             const std::string& file = "",
                                             const std::string& function = "",
                                             uint32_t line = 0);

        /**
         * @brief Finish the logging ensuring proper termination of all streams
         */
        static void finish();

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
         * @param level Name of the level
         * @return Log level corresponding to the name
         */
        static LogLevel getLevelFromString(const std::string& level);
        /**
         * @brief Convert a LogLevel to a string
         * @param level Log level
         * @return Name corresponding to the log level
         */
        static std::string getStringFromLevel(LogLevel level);

        /**
         * @brief Get the logging format
         * @return Current log format
         */
        static LogFormat getFormat();
        /**
         * @brief Set a new logging format
         * @param format New log log format
         */
        static void setFormat(LogFormat format);

        /**
         * @brief Convert a string to a LogFormat
         * @param format Name of the format
         * @return Log format corresponding to the name
         */
        static LogFormat getFormatFromString(const std::string& format);
        /**
         * @brief Convert a LogFormat to a string
         * @param format Log format
         * @return Name corresponding to the log format
         */
        static std::string getStringFromFormat(LogFormat format);

        /**
         * @brief Add a stream to write the log message
         * @param stream Stream to write to
         */
        static void addStream(std::ostream& stream);
        /**
         * @brief Clear and delete all streams that the logger writes to
         */
        static void clearStreams();
        /**
         * @brief Return all the streams the logger writes to
         * @return List of all the streams used
         */
        static const std::vector<std::ostream*>& getStreams();

        /**
         * @brief Set the section header to use from now on
         * @param section Section header to use
         */
        static void setSection(std::string section);
        /**
         * @brief Get the current section header
         * @return Header used
         */
        static std::string getSection();

        /**
         * @brief Set the current event number from now on
         * @param event_num Event number to use
         */
        static void setEventNum(uint64_t event_num);
        /**
         * @brief Get the current event number
         * @return Event number used
         */
        static uint64_t getEventNum();

    private:
        /**
         * @brief Get the current date as a printable string
         * @return Current date as a string
         */
        static std::string get_current_date();

        /**
         * @brief Return if a stream is likely a terminal screen (supporting colors etc.)
         * @return True if the stream is terminal, false otherwise
         */
        static bool is_terminal(std::ostream& stream);

        // Output stream
        std::ostringstream os;

        // Number of exceptions to prevent abort
        int exception_count_{};
        // Saved value of the length of the header indent
        unsigned int indent_count_{};

        // Internal methods to store static values
        static std::string& get_section();
        static uint64_t& get_event_num();
        static LogLevel& get_reporting_level();
        static LogFormat& get_format();
        static std::vector<std::ostream*>& get_streams();

        // Name of the process to log or empty if a normal log message
        std::string identifier_;
        static std::string last_message_;
        static std::string last_identifier_;

        static std::mutex write_mutex_;
    };

    using Log = DefaultLogger;

#ifndef __FILE_NAME__
/**
 *  @brief Base name of the file without the directory
 */
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/**
 * @brief Execute a block only if the reporting level is high enough
 * @param level The minimum log level
 */
#define IFLOG(level) if(allpix::LogLevel::level <= allpix::Log::getReportingLevel() && !allpix::Log::getStreams().empty())

/**
 * @brief Create a logging stream if the reporting level is high enough
 * @param level The log level of the stream
 */
#define LOG(level)                                                                                                          \
    if(allpix::LogLevel::level <= allpix::Log::getReportingLevel() && !allpix::Log::getStreams().empty())                   \
    allpix::Log().getStream(                                                                                                \
        allpix::LogLevel::level, __FILE_NAME__, std::string(static_cast<const char*>(__func__)), __LINE__)

/**
 * @brief Create a logging stream that overwrites the line if the previous message has the same identifier
 * @param level The log level of the stream
 * @param identifier Identifier for this stream to determine overwrites
 */
#define LOG_PROGRESS(level, identifier)                                                                                     \
    if(allpix::LogLevel::level <= allpix::Log::getReportingLevel() && !allpix::Log::getStreams().empty())                   \
    allpix::Log().getProcessStream(                                                                                         \
        identifier, allpix::LogLevel::level, __FILE_NAME__, std::string(static_cast<const char*>(__func__)), __LINE__)

/**
 * @brief Create a logging stream if the reporting level is high enough and this message has not yet been logged
 * @param level The log level of the stream
 */
#define LOG_ONCE(level) LOG_N(level, 1)

/// @{
/**
 * @brief Macros to generate and retrieve line-specific local variables to hold the logging count of a message
 *
 * Note: the double concat macro is needed to ensure __LINE__ is evaluated, see https://stackoverflow.com/a/19666216/17555746
 */
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define GENERATE_LOG_VAR(Count)                                                                                             \
    static std::atomic<int> CONCAT(local___FUNCTION__, __LINE__) { Count }
#define GET_LOG_VARIABLE() CONCAT(local___FUNCTION__, __LINE__)
/// @}

/**
 * @brief Create a logging stream if the reporting level is high enough and this message has not yet been logged more than
 * max_log_count times.
 * @param level The log level of the stream
 * @param max_log_count Maximum number of times this message is allowed to be logged
 */
#define LOG_N(level, max_log_count)                                                                                         \
    GENERATE_LOG_VAR(max_log_count);                                                                                        \
    if(GET_LOG_VARIABLE() > 0)                                                                                              \
        if(allpix::LogLevel::level <= allpix::Log::getReportingLevel() && !allpix::Log::getStreams().empty())               \
    allpix::Log().getStream(                                                                                                \
        allpix::LogLevel::level, __FILE_NAME__, std::string(static_cast<const char*>(__func__)), __LINE__)                  \
        << ((--GET_LOG_VARIABLE() == 0) ? "[further messages suppressed] " : "")

    /**
     * @brief Suppress a stream from writing any output
     * @param stream The stream to suppress
     */
    // suppress a (logging) stream
    // TODO [doc] rewrite as a lowercase function in a namespace?
    inline void SUPPRESS_STREAM(std::ostream& stream) { stream.setstate(std::ios::failbit); }

/**
 * @brief Suppress a stream from writing output unless logging is below \ref allpix::LogLevel
 * @param  level  The log level of the stream
 * @param  stream Stream to suppress
 */
#define SUPPRESS_STREAM_EXCEPT(level, stream)                                                                               \
    IFLOG(level);                                                                                                           \
    else SUPPRESS_STREAM(stream);

    /**
     * @brief Release an suppressed stream so it can write again
     * @param stream The stream to release
     */
    // TODO [doc] rewrite as a lowercase function in a namespace?
    inline void RELEASE_STREAM(std::ostream& stream) { stream.clear(); }
} // namespace allpix

#endif /* ALLPIX_LOG_H */
