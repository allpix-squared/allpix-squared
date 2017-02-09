#ifndef LOGGER_H
#define LOGGER_H 1

// Global includes
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <time.h>

//--------------------------------------
// Simple class to perform logging.
//--------------------------------------

// Types of log level
enum LogLevel {
  Debug,
  Info,
  Warning,
  Error,
} ;

// Colors for different log levels
const int logColors[4] = {36, 32, 33, 31}; // cyan (debug), green (info), yellow (notice), red (error)

// Global variable to set which streams are active
// Default is only info and above
extern LogLevel m_globalLogLevel;

// The logging class
class logger{
public:
  // Constructor and desctructor
  logger();
  logger(std::string, LogLevel);
  ~logger();
  
  // Template for information handling (to parse strings, doubles, etc.)
  template<class T>
  logger &operator<<(const T &x)
  {
    // If log level not active do nothing
    if(m_logLevel < m_globalLogLevel) return *this;
    // Cout the message, prefixing with the stream id
    if(m_newLine){
      std::cout<<"\033[1;"<<logColors[m_logLevel]<<"m["<<m_streamID<<"] "<<x<<"\033[0m";
      m_logFile << "[" << m_streamID << "] " << x;
      m_newLine = false;
    }else{
      std::cout<<"\033[1;"<<logColors[m_logLevel]<<"m"<<x<<"\033[0m";
      m_logFile << x;
    }
    return *this;
  }
  
  // Function to handle std::ostream functions
  logger& operator<<(std::ostream& (*f)(std::ostream&)){
    // If log level not active do nothing
    if(m_logLevel < m_globalLogLevel) return *this;
    // Pass the function to cout
    f(std::cout);
    f(m_logFile);// << std::endl;
    m_newLine=true;
    return *this;
  }
  
  // Function to create a new output log file
  void createLogFile(std::string filename = "log"){
    
    // Append the current time to the filename
    time_t currentTime = time(NULL);
    struct tm * timeinfo;
    char buffer[64];
    timeinfo = localtime (&currentTime);
    strftime(buffer,64,"%d%m%y_%H%M%S",timeinfo);
    m_logFileName = filename + "_" + std::string(buffer) + ".txt";
    
    // Make a temporary object to keep the filename and directory name separate
    std::string logName =  m_logPath + "/" + m_logFileName;
    
    // Create the new file
    std::cout<<"Writing log to: "<<logName<<std::endl;
    if(m_logFile.is_open()) m_logFile.close();
    m_logFile.open(logName.c_str());
  }
  
  // Function to move the current log file to a new location
  void moveLogFile(std::string newDirectory){
    // If there is no file open we can't move it
    if(!m_logFile.is_open()) return;
    // Close the current log file
    m_logFile.close();
    // Rename it
    std::string oldName = m_logPath + "/" + m_logFileName;
    std::string newName = newDirectory + "/" + m_logFileName;
    rename(oldName.c_str(),newName.c_str());
    // Open a new log file
    this->createLogFile();
  }
  
  // Member variables - the debug level, log file name (static), etc.
  std::string m_streamID;
  LogLevel m_logLevel;
  static std::ofstream m_logFile;
  static std::string m_logFileName;
  static std::string m_logPath;
  bool m_newLine;
  
};

// Make external initialisations of info, debug and error. This means
// that all calls from all classes get directed to the same objects
extern logger info;
extern logger debug;
extern logger warning;
extern logger error;

#endif // LOGGER_H