#include "log.h"

#include <array>

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
    os << "[" << get_current_date() << "] ";
    /*if (logName().size() > 0) {
        os << "<" << logName() << "> ";
    }*/
    os << std::setw(8) << getStringFromLevel(level) << ": ";

    // For debug levels we want also function name and line number printed:
    if(level != LogLevel::INFO && level != LogLevel::QUIET && level != LogLevel::WARNING) {
        os << "<" << file << "/" << function << ":L" << line << "> ";
    }

    indent_count_ = static_cast<unsigned int>(os.str().size());
    return os;
}

// set reporting level
LogLevel& DefaultLogger::get_reporting_level() {
    static LogLevel reporting_level;
    return reporting_level;
}
void DefaultLogger::setReportingLevel(LogLevel level) { get_reporting_level() = level; }

LogLevel DefaultLogger::getReportingLevel() { return get_reporting_level(); }

// change streams
std::vector<std::ostream*>& DefaultLogger::get_streams() {
    static std::vector<std::ostream*> streams = {&std::cerr};
    return streams;
}
const std::vector<std::ostream*>& DefaultLogger::getStreams() { return get_streams(); }
void                              DefaultLogger::clearStreams() { get_streams().clear(); }
void DefaultLogger::addStream(std::ostream& stream) { get_streams().push_back(&stream); }

// convert string to log level and vice versa
std::string DefaultLogger::getStringFromLevel(LogLevel level) {
    static const std::array<std::string, 6> type = {{"QUIET", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG"}};
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
    if(level == "CRITICAL") {
        return LogLevel::CRITICAL;
    }
    if(level == "QUIET") {
        return LogLevel::QUIET;
    }

    DefaultLogger().getStream(LogLevel::WARNING) << "Unknown logging level '" << level
                                                 << "'. Using WARNING level as default.";
    return LogLevel::WARNING;
}

std::string DefaultLogger::get_current_date() {
    // FIXME: revise this to get microseconds in a better way
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%X");

    auto seconds_from_epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - seconds_from_epoch).count();
    ss << "." << millis;
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
