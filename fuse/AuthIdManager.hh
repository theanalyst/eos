// ----------------------------------------------------------------------
// File: AuthIdManager.hh
// Author: Geoffray Adde - CERN
// ----------------------------------------------------------------------

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

#ifndef __AUTHIDMANAGER__HH__
#define __AUTHIDMANAGER__HH__

/*----------------------------------------------------------------------------*/
#include "MacOSXHelper.hh"
#include "common/Logging.hh"
#include "common/RWMutex.hh"
#include "common/SymKeys.hh"
#include "ProcCache.hh"
#include "CredentialFinder.hh"
#include "LoginIdentifier.hh"
#include "CredentialCache.hh"
#include "Utils.hh"
/*----------------------------------------------------------------------------*/
#include "XrdOuc/XrdOucString.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdSys/XrdSysAtomics.hh"
/*----------------------------------------------------------------------------*/
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

class CredentialConfig {
public:
  CredentialConfig() : use_user_krb5cc(false), use_user_gsiproxy(false),
  use_unsafe_krk5(false), tryKrb5First(false), fallback2nobody(false), fuse_shared(false) {}

  //! Indicates if user krb5cc file should be used for authentication
  bool use_user_krb5cc;
  //! Indicates if user gsi proxy should be used for authentication
  bool use_user_gsiproxy;
  //! Indicates if in memory krb5 tickets can be used without any safety check
  bool use_unsafe_krk5;
  //! Indicates if Krb5 should be tried before Gsi
  bool tryKrb5First;
  //! Indicates if unix authentication (as nobody) should be used as a fallback
  //! if strong authentication is configured and none is found
  bool fallback2nobody;
  //! Indicates if this is a shared fuse mount
  bool fuse_shared;
};

//------------------------------------------------------------------------------
// Class in charge of managing the xroot login (i.e. xroot connection)
// logins are 8 characters long : ABgE73AA23@myrootserver
// it's base 64 , first 6 are userid and 2 lasts are authid
// authid is an idx a pool of identities for the specified user
// if the user comes with a new identity, it's added the pool
// if the identity is already in the pool, the connection is reused
// identity are NEVER removed from the pool
// for a given identity, the SAME conneciton is ALWAYS reused
//------------------------------------------------------------------------------

class AuthIdManager
{
public:
  CredentialConfig credConfig;

  void
  setAuth(const CredentialConfig &cf)
  {
    credConfig = cf;
  }

  void
  resize(ssize_t size)
  {
    proccachemutexes.resize(size);
  }

  int connectionId;
  XrdSysMutex connectionIdMutex;

  int
  GetConnectionId()
  {
    XrdSysMutexHelper l(connectionIdMutex);
    return connectionId;
  }

  void
  IncConnectionId()
  {
    XrdSysMutexHelper l(connectionIdMutex);
    connectionId++;
  }

  static const unsigned int proccachenbins;
  std::vector<eos::common::RWMutex> proccachemutexes;

  //------------------------------------------------------------------------------
  // Lock
  //------------------------------------------------------------------------------

  void
  lock_r_pcache(pid_t pid, pid_t pid_locked)
  {
    if ((pid % proccachenbins) != (pid_locked % proccachenbins)) {
      proccachemutexes[pid % proccachenbins].LockRead();
    }
  }

  void
  lock_w_pcache(pid_t pid, pid_t pid_locked)
  {
    if ((pid % proccachenbins) != (pid_locked % proccachenbins)) {
      proccachemutexes[pid % proccachenbins].LockWrite();
    }
  }

  //------------------------------------------------------------------------------
  // Unlock
  //------------------------------------------------------------------------------
  void
  unlock_r_pcache(pid_t pid, pid_t pid_locked)
  {
    if ((pid % proccachenbins) != (pid_locked % proccachenbins)) {
      proccachemutexes[pid % proccachenbins].UnLockRead();
    }
  }

  void
  unlock_w_pcache(pid_t pid, pid_t pid_locked)
  {
    if ((pid % proccachenbins) != (pid_locked % proccachenbins)) {
      proccachemutexes[pid % proccachenbins].UnLockWrite();
    }
  }

  //----------------------------------------------------------------------------
  //! Constructor
  //----------------------------------------------------------------------------
  AuthIdManager():
    connectionId(0), mCleanupThread()
  {
    resize(proccachenbins);
  }

protected:
  CredentialCache credentialCache;

  static uint64_t sConIdCount;
  std::set<pid_t> runningPids;
  pthread_t mCleanupThread;

