// ----------------------------------------------------------------------
// File: ProcCache.cc
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

#define __STDC_FORMAT_MACROS
#include <signal.h>
#define __PROCCACHE__NOGPROCCACHE__
#include "ProcCache.hh"
#include "common/Logging.hh"
#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <XrdSys/XrdSysAtomics.hh>
#include <vector>

//ProcCache gProcCache;
std::vector<ProcCache> gProcCacheV;
int gProcCacheShardSize;

ProcCache gProcCache;

int ProcCacheEntry::UpdateIfPsChanged()
{
  ProcessInfo current;
  if(!ProcessInfoProvider::retrieveBasic(pid, current)) {
    // Looks like our pid has disappeared.
    return ESRCH;
  }

  eos::common::RWMutexWriteLock lock(pMutex);

  if(pInfo.isEmpty()) {
    // First time this function was called
    pInfo = current;
  }
  else if(!pInfo.updateIfSameProcess(current)) {
    // Looks like a different process with the same pid has replaced ours..
    // Refresh everything.

    ProcessInfo newPInfo;
    if(!ProcessInfoProvider::retrieveFull(pid, newPInfo)) {
      // Shouldn't normally happen..
      return ESRCH;
    }

    pInfo = newPInfo;
  }

  return 0;
}
