//------------------------------------------------------------------------------
// File: CredentialFinder.hh
// Author: Georgios Bitzes - CERN
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

#ifndef __CREDENTIALFINDER__HH__
#define __CREDENTIALFINDER__HH__

#include <string>
#include <map>
#include <vector>

#define SSTR(message) static_cast<std::ostringstream&>(std::ostringstream().flush() << message).str()

// A class to read and parse environment values
class Environment {
public:
  Environment() {}

  void fromFile(const std::string &path);
  void fromString(const std::string &str);
  void fromVector(const std::vector<std::string> &vec);

  std::string get(const std::string &key) const;
  std::vector<std::string> getAll() const;
private:
  std::vector<std::string> contents;
};

class CredentialFinder {
public:
  static std::string locateKerberosTicket(const Environment &env);
  static std::string locateX509Proxy(const Environment &env, uid_t uid);

};

#endif