  bool
  findCred(CredInfo& credinfo, struct stat& filestat, pid_t pid, uid_t uid)
  {
    if (!(credConfig.use_user_gsiproxy || credConfig.use_user_krb5cc)) {
      return false;
    }

    // get process environment
    Environment processEnv;
    processEnv.fromFile(SSTR("/proc/" << pid << "/environ"));

    // try krb5
    if(credConfig.use_user_krb5cc) {
      std::string path = CredentialFinder::locateKerberosTicket(processEnv);
      eos_static_debug("locate kerberos, path: %s", path.c_str());

      if (::stat(path.c_str(), &filestat) == 0 && checkCredSecurity(filestat, uid)) {
        credinfo.fname = path;
        credinfo.type = CredInfo::krb5;

        eos_static_debug("found credential %s for pid %d", credinfo.fname.c_str(), (int) pid);
        return true;
      }
    }

    // try gsi
    if(credConfig.use_user_gsiproxy) {
      std::string path = CredentialFinder::locateX509Proxy(processEnv, uid);
      eos_static_debug("locate gsi proxy, path: %s", path.c_str());

      if (::stat(path.c_str(), &filestat) == 0 && checkCredSecurity(filestat, uid)) {
        credinfo.fname = path;
        credinfo.type = CredInfo::x509;

        eos_static_debug("found credential %s for pid %d", credinfo.fname.c_str(), (int) uid);
        return true;
      }
    }

    eos_static_debug("could not find any credential for pid %d", (int) pid);
    return false;
  }

  inline bool
  checkKrk5StringSafe(const std::string& krk5Str)
  {
    // TODO: implement here check to be done on in memory krb5 tickets
    return credConfig.use_unsafe_krk5;
  }

  inline LoginIdentifier getNewConId(uid_t uid, gid_t gid, pid_t pid)
  {
    //NOTE: we have (2^6)^7 ~= 5e12 connections which is basically infinite
    //      fot the moment, we don't reuse connections at all, we leave them behind
    //TODO: implement conid pooling when disconnect is implementend in XRootD
    if (sConIdCount == ((1ull << 42) - 1)) {
      return 0;
    }

    return LoginIdentifier(AtomicInc(sConIdCount) + 1);
  }

  inline void releaseConId(uint64_t conid)
  {
    //TODO: implement channel disconnection when implementend in XRootD
  }

  bool populatePids()
  {
    runningPids.clear();
    // when entering this function proccachemutexes[pid] must be write locked
    errno = 0;
    auto dirp = opendir(gProcCache(0).GetProcPath().c_str());

    if (dirp == NULL) {
      eos_static_err("error openning %s to get running pids. errno=%d",
                     gProcCache(0).GetProcPath().c_str(), errno);
      return false;
    }

    // this is useful even in gateway mode because of the recursive deletion protection
    struct dirent* dp = NULL;
    long long int i = 0;

    while ((dp = readdir(dirp)) != NULL) {
      errno = 0;

      if (dp->d_type == DT_DIR && (i = strtoll(dp->d_name, NULL, 10)) &&
          (errno == 0)) {
        runningPids.insert(static_cast<pid_t>(i));
      }
    }

    (void) closedir(dirp);
    return true;
  }
  void cleanProcCacheBin(unsigned int i, int& cleancountProcCache)
  {
    eos::common::RWMutexWriteLock lock(proccachemutexes[i]);
    cleancountProcCache += gProcCacheV[i].RemoveEntries(&runningPids);
  }

  int cleanProcCache()
  {
    int cleancountProcCache = 0;

    if (populatePids()) {
      for (unsigned int i = 0; i < proccachenbins; i++) {
        cleanProcCacheBin(i, cleancountProcCache);
      }
    }

    eos_static_info("ProcCache cleaning removed %d entries in gProcCache",
                    cleancountProcCache);
    return 0;
  }

  static void*
  CleanupThread(void* arg);

  void CleanupLoop()
  {
    XrdSysTimer sleeper;

    while (true) {
      sleeper.Snooze(300);
      cleanProcCache();
    }
  }


