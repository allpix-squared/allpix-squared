/**
 * AllPix exception classes
 * 
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 */

#ifndef ALLPIX_EXCEPTIONS_H
#define ALLPIX_EXCEPTIONS_H

#include <exception>
#include <string>
#include <typeinfo>

namespace allpix {

  /** 
   * Base class for all exceptions thrown by the allpix framework.
   */
  class Exception : public std::exception {
  public:
      Exception(): error_message_("Unspecified error") {}
      Exception(const std::string& what_arg) : std::exception(), error_message_(what_arg) {}
      ~Exception() throw() {}
      virtual const char* what() const throw(){
          return error_message_.c_str();
      };
  private:
      std::string error_message_;
  };
  
  
  /* 
   * Base class for all configuration related errors
   */
  class ConfigurationError : public Exception {};
  
  class InvalidKeyError : public ConfigurationError {
  public:
      InvalidKeyError(const std::string&key, const std::string &value, const std::type_info &type): key_(key), value_(value), type_(type.name()) {};
      ~InvalidKeyError() throw() {}
      virtual const char* what() const throw(){
          std::string msg = "Could not convert value '"+value_+"' of key '"+key_+"' to type "+type_;
          return msg.c_str();
      };
  private:
      std::string key_;
      std::string value_;
      std::string type_;
  };
  
  class ConfigParseError : public ConfigurationError{
  public:
      ConfigParseError(std::string &line, int line_num);
      ~ConfigParseError() throw() {}
      /*virtual const char* what() const throw(){
          return error_message_.c_str();
      };*/
  private:
  };
  
  class MissingKeyError : public ConfigurationError{
  public:
      MissingKeyError(const std::string&key);
      ~MissingKeyError() throw() {}
      /*virtual const char* what() const throw(){
          return error_message_.c_str();
      };*/
  private:
  };
  
  // errors config users can trigger (should always come from our exceptions)
  /**
  
  class ConfigurationError : public exception or runtime_error; // all config errors
  class InvalidKeyError : ConfigurationError
  class ConfigParseError : ConfigurationError
  class MissingKeyError : ConfigurationError
  
  class InvalidConfigurationException : public ConfigurationException //out of range parameters , missing parameters , inconsistent configuration
  
  class InstantiationError: public runtime_error // errors with instantiation (NOTE: never thrown by the module itself, factory is allowed)
  class UnexpectedFinalizeException  : public runtime_error //if a module should have run but it did not (WARNING: can both be a config or module error)
  class UnexpectedMessageException : public runtime_error // if a module receives a message more times than expected (or a message is called that has no receivers but should)
  
  // errors the module developers (or we can trigger) mostly logic errors
  // std::logic_error out_of_range etc if applicable internally if the user can call the method
  class InvalidModuleStateException or UnexpectedModuleBehaviourException : public exception or logic_error // if we detect that messages are dispatched outside the run function
  class InvalidModuleActionException : public exception or logic_error // if a module execute an action it should not (could be useful to declare beforehand which actions it should do or is this included in the previous one)
  class InvalidModuleException ??? : // more general exception if module misbehaves
  
  // assert if only the internal runtime can trigger it and it should never happen (like dispatching the message linked to a typeid)
  
  **/
  
} //namespace allpix

#endif /* ALLPIX_EXCEPTIONS_H */
