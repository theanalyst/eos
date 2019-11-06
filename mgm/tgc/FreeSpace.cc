// ----------------------------------------------------------------------
// File: FreeSpace.cc
// Author: Steven Murray - CERN
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

#include "mgm/FsView.hh"
#include "mgm/tgc/FreeSpace.hh"
#include "mgm/tgc/SpaceNotFound.hh"
#include "mgm/tgc/Utils.hh"

#include <functional>
#include <sstream>

EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
FreeSpace::FreeSpace(const std::string &space,
  const time_t queryPeriodSecs):
  m_space(space),
  m_queryPeriodSecs(
    std::bind(FreeSpace::getConfSpaceQueryPeriodSecs, space, queryPeriodSecs),
    10), // Maximum age of cached value in seconds
  m_freeSpaceBytes(0),
  m_freeSpaceQueryTimestamp(0)
{
}

//------------------------------------------------------------------------------
// Notify this object that a file has been queued for deletion
//------------------------------------------------------------------------------
void
FreeSpace::fileQueuedForDeletion(const size_t deletedFileSize)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if(m_freeSpaceBytes < deletedFileSize) {
    m_freeSpaceBytes = 0;
  } else {
    m_freeSpaceBytes -= deletedFileSize;
  }
}

//------------------------------------------------------------------------------
// Return the amount of free space in bytes
//------------------------------------------------------------------------------
uint64_t
FreeSpace::getFreeBytes()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  const time_t now = time(nullptr);
  const time_t secsSinceLastQuery = now - m_freeSpaceQueryTimestamp;

  const auto queryPeriodSecs = getQueryPeriodSecsAndLogIfChanged();

  if(secsSinceLastQuery >= queryPeriodSecs) {
    m_freeSpaceQueryTimestamp = now;
    m_freeSpaceBytes = queryMgmForFreeBytes();
  }
  return m_freeSpaceBytes;
}

//------------------------------------------------------------------------------
// Returns the configured query period and logs if changed
//------------------------------------------------------------------------------
time_t
FreeSpace::getQueryPeriodSecsAndLogIfChanged()
{
  const auto queryPeriodSecs = m_queryPeriodSecs.get();
  if (queryPeriodSecs.prev != queryPeriodSecs.current) {
    std::ostringstream msg;
    msg << "msg=\"spaceQueryPeriodSecs has been changed from " << queryPeriodSecs.prev  << " to " <<
      queryPeriodSecs.current << "\"";
    eos_static_info(msg.str().c_str());
  }

  return queryPeriodSecs.current;
}

//------------------------------------------------------------------------------
// Return the timestamp at which the last free space query was made
//------------------------------------------------------------------------------
time_t
FreeSpace::getFreeSpaceQueryTimestamp() {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_freeSpaceQueryTimestamp;
}

//------------------------------------------------------------------------------
// Query the EOS MGM for the amount of free space in bytes
//------------------------------------------------------------------------------
uint64_t
FreeSpace::queryMgmForFreeBytes() {
  eos::common::RWMutexReadLock lock(FsView::gFsView.ViewMutex);

  const auto spaceItor = FsView::gFsView.mSpaceView.find(m_space);

  if(FsView::gFsView.mSpaceView.end() == spaceItor) {
    throw SpaceNotFound(std::string(__FUNCTION__) + ": Cannot find space " + m_space +
                        ": FsView does not know the space name");
  }

  if(nullptr == spaceItor->second) {
    throw SpaceNotFound(std::string(__FUNCTION__) + ": Cannot find space " + m_space +
                        ": Pointer to FsSpace is nullptr");
  }

  const FsSpace &space = *(spaceItor->second);

  uint64_t freeBytes = 0;
  for(const auto fsid: space) {
    FileSystem * const fs = FsView::gFsView.mIdView.lookupByID(fsid);

    // Skip this file system if it cannot be found
    if(nullptr == fs) {
      std::ostringstream msg;
      msg << "Unable to find file system: space=" << m_space << " fsid=" << fsid;
      eos_static_warning(msg.str().c_str());
      continue;
    }

    common::FileSystem::fs_snapshot_t fsSnapshot;

    // Skip this file system if a snapshot cannot be taken
    const bool doLock = true;
    if(!fs->SnapShotFileSystem(fsSnapshot, doLock)) {
      std::ostringstream msg;
      msg << "Unable to take a snaphot of file system: space=" << m_space << " fsid=" << fsid;
      eos_static_warning(msg.str().c_str());
    }

    // Only consider file systems that are booted, on-line and read/write
    if(common::BootStatus::kBooted == fsSnapshot.mStatus &&
       common::ActiveStatus::kOnline == fsSnapshot.mActiveStatus &&
       common::ConfigStatus::kRW == fsSnapshot.mConfigStatus) {
      freeBytes += (uint64_t)fsSnapshot.mDiskBavail * (uint64_t)fsSnapshot.mDiskBsize;
    }
  }

  return freeBytes;
}

//------------------------------------------------------------------------------
// Return the configured delay in seconds between free space queries
//------------------------------------------------------------------------------
uint64_t
FreeSpace::getConfSpaceQueryPeriodSecs(const std::string spaceName,
  const uint64_t defaultValue) noexcept
{
  try {
    std::string valueStr;

    {
      eos::common::RWMutexReadLock lock(FsView::gFsView.ViewMutex);

      const auto spaceItor = FsView::gFsView.mSpaceView.find(spaceName);
      if (FsView::gFsView.mSpaceView.end() != spaceItor && nullptr != spaceItor->second) {
        const auto &space = *(spaceItor->second);
        valueStr = space.GetConfigMember("tapeawaregc.spacequeryperiodsecs");
      }
    }

    return Utils::toUint64(valueStr);
  } catch(...) {
    return defaultValue;
  }
}


EOSTGCNAMESPACE_END