  int
  updateProcCache(uid_t uid, gid_t gid, pid_t pid, bool reconnect)
  {
    // when entering this function proccachemutexes[pid] must be write locked
    // this is to prevent several thread calling fuse from the same pid to enter this code
    // and to create a race condition
    // most of the time, credentials in the cache are up to date and the lock is held for a short time
    // and the locking is shared so that only the pid with the same pid%AuthIdManager::proccachenbins
    int errCode;

    // this is useful even in gateway mode because of the recursive deletion protection
    if ((errCode = gProcCache(pid).InsertEntry(pid))) {
      eos_static_err("updating proc cache information for process %d. Error code is %d",
                     (int)pid, errCode);
      return errCode;
    }

    // check if we are using strong authentication
    if (!(credConfig.use_user_krb5cc || credConfig.use_user_gsiproxy)) {
      return 0;
    }

    // get the session id
    pid_t sid = 0;
    gProcCache(pid).GetSid(pid, sid);

    // Update the proccache of the session leader
    if (sid != pid) {
      lock_w_pcache(sid, pid);

      if ((errCode = gProcCache(sid).InsertEntry(sid))) {
        unlock_w_pcache(sid, pid);
        eos_static_debug("updating proc cache information for session leader process %d failed. Session leader process %d does not exist",
                         (int)pid, (int)sid);
        sid = -1;
      } else {
        unlock_w_pcache(sid, pid);
      }
    }

    // Does this process have a bound identity already? Nothing to do
    if (!reconnect && gProcCache(pid).HasEntry(pid) && gProcCache(pid).HasBoundIdentity(pid)) return 0;

    // No bound identity, let's build one. First, let's check the environment
    // of this PID for KRB5CCNAME and friends..

    CredInfo credinfo;
    struct stat filestat;

    if(!findCred(credinfo, filestat, pid, uid)) {
      // Nope, nothing here. Let's check its session leader..
      // TODO(gbitzes): AFS does not check the environment of the session leader,
      // as far I know.. is this necessary for us?
      if(!findCred(credinfo, filestat, sid, uid)) {

        // No credentials found..
        if(credConfig.fallback2nobody) {
          // Bind this process to nobody.
          BoundIdentity identity;
          gProcCache(pid).SetBoundIdentity(pid, identity);
          return 0;
        }

        // Return "permission denied"
        return EACCES;
      }
    }

    // We found some credentials, yay. We have to bind them to an xrootd
    // connection - does such a binding exist already? We don't want to
    // waste too many LoginIdentifiers, so we re-use them when possible.

    std::shared_ptr<const BoundIdentity> boundIdentity = credentialCache.retrieve(credinfo);

    if(boundIdentity && !reconnect) {
      // Cache hit, CredInfo is bound already, re-use.
      gProcCache(pid).SetBoundIdentity(pid, *boundIdentity.get());
      return 0;
    }

    // No binding exists yet, let's create one.
    LoginIdentifier login = getNewConId(uid, gid, pid);
    std::shared_ptr<TrustedCredentials> trustedCreds(new TrustedCredentials());

    if (credinfo.type == CredInfo::krb5) {
      trustedCreds->setKrb5(credinfo.fname, uid, gid);
    } else if (credinfo.type == CredInfo::krk5) {
      trustedCreds->setKrk5(credinfo.fname, uid, gid);
    } else if (credinfo.type == CredInfo::x509) {
      trustedCreds->setx509(credinfo.fname, uid, gid);
    }

    BoundIdentity *binding = new BoundIdentity(login, trustedCreds);
    gProcCache(pid).SetBoundIdentity(pid, *binding);
    credentialCache.store(credinfo, binding);
    // no delete on binding, ownership was transferred to the cache
    return 0;
  }

  //------------------------------------------------------------------------------
  // Get user name from the uid and change the effective user ID of the thread
  //------------------------------------------------------------------------------

  LoginIdentifier
  getXrdLogin(pid_t pid)
  {
    BoundIdentity boundIdentity;
    gProcCache(pid).GetBoundIdentity(pid, boundIdentity);
    return boundIdentity.getLogin();
  }

public:
  bool
  StartCleanupThread()
  {
    // Start worker thread
    if ((XrdSysThread::Run(&mCleanupThread, AuthIdManager::CleanupThread,
                           static_cast<void*>(this)))) {
      eos_static_crit("can not start cleanup thread");
      return false;
    }

    return true;
  }

  inline int
  updateProcCache(uid_t uid, gid_t gid, pid_t pid)
  {
    eos::common::RWMutexWriteLock lock(proccachemutexes[pid % proccachenbins]);
    return updateProcCache(uid, gid, pid, false);
  }

  inline int
  reconnectProcCache(uid_t uid, gid_t gid, pid_t pid)
  {
    eos::common::RWMutexWriteLock lock(proccachemutexes[pid % proccachenbins]);
    return updateProcCache(uid, gid, pid, true);
  }

  LoginIdentifier
  getLogin(uid_t uid, gid_t gid, pid_t pid)
  {
    if(credConfig.use_user_krb5cc || credConfig.use_user_gsiproxy) {
      return getXrdLogin(pid);
    }

    return LoginIdentifier(uid, gid, pid, GetConnectionId());
  }

};

#endif // __AUTHIDMANAGER__HH__
