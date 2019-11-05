// ----------------------------------------------------------------------
// File: TapeAwareMultiSpaceGc.hh
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

#ifndef __EOSMGM_MULTISPACETAPEGC_HH__
#define __EOSMGM_MULTISPACETAPEGC_HH__

#include "common/Logging.hh"
#include "mgm/Namespace.hh"
#include "mgm/tgc/TapeAwareGcFreeSpace.hh"
#include "mgm/tgc/TapeAwareGcLru.hh"
#include "mgm/tgc/TapeAwareGcThreadSafeCachedValue.hh"
#include "namespace/interface/IFileMD.hh"
#include "proto/ConsoleReply.pb.h"
#include "proto/ConsoleRequest.pb.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <stdint.h>
#include <thread>
#include <time.h>

/*----------------------------------------------------------------------------*/
/**
 * @file TapeAwareMultiSpaceGc.hh
 *
 * @brief Class implementing a tape aware garbage collector
 *
 */
/*----------------------------------------------------------------------------*/
EOSMGMNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! A tape aware garbage collector that can work over multiple EOS spaces
//------------------------------------------------------------------------------
class TapeAwareMultiSpaceGc
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //----------------------------------------------------------------------------
  TapeAwareMultiSpaceGc();

  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  ~TapeAwareMultiSpaceGc();

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  TapeAwareMultiSpaceGc(const TapeAwareMultiSpaceGc &) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  TapeAwareMultiSpaceGc &operator=(const TapeAwareMultiSpaceGc &) = delete;

  //----------------------------------------------------------------------------
  //! Enable the GC
  //----------------------------------------------------------------------------
  void enable() noexcept;

  //----------------------------------------------------------------------------
  //! Notify GC the specified file has been opened
  //! @note This method does nothing and returns immediately if the GC has not
  //! been enabled
  //!
  //! @param space the name of the EOS space where the file resides
  //! @param path file path
  //! @param fmd file metadata
  //----------------------------------------------------------------------------
  void fileOpened(const std::string &space, const std::string &path,
    const IFileMD &fmd) noexcept;

  //----------------------------------------------------------------------------
  //! Notify GC a replica of the specified file has been committed
  //! @note This method does nothing and returns immediately if the GC has not
  //! been enabled
  //!
  //! @param space the name of the EOS space where the file resides
  //! @param path file path
  //! @param fmd file metadata
  //----------------------------------------------------------------------------
  void fileReplicaCommitted(const std::string &space, const std::string &path,
    const IFileMD &fmd) noexcept;

  //----------------------------------------------------------------------------
  //! @return the number of files successfully stagerrm'ed since boot for the
  //! specified EOS space
  //!
  //! @param space the name of the EOS space
  //----------------------------------------------------------------------------
  uint64_t getNbStagerrms(const std::string &space) const;

  //----------------------------------------------------------------------------
  //! @return the size of the LRU queue for the specified EOS space
  //!
  //! @param space the name of the EOS space
  //----------------------------------------------------------------------------
  TapeAwareGcLru::FidQueue::size_type getLruQueueSize(const std::string &space);

  //----------------------------------------------------------------------------
  //! @return the amount of free bytes in the specified EOS space
  //!
  //! @param space the name of the EOS space
  //----------------------------------------------------------------------------
  uint64_t getSpaceFreeBytes(const std::string &space);

  //----------------------------------------------------------------------------
  //! @return the timestamp at which the specified EOS space was queried for
  //! free space
  //!
  //! @param space the name of the EOS space
  //----------------------------------------------------------------------------
  time_t getSpaceFreeSpaceQueryTimestamp(const std::string &space);

private:

  //----------------------------------------------------------------------------
  //! Map from EOS space name to tape aware garbage collector
  //----------------------------------------------------------------------------
  SpaceToTapeAwareGc m_gcs;
};

EOSMGMNAMESPACE_END

#endif
