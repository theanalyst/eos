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
  ProcessSnapshot entry = cache.retrieve(ProcessCacheKey(pid, uid, gid));
  if(entry) {

    // Cache hit.. but it could refer to different processes, even if PID is the same.
    ProcessInfo processInfo;
    if(!ProcessInfoProvider::retrieveBasic(pid, processInfo)) {
      return {};
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

  std::shared_ptr<const BoundIdentity> boundIdentity = boundIdentityProvider.retrieve(pid, uid, gid, reconnect);
  ProcessCacheEntry *cacheEntry = new ProcessCacheEntry(processInfo, *boundIdentity.get(), uid, gid);
  cache.store(ProcessCacheKey(pid, uid, gid), cacheEntry);

  return cache.retrieve(ProcessCacheKey(pid, uid, gid));
}
