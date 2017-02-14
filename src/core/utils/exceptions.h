/**
 * AllPix exception classes
 * 
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 */

#ifndef ALLPIX_EXCEPTIONS_H
#define ALLPIX_EXCEPTIONS_H

#include <exception>
#include <string>

namespace allpix {

  /** Base class for all exceptions thrown by the allpix framework.
   */
  class exception : public std::exception {
  public:
    exception(const std::string& what_arg) : std::exception(),error_message(what_arg) {}
    ~exception() throw() {};
    virtual const char* what() const throw(){
      return error_message.c_str();
    };
  private:
    std::string error_message;
  };
  
} //namespace allpix

#endif /* ALLPIX_EXCEPTIONS_H */
