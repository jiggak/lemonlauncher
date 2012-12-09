/*
 * Copyright 2007 Josh Kropf
 * 
 * This file is part of Lemon Launcher.
 * 
 * Lemon Launcher is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Lemon Launcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Lemon Launcher; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <cstdio>

using namespace std;

namespace ll {

/**
 * Log level enumeration for defining at what level lop output occurs
 */
typedef enum { off, error, info, warn, debug } log_level;

/**
 * Implementation of streambuf to provide output of logging message.
 * Currently this implementation simply relays output to stdout but
 * could be modified to add other destinations such as a file.
 */
class log_buf : public streambuf
{
private:
   log_level _threshold;
   log_level _current;

protected:
   virtual int overflow(int c = EOF)
   {
      if (_current <= _threshold) cout.put(c);
      return c;
   }

   virtual int sync()
   {
      _current = off;
      return 0;
   }

public:
   void current(log_level level)
   { _current = level; }
   
   void threshold(log_level level)
   { _threshold = level; }
};

/**
 * Handy dandy logging class.  See log_bug for buffer implementation.
 * 
 * Use standard ostream insertion operator to for logging.  All logging
 * operations must begin with a log_level enum constant or else output
 * is ignored.
 * 
 * Furthurmore, if the operation does not end with an endl manipulator
 * or the flush method is not called, the log level persists!
 * 
 * Example:
 *   log << info << "some info level logging" << endl;
 */
class logger : public ostream {
public:
   logger() : ostream(new log_buf) { }
   
   void level(log_level level)
   { rdbuf()->threshold(level); }
   
   /** Returns a typesafe pointer to log buf */
   log_buf* rdbuf() const
   { return (log_buf*)ostream::rdbuf(); }
};

extern logger log;

/**
 * Insertion operator to handle setting log level for the current log output
 * operation.  This operation stays current until the endl manipulator is
 * reached, or the stream has been flushed.  If neither of these conditions
 * occur the current log level persists (probably not the most idea scenario).
 */
static logger& operator<<(logger& stream, log_level level)
{
   stream.rdbuf()->current(level);
   return stream;
}

} // end namespace declaration

#endif /*LOG_H_*/
