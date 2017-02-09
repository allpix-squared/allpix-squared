// Local includes
#include "logger.hpp"

// Set the default global debug level
LogLevel m_globalLogLevel(Info);

// Make the globally accessible log streams
logger info("info", Info);
logger debug("debug", Debug);
logger warning("warning", Warning);
logger error("error", Error);

// Constructors
logger::logger(){
  m_newLine=true;
}
logger::logger(std::string streamID, LogLevel logLevel){
  m_newLine=true;
  m_logLevel = logLevel;
  m_streamID = streamID;
}

// Destructor
logger::~logger(){}

// Static member declaration
std::string logger::m_logFileName;
std::string logger::m_logPath = ".";
std::ofstream logger::m_logFile;
