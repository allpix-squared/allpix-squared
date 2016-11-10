/**
 * allpix API exception classes
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
    exception(const std::string& what_arg) : std::exception(),ErrorMessage(what_arg) {}
    ~exception() throw() {};
    virtual const char* what() const throw(){
      return ErrorMessage.c_str();
    };
  private:
    std::string ErrorMessage;
  };
  
} //namespace allpix

#endif /* ALLPIX_EXCEPTIONS_H */
