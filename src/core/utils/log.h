/**
 * allpix logging class
 */

#ifndef ALLPIX_LOG_H
#define ALLPIX_LOG_H

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef unsigned int uint32_t;
typedef unsigned int DWORD;
#include <Windows4Root.h>
#else
#if (defined WIN32)
typedef unsigned int uint32_t;
#include <Windows.h>
#else
#include <sys/time.h>
#include <stdint.h>
#endif //WIN32
#endif //WIN32 && CINT

#ifdef WIN32
#define __func__ __FUNCTION__
#endif // WIN32

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <string.h>


namespace allpix {

  enum TLogLevel {
    logQUIET,
    logCRITICAL,
    logERROR,
    logWARNING,
    logINFO,
    logDEBUG
  };

  template <typename T>
    class allpixLog {
  public:
    allpixLog();
    virtual ~allpixLog();
    std::ostringstream& Get(TLogLevel level = logINFO, std::string file = "", std::string function = "", uint32_t line = 0);
    static std::string& logName(std::string setLogName = "") {
        static std::string logName("");
        static bool is_initialized = false;
        if (!is_initialized && setLogName.size() > 0) {
          is_initialized = true;
          logName = setLogName;
        }
        return logName;
    }; 
  public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
    static TLogLevel FromString(const std::string& level);
  protected:
    std::ostringstream os;
  private:
    allpixLog(const allpixLog&);
    allpixLog& operator =(const allpixLog&);
    std::string NowTime();
  };

  template <typename T>
    allpixLog<T>::allpixLog() {}


#ifdef WIN32

  template <typename T>
    std::string allpixLog<T>::NowTime(){
    const int MAX_LEN = 200;
    char buffer[MAX_LEN];
    if (GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0, 
            "HH':'mm':'ss", buffer, MAX_LEN) == 0)
        return "Error in NowTime()";

    char result[100] = {0};
    static DWORD first = GetTickCount();
    std::sprintf(result, "%s.%03ld", buffer, static_cast<long>(GetTickCount() - first) % 1000); 
    return result;
}

#else

  template <typename T>
    std::string allpixLog<T>::NowTime() {
    char buffer[11];
    time_t t;
    time(&t);
    tm r = * localtime(&t);
    strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
    struct timeval tv;
    gettimeofday(&tv, 0);
    char result[100] = {0};
    std::sprintf(result, "%s.%03ld", buffer, static_cast<long>(tv.tv_usec) / 1000); 
    return result;
  }

#endif //WIN32

  template <typename T>
    std::ostringstream& allpixLog<T>::Get(TLogLevel level, std::string file, std::string function, uint32_t line) {
    os << "[" << NowTime() << "] ";
    if (logName().size() > 0) {
      os << "<" << logName() << "> ";
    }
    os << std::setw(8) << ToString(level) << ": ";
    
    // For debug levels we want also function name and line number printed:
    if (level != logINFO && level != logQUIET && level != logWARNING)
      os << "<" << file << "/" << function << ":L" << line << "> ";

    return os;
  }

  template <typename T>
    allpixLog<T>::~allpixLog() {
    os << std::endl;
    T::Output(os.str());
  }

  template <typename T>
    TLogLevel& allpixLog<T>::ReportingLevel() {
    static TLogLevel reportingLevel = logINFO;
    return reportingLevel;
  }

  template <typename T>
    std::string allpixLog<T>::ToString(TLogLevel level) {
    static const char* const buffer[] = {"QUIET","CRITICAL","ERROR", "WARNING", "INFO", "DEBUG"};
    return buffer[level];
  }

  template <typename T>
    TLogLevel allpixLog<T>::FromString(const std::string& level) {
    if (level == "DEBUG")
      return logDEBUG;
    if (level == "INFO")
      return logINFO;
    if (level == "WARNING")
      return logWARNING;
    if (level == "ERROR")
      return logERROR;
    if (level == "CRITICAL")
      return logCRITICAL;
    if (level == "QUIET")
      return logQUIET;
    allpixLog<T>().Get(logWARNING) << "Unknown logging level '" << level << "'. Using WARNING level as default.";
    return logWARNING;
  }


  class SetLogOutput
  {
  public:
    static FILE*& Stream();
    static bool& Duplicate();
    static void Output(const std::string& msg);
  };

  inline bool& SetLogOutput::Duplicate()
  {
    static bool duplic = false;
    return duplic;
  }

  inline FILE*& SetLogOutput::Stream()
  {
    static FILE* pStream = stderr;
    return pStream;
  }

  inline void SetLogOutput::Output(const std::string& msg)
  {   
    FILE* pStream = Stream();
    if (!pStream)
      return;
    // Check if duplication to stderr is needed:
    if (Duplicate() && pStream != stderr)
      fprintf(stderr, "%s", msg.c_str());
    fprintf(pStream, "%s", msg.c_str());
    fflush(pStream);
  }

typedef allpixLog<SetLogOutput> Log;

#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define IFLOG(level) \
  if (level > allpix::Log::ReportingLevel() || !allpix::SetLogOutput::Stream()) ; \
  else 

#define LOG(level)				\
  if (level > allpix::Log::ReportingLevel() || !allpix::SetLogOutput::Stream()) ; \
  else allpix::Log().Get(level,__FILE_NAME__,__func__,__LINE__)

} //namespace allpix

#endif /* ALLPIX_LOG_H */
