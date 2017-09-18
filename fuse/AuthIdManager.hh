
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
    pid2StrongLogin.resize(size);
    siduid2credinfo.resize(size);
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
  // LOCKING INFORMATION
  // The AuthIdManager is a stressed system. The credentials are checked for (almost) every single call to fuse.
  // To speed up things, several levels of caching are implemented.
  // On top of that, the following maps that are used for this caching are sharded to avoid contention.
  // This sharding is made such that AuthIdManager::proccachemutexes consecutive pids don't interfere at all between each other.
  // The size of the sharding is given by AuthIdManager::proccachenbins which is copied to gProcCacheShardingSize
  // AuthIdManager::proccachemutexes vector of mutex each one protecting a bin in the sharding.
  std::vector<std::map<pid_t, LoginIdentifier> > pid2StrongLogin;
  // maps (sessionid,userid) -> ( credinfo )
  std::vector<std::map<pid_t, std::map<uid_t, CredInfo>  >> siduid2credinfo;
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

      if (::stat(path.c_str(), &filestat) == 0) {
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

      if (::stat(path.c_str(), &filestat) == 0) {
        credinfo.fname = path;
        credinfo.type = CredInfo::x509;

        eos_static_debug("found credential %s for pid %d", credinfo.fname.c_str(), (int) uid);
        return true;
      }
    }

    eos_static_debug("could not find any credential for pid %d", (int) pid);
    return false;
  }

  bool
  readCred(CredInfo& credinfo)
  {
    bool ret = false;
    eos_static_debug("reading %s credential file %s",
                     credinfo.type == CredInfo::krb5 ? "krb5" : (credinfo.type == CredInfo::krb5 ? "krk5" : "x509"),
                     credinfo.fname.c_str());

    if (credinfo.type == CredInfo::krk5) {
      // fileless authentication cannot rely on symlinks to be able to change the cache credential file
      // instead of the identity, we use the keyring information and each has a different xrd login
      credinfo.identity = credinfo.fname;
      ret = true;
    }

    if (credinfo.type == CredInfo::krb5) {
      ProcReaderKrb5UserName reader(credinfo.fname);

      if (!reader.ReadUserName(credinfo.identity)) {
        eos_static_debug("could not read principal in krb5 cc file %s",
                         credinfo.fname.c_str());
      } else {
        ret = true;
      }
    }

    if (credinfo.type == CredInfo::x509) {
      ProcReaderGsiIdentity reader(credinfo.fname);

      if (!reader.ReadIdentity(credinfo.identity)) {
        eos_static_debug("could not read identity in x509 proxy file %s",
                         credinfo.fname.c_str());
      } else {
        ret = true;
      }
    }

    return ret;
  }

  bool
  checkCredSecurity(const struct stat& filestat, uid_t uid, CredInfo::CredType credtype)
  {
    if (credtype == CredInfo::krk5) {
        return true;
      } else if (filestat.st_uid == uid &&
                 (filestat.st_mode & 0077) == 0 // no access to other users/groups
                 && (filestat.st_mode & 0400) != 0 // read allowed for the user
                ) {
        return true;
      }
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
  void cleanProcCacheBin(unsigned int i, int& cleancountProcCache,
                         int& cleancountStrongLogin, int& cleancountCredInfo)
  {
    eos::common::RWMutexWriteLock lock(proccachemutexes[i]);
    cleancountProcCache += gProcCacheV[i].RemoveEntries(&runningPids);

    for (auto it = pid2StrongLogin[i].begin(); it != pid2StrongLogin[i].end();) {
      if (!runningPids.count(it->first)) {
        pid2StrongLogin[i].erase(it++);
        ++cleancountStrongLogin;
      } else {
        ++it;
      }
    }

    for (auto it = siduid2credinfo[i].begin(); it != siduid2credinfo[i].end();) {
      if (!runningPids.count(it->first)) {
        siduid2credinfo[i].erase(it++);
        cleancountCredInfo++;
      } else {
        ++it;
      }
    }
  }

  int cleanProcCache()
  {
    int cleancountProcCache = 0;
    int cleancountStrongLogin = 0;
    int cleancountCredInfo = 0;

    if (populatePids()) {
      for (unsigned int i = 0; i < proccachenbins; i++) {
        cleanProcCacheBin(i, cleancountProcCache, cleancountStrongLogin,
                          cleancountCredInfo);
      }
    }

    eos_static_info("ProcCache cleaning removed %d entries in gProcCache",
                    cleancountProcCache);
    eos_static_debug("ProcCache cleaning removed %d entries in pid2StrongLogin",
                     cleancountStrongLogin);
    eos_static_debug("ProcCache cleaning removed %d entries in siduid2CredInfo",
                     cleancountCredInfo);
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

    // get the startuptime of the leader of the session
    Jiffies sessionSut = 0;

    if (!gProcCache(sid).GetStartupTime(sid, sessionSut)) {
      sessionSut = 0;
    }

    // find the credentials
    CredInfo credinfo;
    struct stat filestat;

    if (!findCred(credinfo, filestat, pid, uid)) {
      if (credConfig.fallback2nobody) {
        credinfo.type = CredInfo::nobody;
        eos_static_debug("could not find any strong credential for uid %d pid %d sid %d, falling back on 'nobody'",
                         (int)uid, (int)pid, (int)sid);
      } else {
        eos_static_notice("could not find any strong credential for uid %d pid %d sid %d",
                          (int)uid, (int)pid, (int)sid);
        return EACCES;
      }
    }

    // check if the credentials in the credential cache cache are up to date
    // TODO: should we implement a TTL , my guess is NO
    bool sessionInCache = false;

    if (sid != pid) {
      lock_r_pcache(sid, pid);
    }

    bool cacheEntryFound = siduid2credinfo[sid % proccachenbins].count(sid) > 0 &&
                           siduid2credinfo[sid % proccachenbins][sid].count(uid) > 0;
    std::map<uid_t, CredInfo>::iterator cacheEntry;

    if (cacheEntryFound) {
      // skip the cache if reconnecting
      cacheEntry = siduid2credinfo[sid % proccachenbins][sid].find(uid);
      sessionInCache = !reconnect;

      if (sessionInCache) {
        sessionInCache = false;
        const CredInfo& ci = cacheEntry->second;

        if (ci.type == credinfo.type) {
          sessionInCache = true;
        }
      }
    }

    if (sid != pid) {
      unlock_r_pcache(sid, pid);
    }

    if (sessionInCache) {
      // TODO: could detect from the call to ptoccahce_InsertEntry if the process was changed
      //       then, it would be possible to bypass this part copy, which is probably not the main bottleneck anyway
      // no lock needed as only one thread per process can access this (lock is supposed to be already taken -> beginning of the function)
      eos_static_debug("uid=%d  sid=%d  pid=%d  found stronglogin in cache %s",
                       (int)uid, (int)sid, (int)pid, cacheEntry->second.cachedStrongLogin.getStringID().c_str());

      pid2StrongLogin[pid % proccachenbins][pid] = cacheEntry->second.cachedStrongLogin;

      if (gProcCache(sid).HasEntry(sid)) {
        BoundIdentity identity;
        gProcCache(sid).GetBoundIdentity(sid, identity);

        if (gProcCache(pid).HasEntry(pid)) {
          gProcCache(pid).SetBoundIdentity(pid, identity);
        }
      }

      return 0;
    }

    LoginIdentifier loginId; // nobody
    std::shared_ptr<TrustedCredentials> trustedCreds(new TrustedCredentials());

    if (credinfo.type == CredInfo::nobody) {
      // trustedCreds remain empty
      BoundIdentity boundIdentity(loginId, trustedCreds);

      /*** using unix authentication and user nobody ***/
      if (gProcCache(pid).HasEntry(pid)) {
        gProcCache(pid).SetBoundIdentity(pid, boundIdentity);
      }

      // refresh the credentials in the cache
      if (gProcCache(sid).HasEntry(sid)) {
        gProcCache(sid).SetBoundIdentity(sid, boundIdentity);
      }

      // update pid2StrongLogin (no lock needed as only one thread per process can access this)
      pid2StrongLogin[pid % proccachenbins][pid] = LoginIdentifier();
    } else {
      // refresh the credentials in the cache
      // check the credential security
      if (!checkCredSecurity(filestat, uid, credinfo.type)) {
        eos_static_alert("credentials are not safe");
        return EACCES;
      }

      // check the credential security
      if (!readCred(credinfo)) {
        return EACCES;
      }

      // update authmethods for session leader and current pid
      if (credinfo.type == CredInfo::krb5) {
        trustedCreds->setKrb5(credinfo.fname, uid, gid);
      } else if (credinfo.type == CredInfo::krk5) {
        trustedCreds->setKrk5(credinfo.fname, uid, gid);
      } else {
        trustedCreds->setx509(credinfo.fname, uid, gid);
      }

      if (credinfo.type == CredInfo::krk5 && !checkKrk5StringSafe(credinfo.fname)) {
        eos_static_err("deny user %d using of unsafe in memory krb5 credential string '%s'",
                       (int)uid, credinfo.fname.c_str());
        return EPERM;
      }

      if (trustedCreds->empty()) {
        eos_static_err("unknown error, should never happen");
        return EACCES;
      }

      loginId = getNewConId(uid, gid, pid);

      if (loginId.getConnectionID() == 0) {
        eos_static_alert("running out of XRootD connections");
        errCode = EBUSY;
        return errCode;
      }

      BoundIdentity boundIdentity(loginId, trustedCreds);
      gProcCache(pid).SetBoundIdentity(pid, boundIdentity);
      gProcCache(sid).SetBoundIdentity(sid, boundIdentity);

      // update pid2StrongLogin (no lock needed as only one thread per process can access this)
      pid2StrongLogin[pid % proccachenbins][pid] = loginId;
    }

    // update uidsid2credinfo
    credinfo.cachedStrongLogin = pid2StrongLogin[pid % proccachenbins][pid];
    eos_static_debug("uid=%d  sid=%d  pid=%d  writing stronglogin in cache %s",
                     (int)uid, (int)sid, (int)pid, credinfo.cachedStrongLogin.getStringID().c_str());

    if (sid != pid) {
      lock_w_pcache(sid, pid);
    }

    siduid2credinfo[sid % proccachenbins][sid][uid] = credinfo;

    if (sid != pid) {
      unlock_w_pcache(sid, pid);
    }

    eos_static_info("trustedCredentials [%s] used for pid %d, xrdlogin is %s (%d/%d)",
                    trustedCreds->toXrdParams().c_str(), (int)pid,
                    pid2StrongLogin[pid % proccachenbins][pid].getStringID().c_str(), (int)uid, (int)loginId.getConnectionID());
    return errCode;
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
