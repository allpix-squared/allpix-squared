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

// NOTE: we have to check for exceptions before we do the actual logging (which may also throw exceptions)
DefaultLogger::DefaultLogger() : os(), exception_count_(get_uncaught_exceptions(true)), indent_count_(0) {}

DefaultLogger::~DefaultLogger() {
    // check if it is potentially safe to throw
    if(exception_count_ != get_uncaught_exceptions(false)) {
        return;
    }

    // get output string
    std::string out(os.str());

    // replace every newline by indented code if necessary
    auto start_pos = out.find('\n');
    if(start_pos != std::string::npos) {
        std::string spcs(indent_count_ + 1, ' ');
        spcs[0] = '\n';
        do {
            out.replace(start_pos, 1, spcs);
            start_pos += spcs.length();
        } while((start_pos = out.find('\n', start_pos)) != std::string::npos);
    }

    // add final newline
    out += '\n';

    // print output to streams
    for(auto stream : get_streams()) {
        (*stream) << out;
    }
}

std::ostringstream&
DefaultLogger::getStream(LogLevel level, const std::string& file, const std::string& function, uint32_t line) {
    // add date in all except short format
    if(get_format() != LogFormat::SHORT) {
        os << "|" << get_current_date() << "| ";
    }

    // add log level (shortly in the short format)
    if(get_format() != LogFormat::SHORT) {
        std::string level_str = "(";
        level_str += getStringFromLevel(level);
        level_str += ")";
        os << std::setw(9) << level_str << " ";
    } else {
        os << "(" << getStringFromLevel(level).substr(0, 1) << ") ";
    }

    // add section if available
    if(!get_section().empty()) {
        os << "[" << get_section() << "] ";
    }

    // print function name and line number information in debug format
    if(get_format() == LogFormat::DEBUG) {
        os << "<" << file << "/" << function << ":L" << line << "> ";
    }

    // save the indent count to fix with newlines
    indent_count_ = static_cast<unsigned int>(os.str().size());
    return os;
}

// reporting level
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
// convert string to log level and vice versa
std::string DefaultLogger::getStringFromLevel(LogLevel level) {
    static const std::array<std::string, 6> type = {{"QUIET", "FATAL", "ERROR", "WARNING", "INFO", "DEBUG"}};
    return type.at(static_cast<decltype(type)::size_type>(level));
}

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

// log format
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
// convert string to log level and vice versa
std::string DefaultLogger::getStringFromFormat(LogFormat format) {
    static const std::array<std::string, 3> type = {{"SHORT", "DEFAULT", "DEBUG"}};
    return type.at(static_cast<decltype(type)::size_type>(format));
}

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

// change streams
std::vector<std::ostream*>& DefaultLogger::get_streams() {
    static std::vector<std::ostream*> streams = {&std::cout};
    return streams;
}
const std::vector<std::ostream*>& DefaultLogger::getStreams() {
    return get_streams();
}
void DefaultLogger::clearStreams() {
    get_streams().clear();
}
void DefaultLogger::addStream(std::ostream& stream) {
    get_streams().push_back(&stream);
}

// section names
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
