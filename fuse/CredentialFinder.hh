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
#include <sstream>
#include "common/Logging.hh"
#include "Utils.hh"
#include "LoginIdentifier.hh"

// Credentials as given by the user. Not trusted by default, the user could
// play games with race conditions if those were passed on to XrdCl.
//
// These are immutable for a pid + startupTime, for its entire lifetime - even
// if the underlying credential file changes. It's just mapped to a different
// TrustedCredentials object if that happens.
struct CredInfo {
  enum CredType {
    krb5, krk5, x509, nobody
  };

  CredType type;     // krb5 , krk5 or x509
  std::string fname; // credential file

  bool operator<(const CredInfo& src) const {
    if(type != src.type) {
      return type < src.type;
    }

    return fname < src.fname;
  }
};

// Anything ending up here can be trusted, and is passed on as-is to XrdCl.
// - In the case of a shared mount, we have control over these credentials,
//   meaning a malicious user can't play race condition games and change it to
//   a symlink of credentials of a different user.
//
//   If a filename ends up here, the user MUST NOT be able to write to it, even
//   though the credentials identify them!
// - In the case of a user mount we don't really care, all credentials
//   can be considered trusted.

// TODO(gbitzes): actually satisfy the above when instantiating such an object ;>
class TrustedCredentials {
public:
  TrustedCredentials() : initialized(false), type(CredInfo::nobody), uid(-2), gid(-2) {}

  void setKrb5(const std::string &filename, uid_t uid, gid_t gid) {
    if(initialized) THROW("already initialized");

    initialized = true;
    type = CredInfo::krb5;
    contents = filename;
  }

  void setKrk5(const std::string &keyring, uid_t uid, gid_t gid) {
    if(initialized) THROW("already initialized");

    initialized = true;
    type = CredInfo::krk5;
    contents = keyring;
  }

  void setx509(const std::string &filename, uid_t uid, gid_t gid) {
    if(initialized) THROW("already initialized");

    initialized = true;
    type = CredInfo::x509;
    contents = filename;
  }

  bool access(uid_t requid, gid_t reqgid) const {
    if(requid == uid) return true;
    if(reqgid == gid) return true;
    return false;
  }

  std::string toXrdParams() const {
    for(size_t i = 0; i < contents.size(); i++) {
      if(contents[i] == '&' || contents[i] == '=') {
        eos_static_alert("rejecting credential for using forbidden characters: %s", contents.c_str());
        return "xrd.wantprot=unix";
      }
    }

    switch(type) {
      case CredInfo::nobody: {
        return "xrd.wantprot=unix";
      }
      case CredInfo::krb5: {
        return SSTR("xrd.k5ccname=" << contents << "&xrd.wantprot=krb5,unix");
      }
      case CredInfo::krk5: {
        return SSTR("xrd.k5ccname=" << contents << "&xrd.wantprot=krb5,unix");
      }
      case CredInfo::x509: {
        return SSTR("xrd.gsiusrpxy=" << contents << "&xrd.wantprot=gsi,unix");
      }
      default: {
        THROW("should never reach here");
      }
    }
  }

  bool empty() const {
    return !initialized;
  }
private:
  bool initialized;
  CredInfo::CredType type;
  std::string contents;
  uid_t uid;
  gid_t gid;
};

// TrustedCredentials bound to a LoginIdentifier. We need this to talk to the MGM.
class BoundIdentity {
public:
  BoundIdentity() {}

  BoundIdentity(const LoginIdentifier &login_, const std::shared_ptr<TrustedCredentials> &creds_)
  : login(login_), creds(creds_) { }

  LoginIdentifier& getLogin() { return login; }

  std::shared_ptr<TrustedCredentials>& getCreds() { return creds; }
  const std::shared_ptr<TrustedCredentials>& getCreds() const { return creds; }

private:
  LoginIdentifier login;
  std::shared_ptr<TrustedCredentials> creds;
};

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
