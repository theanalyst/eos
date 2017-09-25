// ----------------------------------------------------------------------
// File: ProcessCache.cc
// Author: Georgios Bitzes - CERN
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

#include "ProcessCache.hh"

ProcessSnapshot ProcessCache::retrieve(pid_t pid, uid_t uid, gid_t gid, bool reconnect) {
  eos_static_debug("ProcessCache::retrieve with pid, uid, gid, reconnect => %d, %d, %d, %d", pid, uid, gid, reconnect);

  ProcessSnapshot entry = cache.retrieve(ProcessCacheKey(pid, uid, gid));
  if(entry) {

    // Cache hit.. but it could refer to different processes, even if PID is the same.
    ProcessInfo processInfo;
    if(!ProcessInfoProvider::retrieveBasic(pid, processInfo)) {
      // dead PIDs issue no syscalls.. or do they?!
      // When a PID dies, the kernel automatically closes its open fds - in this
      // strange case, let's just return the cached info.
      return entry;
    }

    if(processInfo.isSameProcess(entry->getProcessInfo())) {
      // Yep, that's a cache hit, nothing more to do.
      return entry;
    }

    // Process has changed, cache miss
  }

  ProcessInfo processInfo;
  if(!ProcessInfoProvider::retrieveFull(pid, processInfo)) {
    return {};
  }

  bool sidHit = false;
  std::shared_ptr<const BoundIdentity> boundIdentity = boundIdentityProvider.retrieve(pid, uid, gid, reconnect);
  if(!boundIdentity && pid != processInfo.getSid()) {
    // No credentials in this process - check the session leader
    sidHit = true;

    ProcessSnapshot sidSnapshot = this->retrieve(processInfo.getSid(), uid, gid, false);
    if(sidSnapshot && sidSnapshot->filledCredentials()) {
      boundIdentity = std::shared_ptr<const BoundIdentity>(new BoundIdentity(sidSnapshot->getBoundIdentity()));
    }
  }

  // No credentials found - fallback to nobody?
  if(!boundIdentity) {
    if(!credConfig.fallback2nobody) {
      // Give back "permission denied"
      return {};
    }

    // Fallback to nobody
    boundIdentity = std::shared_ptr<const BoundIdentity>(new BoundIdentity());
  }

  ProcessCacheEntry *cacheEntry = new ProcessCacheEntry(processInfo, *boundIdentity.get(), uid, gid);
  cache.store(ProcessCacheKey(pid, uid, gid), cacheEntry);

  // Additionally associate these credentials to (session leader, uid, gid),
  // replacing any existing entries
  if(!sidHit && pid != processInfo.getSid()) {
    ProcessInfo sidInfo;
    if(ProcessInfoProvider::retrieveFull(processInfo.getSid(), sidInfo)) {
      ProcessCacheEntry *sidEntry = new ProcessCacheEntry(sidInfo, *boundIdentity.get(), uid, gid);
      cache.store(ProcessCacheKey(sidInfo.getPid(), uid, gid), sidEntry);
    }
  }

  return cache.retrieve(ProcessCacheKey(pid, uid, gid));
}
