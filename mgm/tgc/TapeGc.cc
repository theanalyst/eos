// ----------------------------------------------------------------------
// File: TapeGc.cc
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

#include "mgm/tgc/Constants.hh"
#include "mgm/tgc/TapeGc.hh"
#include "mgm/tgc/SpaceNotFound.hh"
#include "mgm/tgc/Utils.hh"

#include <functional>
#include <ios>
#include <sstream>

EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
TapeGc::TapeGc(ITapeGcMgm &mgm, const std::string &spaceName, const std::time_t maxConfigCacheAgeSecs):
  m_mgm(mgm),
  m_spaceName(spaceName),
  m_enabled(false),
  m_config(m_mgm, spaceName, maxConfigCacheAgeSecs),
  m_spaceStats(spaceName, mgm, m_config),
  m_nbStagerrms(0)
{
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
TapeGc::~TapeGc()
{
  try {
    // m_enabled is an std::atomic and is set within enable() after m_worker
    if(m_enabled && m_worker) {
      m_stop.setToTrue();
      m_worker->join();
    }
  } catch(std::exception &ex) {
    eos_static_err("msg=\"%s\"", ex.what());
  } catch(...) {
    eos_static_err("msg=\"Caught an unknown exception\"");
  }
}

//------------------------------------------------------------------------------
// Enable the GC
//------------------------------------------------------------------------------
void
TapeGc::enable() noexcept
{
  try {
    // Do nothing if the calling thread is not the first to call eable()
    if (m_enabledMethodCalled.test_and_set()) return;

    m_enabled = true;

    std::function<void()> entryPoint = std::bind(&TapeGc::workerThreadEntryPoint, this);
    m_worker = std::make_unique<std::thread>(entryPoint);
  } catch(std::exception &ex) {
    eos_static_err("msg=\"%s\"", ex.what());
  } catch(...) {
    eos_static_err("msg=\"Caught an unknown exception\"");
  }
}

//------------------------------------------------------------------------------
// Entry point for the GC worker thread
//------------------------------------------------------------------------------
void
TapeGc::workerThreadEntryPoint() noexcept
{
  do {
    while(!m_stop && tryToGarbageCollectASingleFile()) {
    }
  } while(!m_stop.waitForTrue(std::chrono::seconds(5)));
}

//------------------------------------------------------------------------------
// Notify GC the specified file has been opened
//------------------------------------------------------------------------------
void
TapeGc::fileOpened(const std::string &path, const IFileMD::id_t fid) noexcept
{
  if(!m_enabled) return;

  try {
    const std::string preamble = createLogPreamble(m_spaceName, path, fid);
    eos_static_debug(preamble.c_str());

    std::lock_guard<std::mutex> lruQueueLock(m_lruQueueMutex);
    const bool exceededBefore = m_lruQueue.maxQueueSizeExceeded();
    m_lruQueue.fileAccessed(fid);

    // Only log crossing the max queue size threshold - don't log each access
    if(!exceededBefore && m_lruQueue.maxQueueSizeExceeded()) {
      eos_static_warning("%s msg=\"Tape aware max queue size has been passed - "
        "new files will be ignored\"", preamble.c_str());
    }
  } catch(std::exception &ex) {
    eos_static_err("msg=\"%s\"", ex.what());
  } catch(...) {
    eos_static_err("msg=\"Caught an unknown exception\"");
  }
}

//------------------------------------------------------------------------------
// Try to garage collect a single file if necessary and possible
//------------------------------------------------------------------------------
bool
TapeGc::tryToGarbageCollectASingleFile() noexcept
{
  try {
    const auto config = m_config.get();

    try {
      const auto spaceStats = m_spaceStats.get();

      // Return no file was garbage collected if there is still enough free
      // space or if there is too little data available
      if(spaceStats.freeBytes >= config.minFreeBytes || spaceStats.usedBytes <= config.minUsedBytes) return false;
    } catch(SpaceNotFound &) {
      // Return no file was garbage collected if the space was not found
      return false;
    }

    IFileMD::id_t fid;

    {
      std::lock_guard<std::mutex> lruQueueLock(m_lruQueueMutex);
      if (m_lruQueue.empty()) return false; // No file was garbage collected
      fid = m_lruQueue.getAndPopFidOfLeastUsedFile();
    }

    const std::uint64_t fileToBeDeletedSizeBytes = m_mgm.getFileSizeBytes(fid);

    std::ostringstream preamble;
    preamble << "fxid=" << std::hex << fid;

    try {
      m_mgm.stagerrmAsRoot(fid);
    } catch(std::exception &ex) {
      {
        std::ostringstream msg;
        msg << preamble.str() << " msg=\"Unable to stagerrm file at this time: "
          << ex.what() << "\"";
        eos_static_info(msg.str().c_str());
      }

      if(m_mgm.fileInNamespaceAndNotScheduledForDeletion(fid)) {
        {
          std::ostringstream msg;
          msg << preamble.str() << " msg=\"Putting file back in GC queue"
                                   " because it is still in the namespace\"";
          eos_static_info(msg.str().c_str());
        }

        std::lock_guard<std::mutex> lruQueueLock(m_lruQueueMutex);
        m_lruQueue.fileAccessed(fid);
        return false; // No file was garbage collected
      } else {
        // Please note that a file is considered successfully garbage collected
        // if it does not exists in the EOS namespace when it is popped from the
        // LRU data structure.
        return true;
      }
    }

    fileQueuedForDeletion(fileToBeDeletedSizeBytes);
    std::ostringstream msg;
    msg << preamble.str() << " msg=\"Garbage collected file using stagerrm\"";
    eos_static_info(msg.str().c_str());

    m_nbStagerrms++;

    return true; // A file was garbage collected
  } catch(std::exception &ex) {
    eos_static_err("msg=\"%s\"", ex.what());
  } catch(...) {
    eos_static_err("msg=\"Caught an unknown exception\"");
  }

  return false; // No file was garbage collected
}

//----------------------------------------------------------------------------
// Return the preamble to be placed at the beginning of every log message
//----------------------------------------------------------------------------
std::string
TapeGc::createLogPreamble(const std::string &space, const std::string &path,
  const IFileMD::id_t fid)
{
  std::ostringstream preamble;

  preamble << "space=\"" << space << "\" fxid=" << std::hex << fid <<
    " path=\"" << path << "\"";

  return preamble.str();
}

//----------------------------------------------------------------------------
// Return statistics
//----------------------------------------------------------------------------
TapeGcStats
TapeGc::getStats() const noexcept
{
  TapeGcStats tgcStats;

  tgcStats.nbStagerrms = m_nbStagerrms;
  tgcStats.lruQueueSize = getLruQueueSize();
  tgcStats.spaceStats = m_spaceStats.get();
  tgcStats.queryTimestamp = m_spaceStats.getQueryTimestamp();

  return tgcStats;
}

//----------------------------------------------------------------------------
// Return the size of the LRUE queue
//----------------------------------------------------------------------------
Lru::FidQueue::size_type
TapeGc::getLruQueueSize() const noexcept
{
  const char *const msgFormat =
    "TapeGc::getLruQueueSize() failed space=%s: %s";
  try {
    std::lock_guard<std::mutex> lruQueueLock(m_lruQueueMutex);
    return m_lruQueue.size();
  } catch(std::exception &ex) {
    eos_static_err(msgFormat, m_spaceName.c_str(), ex.what());
  } catch(...) {
    eos_static_err(msgFormat, m_spaceName.c_str(), "Caught an unknown exception");
  }

  return 0;
}

//----------------------------------------------------------------------------
// Enabling this garbage collector without starting the worker thread
//----------------------------------------------------------------------------
void
TapeGc::enableWithoutStartingWorkerThread() {
  // Do nothing if the calling thread is not the first to call enable()
  if (m_enabledMethodCalled.test_and_set()) return;

  m_enabled = true;
}

//------------------------------------------------------------------------------
// Notify this object that a file has been queued for deletion
//------------------------------------------------------------------------------
void
TapeGc::fileQueuedForDeletion(const size_t deletedFileSize)
{
  m_spaceStats.fileQueuedForDeletion(deletedFileSize);
}

EOSTGCNAMESPACE_END
