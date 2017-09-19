//------------------------------------------------------------------------------
// File: BoundIdentityProvider.cc
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

#include "Utils.hh"
#include "BoundIdentityProvider.hh"
#include <sys/stat.h>

// A preliminary check that provided credentials are sane.
// The strong check will be provided by XrdCl, which changes its fsuid when
// reading the credentials to that of the real user.
bool BoundIdentityProvider::checkCredsPath(const std::string &path, uid_t uid) {
  if(path.size() == 0) {
    return false;
  }

  struct stat filestat;
  if(::stat(path.c_str(), &filestat) != 0) {
    eos_static_debug("Cannot stat credentials path %s (requested by uid %d)", path.c_str(), uid);
    return false;
  }

  if(!checkCredSecurity(filestat, uid)) {
    eos_static_alert("Credentials path %s was requested for use by uid %d, but permission check failed!", path.c_str(), uid);
    return false;
  }

  return true;
}

bool BoundIdentityProvider::fillKrb5FromEnv(const Environment &env, CredInfo &creds, uid_t uid) {
  std::string path = CredentialFinder::locateKerberosTicket(env);
  if(!checkCredsPath(path, uid)) {
    return false;
  }

  eos_static_info("Using kerberos credentials '%s' for uid %d", path.c_str(), uid);
  creds.fname = path;
  creds.type = CredInfo::krb5;
  return true;
}

bool BoundIdentityProvider::fillX509FromEnv(const Environment &env, CredInfo &creds, uid_t uid) {
  std::string path = CredentialFinder::locateX509Proxy(env, uid);
  if(!checkCredsPath(path, uid)) {
    return false;
  }

  eos_static_info("Using x509 credentials '%s' for uid %d", path.c_str(), uid);
  creds.fname = path;
  creds.type = CredInfo::x509;
  return true;
}

bool BoundIdentityProvider::fillCredsFromEnv(const Environment &env, const CredentialConfig &credConfig, CredInfo &creds, uid_t uid) {
  if(credConfig.tryKrb5First) {
    if(credConfig.use_user_krb5cc && BoundIdentityProvider::fillKrb5FromEnv(env, creds, uid)) {
      return true;
    }

    if(credConfig.use_user_gsiproxy && BoundIdentityProvider::fillX509FromEnv(env, creds, uid)) {
      return true;
    }

    return false;
  }

  // Try krb5 second
  if(credConfig.use_user_gsiproxy && BoundIdentityProvider::fillX509FromEnv(env, creds, uid)) {
    return true;
  }

  if(credConfig.use_user_krb5cc && BoundIdentityProvider::fillKrb5FromEnv(env, creds, uid)) {
    return true;
  }

  return false;
}
