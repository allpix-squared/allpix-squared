/**
 * @file
 * @brief Implementation of logger
 *
 * @copyright MIT License
 */

#include "log.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>

using namespace allpix;

/**
 * The logger will save the number of uncaught exceptions during construction to compare that with the number of exceptions
 * during destruction later.
 */
DefaultLogger::DefaultLogger() : exception_count_(get_uncaught_exceptions(true)) {}

/**
 * The output is written to the streams as soon as the logger gets out-of-scope and desctructed. The destructor checks
 * specifically if an exception is thrown while output is written to the stream. In that case the log stream will not be
 * forwarded to the output streams and the message will be discarded.
 */
DefaultLogger::~DefaultLogger() {
    // Check if an exception is thrown while adding output to the stream
    if(exception_count_ != get_uncaught_exceptions(false)) {
        return;
    }

    // TODO [doc] any extra exceptions here need to be catched

    // Get output string
    std::string out(os.str());

    // Replace every newline by indented code if necessary
    auto start_pos = out.find('\n');
    if(start_pos != std::string::npos) {
        std::string spcs(indent_count_ + 1, ' ');
        spcs[0] = '\n';
        do {
            out.replace(start_pos, 1, spcs);
            start_pos += spcs.length();
        } while((start_pos = out.find('\n', start_pos)) != std::string::npos);
    }

    // Add final newline
    out += '\n';

    // Print output to streams
    for(auto stream : get_streams()) {
        (*stream) << out;
    }
}

/**
 * This method is typically automatically called by the \ref LOG macro to return a stream after constructing the logger. The
 * header of the stream is added before returning the output stream.
 */
std::ostringstream&
DefaultLogger::getStream(LogLevel level, const std::string& file, const std::string& function, uint32_t line) {
    // Add date in all except short format
    if(get_format() != LogFormat::SHORT) {
        os << "|" << get_current_date() << "| ";
    }

    // Add log level (shortly in the short format)
    if(get_format() != LogFormat::SHORT) {
        std::string level_str = "(";
        level_str += getStringFromLevel(level);
        level_str += ")";
        os << std::setw(9) << level_str << " ";
    } else {
        os << "(" << getStringFromLevel(level).substr(0, 1) << ") ";
    }

    // Add section if available
    if(!get_section().empty()) {
        os << "[" << get_section() << "] ";
    }

    // Print function name and line number information in debug format
    if(get_format() == LogFormat::DEBUG) {
        os << "<" << file << "/" << function << ":L" << line << "> ";
    }

    // Save the indent count to fix with newlines
    indent_count_ = static_cast<unsigned int>(os.str().size());
    return os;
}

// Getter and setters for the reporting level
LogLevel& DefaultLogger::get_reporting_level() {
    static LogLevel reporting_level = LogLevel::INFO;
    return reporting_level;
}
void DefaultLogger::setReportingLevel(LogLevel level) {
    get_reporting_level() = level;
}
LogLevel DefaultLogger::getReportingLevel() {
    return get_reporting_level();
}

// String to LogLevel conversions and vice versa
std::string DefaultLogger::getStringFromLevel(LogLevel level) {
    static const std::array<std::string, 6> type = {{"QUIET", "FATAL", "ERROR", "WARNING", "INFO", "DEBUG"}};
    return type.at(static_cast<decltype(type)::size_type>(level));
}
/**
 * @throws std::invalid_argument If the string does not correspond with an existing log level
 */
LogLevel DefaultLogger::getLevelFromString(const std::string& level) {
    if(level == "DEBUG") {
        return LogLevel::DEBUG;
    }
    if(level == "INFO") {
        return LogLevel::INFO;
    }
    if(level == "WARNING") {
        return LogLevel::WARNING;
    }
    if(level == "ERROR") {
        return LogLevel::ERROR;
    }
    if(level == "FATAL") {
        return LogLevel::FATAL;
    }
    if(level == "QUIET") {
        return LogLevel::QUIET;
    }

    throw std::invalid_argument("unknown log level");
}

// Getter and setters for the format
LogFormat& DefaultLogger::get_format() {
    static LogFormat reporting_level = LogFormat::DEFAULT;
    return reporting_level;
}
void DefaultLogger::setFormat(LogFormat level) {
    get_format() = level;
}
LogFormat DefaultLogger::getFormat() {
    return get_format();
}

// Convert string to log format and vice versa
std::string DefaultLogger::getStringFromFormat(LogFormat format) {
    static const std::array<std::string, 3> type = {{"SHORT", "DEFAULT", "DEBUG"}};
    return type.at(static_cast<decltype(type)::size_type>(format));
}
/**
 * @throws std::invalid_argument If the string does not correspond with an existing log format
 */
LogFormat DefaultLogger::getFormatFromString(const std::string& format) {
    if(format == "SHORT") {
        return LogFormat::SHORT;
    }
    if(format == "DEFAULT") {
        return LogFormat::DEFAULT;
    }
    if(format == "DEBUG") {
        return LogFormat::DEBUG;
    }

    throw std::invalid_argument("unknown log format");
}

/**
 * The streams are shared by all logger instantiations.
 */
const std::vector<std::ostream*>& DefaultLogger::getStreams() {
    return get_streams();
}
std::vector<std::ostream*>& DefaultLogger::get_streams() {
    static std::vector<std::ostream*> streams = {&std::cout};
    return streams;
}
void DefaultLogger::clearStreams() {
    get_streams().clear();
}
/**
 * The caller has to make sure that the added ostream exists for as long log messages may be written. The std::cout stream is
 * added automatically to the list of streams and does not need to be added itself.
 *
 * @note Streams cannot be individually removed at the moment and only all at once using \ref clearStreams().
 */
void DefaultLogger::addStream(std::ostream& stream) {
    get_streams().push_back(&stream);
}

// Getters and setters for the section header
std::string& DefaultLogger::get_section() {
    static std::string section;
    return section;
}
void DefaultLogger::setSection(std::string section) {
    get_section() = std::move(section);
}
std::string DefaultLogger::getSection() {
    return get_section();
}

/**
 * The date is returned in the hh:mm:ss.ms format
 */
std::string DefaultLogger::get_current_date() {
    // FIXME: revise this to get microseconds in a better way
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%X");

    auto seconds_from_epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - seconds_from_epoch).count();
    ss << ".";
    ss << std::setfill('0') << std::setw(3);
    ss << millis;
    return ss.str();
}

/**
 * The number of uncaught exceptions can only be properly determined in C++17. In earlier versions it is only possible to
 * check if there is at least a single exception thrown and that function is used instead. This means a return value of zero
 * corresponds to no exception and one to at least one exception.
 */
int DefaultLogger::get_uncaught_exceptions(bool cons = false) {
#if __cplusplus > 201402L
    // we can only do this fully correctly in C++17
    return std::uncaught_exceptions();
#else
    if(cons) {
        return 0;
    }
    return static_cast<int>(std::uncaught_exception());
#endif
}
