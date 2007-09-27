#ifndef ERROR_H_
#define ERROR_H_

#include "log.h"

namespace ll {

/**
 * Our own exception for errors that occur in the system
 */
class bad_lemon : public exception {
private:
  const char* _msg;

public:
  bad_lemon(const char* msg = NULL) : _msg(msg)
  { log << error << msg << endl; }
  
  virtual const char* what() const throw()
  { return _msg; }
};

} // end namespace

#endif /*ERROR_H_*/
