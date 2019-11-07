// ----------------------------------------------------------------------
// File: TapeGc.hh
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

#ifndef __EOSMGM_TAPEGC_HH__
#define __EOSMGM_TAPEGC_HH__

#include "common/Logging.hh"
#include "mgm/Namespace.hh"
#include "mgm/tgc/BlockingFlag.hh"
#include "mgm/tgc/FreeSpace.hh"
#include "mgm/tgc/Lru.hh"
#include "namespace/interface/IFileMD.hh"
#include "proto/ConsoleReply.pb.h"
#include "proto/ConsoleRequest.pb.h"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <stdint.h>
#include <thread>
#include <time.h>

/*----------------------------------------------------------------------------*/
/**
 * @file TapeGc.hh
 *
 * @brief Class implementing a tape aware garbage collector
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! A tape aware garbage collector
//------------------------------------------------------------------------------
class TapeGc
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //----------------------------------------------------------------------------
  TapeGc();

  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  ~TapeGc();

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  TapeGc(const TapeGc &) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  TapeGc &operator=(const TapeGc &) = delete;

  //----------------------------------------------------------------------------
  //! Enable the GC
  //----------------------------------------------------------------------------
  void enable() noexcept;

  //----------------------------------------------------------------------------
  //! Notify GC the specified file has been opened
  //! @note This method does nothing and returns immediately if the GC has not
  //! been enabled
  //!
  //! @param path file path
  //! @param fmd file metadata
  //----------------------------------------------------------------------------
  void fileOpened(const std::string &path, const IFileMD &fmd) noexcept;

  //----------------------------------------------------------------------------
  //! Notify GC a replica of the specified file has been committed
  //! @note This method does nothing and returns immediately if the GC has not
  //! been enabled
  //!
  //! @param path file path
  //! @param fmd file metadata
  //----------------------------------------------------------------------------
  void fileReplicaCommitted(const std::string &path, const IFileMD &fmd)
    noexcept;

  //----------------------------------------------------------------------------
  //! @return the number of files successfully stagerrm'ed since boot.  Zero is
  //! returned in the case of error.
  //----------------------------------------------------------------------------
  uint64_t getNbStagerrms() const noexcept;

  //----------------------------------------------------------------------------
  //! @return the size of the LRU queue.  Zero is returned in the case of error.
  //----------------------------------------------------------------------------
  Lru::FidQueue::size_type getLruQueueSize() const noexcept;

  //----------------------------------------------------------------------------
  //! @return the amount of free bytes in the EOS space worked on by this
  //! garbage collector.  Zero is in the case of error.
  //----------------------------------------------------------------------------
  uint64_t getFreeBytes() const noexcept;

  //----------------------------------------------------------------------------
  //! @return the timestamp at which the EOS space named default was queried for
  //! free space.  Zero is returned in the case of error.
  //----------------------------------------------------------------------------
  time_t getFreeSpaceQueryTimestamp() const noexcept;

protected:

  /// Used to ensure the enable() method only starts the worker thread once
  std::atomic_flag m_enabledMethodCalled = ATOMIC_FLAG_INIT;

  /// True if the GC has been enabled
  std::atomic<bool> m_enabled;

  /// True if the worker thread should stop
  BlockingFlag m_stop;

  /// The one and only GC worker thread
  std::unique_ptr<std::thread> m_worker;

  /// Mutex protecting mLruQueue
  mutable std::mutex m_lruQueueMutex;

  /// Queue of Least Recently Used (LRU) files
  Lru m_lruQueue;

  //----------------------------------------------------------------------------
  //! Entry point for the GC worker thread
  //----------------------------------------------------------------------------
  void workerThreadEntryPoint() noexcept;

  //----------------------------------------------------------------------------
  //! @return The minimum number of free bytes the specified space should have
  //! as set in the configuration variables of the space.  If the minimum
  //! number of free bytes cannot be determined for whatever reason then 0 is
  //! returned.
  //!
  //! @param spaceName The name of the space
  //----------------------------------------------------------------------------
  static uint64_t getSpaceConfigMinFreeBytes(const std::string &spaceName) noexcept;

  //----------------------------------------------------------------------------
  //! Try to garbage collect a single file if necessary and possible.
  //!
  //! Please note that a file is considered successfully garbage collected if
  //! it does not exists in the EOS namespace when it is popped from the LRU
  //! data structure.
  //!
  //! @return True if a file was garbage collected
  //----------------------------------------------------------------------------
  bool tryToGarbageCollectASingleFile() noexcept;

  //----------------------------------------------------------------------------
  //! Execute stagerrm as user root
  //!
  //! \param fid The file identifier
  //! \return stagerrm result
  //----------------------------------------------------------------------------
  console::ReplyProto stagerrmAsRoot(const IFileMD::id_t fid);

  //----------------------------------------------------------------------------
  //! @param fid The file identifier
  //! @return The size of the specified file in bytes.  If the file cannot be
  //! found in the EOS namespace then a file size of 0 is returned.
  //----------------------------------------------------------------------------
  uint64_t getFileSizeBytes(const IFileMD::id_t fid);

  //----------------------------------------------------------------------------
  //! Determine if the specified file exists and is not scheduled for deletion
  //!
  //! @param fid The file identifier
  //! @return True if the file exists in the EOS namespace and is not scheduled
  //! for deletion
  //----------------------------------------------------------------------------
  bool fileInNamespaceAndNotScheduledForDeletion(const IFileMD::id_t fid);

  //----------------------------------------------------------------------------
  //! Return the preamble to be placed at the beginning of every log message
  //!
  //! @param path The file path
  //! @param fid The file identifier
  //----------------------------------------------------------------------------
  static std::string createLogPreamble(const std::string &path,
    const IFileMD::id_t fid);

  /// Thrown when a string is not a valid unsigned 64-bit integer
  struct InvalidUint64: public std::runtime_error {
    InvalidUint64(const std::string &msg): std::runtime_error(msg) {}
  };

  /// Thrown when a string representing a 64-bit integer is out of range
  struct OutOfRangeUint64: public InvalidUint64 {
    OutOfRangeUint64(const std::string &msg): InvalidUint64(msg) {}
  };

  //----------------------------------------------------------------------------
  //! Cached value for the minimum number of free bytes to be available in the
  //! default EOS space.  If the actual number of free bytes is less than this
  //! value then the garbage collector will try to free up space by garbage
  //! collecting disk replicas.
  //----------------------------------------------------------------------------
  CachedValue<uint64_t> m_cachedDefaultSpaceMinFreeBytes;

  //----------------------------------------------------------------------------
  //! Object responsible for determining the number of free bytes in the EOS
  //! space worked on by this garbage collector
  //----------------------------------------------------------------------------
  mutable FreeSpace m_freeSpace;

  //----------------------------------------------------------------------------
  //! Counter that is incremented each time a file is successfully stagerrm'ed
  //----------------------------------------------------------------------------
  std::atomic<uint64_t> m_nbStagerrms;

  //----------------------------------------------------------------------------
  //! @return the configured min free bytes for default space and log if changed
  //----------------------------------------------------------------------------
  uint64_t getDefaultSpaceMinFreeBytesAndLogIfChanged();
};

EOSTGCNAMESPACE_END

#endif
