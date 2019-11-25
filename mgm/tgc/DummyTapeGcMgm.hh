// ----------------------------------------------------------------------
// File: DummyTapeGcMgm.hh
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

#ifndef __EOSMGMTGC_DUMMYTAPEGCMGM_HH__
#define __EOSMGMTGC_DUMMYTAPEGCMGM_HH__

#include "mgm/tgc/ITapeGcMgm.hh"

#include <map>
#include <mutex>

/*----------------------------------------------------------------------------*/
/**
 * @file DummyTapeGcMgm.hh
 *
 * @brief A dummy implementation of access to the EOS MGM.  The main purpose of
 * this class is to facilitate unit testing.
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! A dummy implementation of access to the EOS MGM.  The main purpose of this
//! class is to facilitate unit testing.
//------------------------------------------------------------------------------
class DummyTapeGcMgm: public ITapeGcMgm {
public:

  //----------------------------------------------------------------------------
  //! Constructor
  //----------------------------------------------------------------------------
  DummyTapeGcMgm();

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  DummyTapeGcMgm(const DummyTapeGcMgm &) = delete;

  //----------------------------------------------------------------------------
  //! Delete move constructor
  //----------------------------------------------------------------------------
  DummyTapeGcMgm(const DummyTapeGcMgm &&) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  DummyTapeGcMgm &operator=(const DummyTapeGcMgm &) = delete;

  //----------------------------------------------------------------------------
  //! @return The minimum number of free bytes the specified space should have
  //! as set in the configuration variables of the space.  If the minimum
  //! number of free bytes cannot be determined for whatever reason then
  //! TGC_DEFAULT_MIN_FREE_BYTES is returned.
  //!
  //! @param spaceName The name of the space
  //----------------------------------------------------------------------------
  uint64_t getSpaceConfigMinFreeBytes(const std::string &spaceName) noexcept override;

  //----------------------------------------------------------------------------
  //! @param fid The file identifier
  //! @return The size of the specified file in bytes.  If the file cannot be
  //! found in the EOS namespace then a file size of 0 is returned.
  //----------------------------------------------------------------------------
  uint64_t getFileSizeBytes(IFileMD::id_t fid) override;

  //----------------------------------------------------------------------------
  //! Determine if the specified file exists and is not scheduled for deletion
  //!
  //! @param fid The file identifier
  //! @return True if the file exists in the EOS namespace and is not scheduled
  //! for deletion
  //----------------------------------------------------------------------------
  bool fileInNamespaceAndNotScheduledForDeletion(IFileMD::id_t fid) override;

  //----------------------------------------------------------------------------
  //! Execute stagerrm as user root
  //!
  //! @param fid The file identifier
  //----------------------------------------------------------------------------
  void stagerrmAsRoot(IFileMD::id_t fid) override;

  //----------------------------------------------------------------------------
  //! Set the minimum number of free bytes for the specified space
  //!
  //! @param space Name of the space.
  //! @param  nbFreeBytes Number of free bytes.
  //----------------------------------------------------------------------------
  void setSpaceConfigMinFreeBytes(const std::string &space,
    uint64_t nbFreeBytes);

  //----------------------------------------------------------------------------
  //! @return number of times getSpaceConfigMinFreeBytes() has been called
  //----------------------------------------------------------------------------
  uint64_t getNbCallsToGetSpaceConfigMinFreeBytes() const;

  //----------------------------------------------------------------------------
  //! @return number of times fileInNamespaceAndNotScheduledForDeletion() has
  //! been called
  //----------------------------------------------------------------------------
  uint64_t getNbCallsToFileInNamespaceAndNotScheduledForDeletion() const;

  //----------------------------------------------------------------------------
  //! @return number of times getFileSizeBytes() has been called
  //----------------------------------------------------------------------------
  uint64_t getNbCallsToGetFileSizeBytes() const;

  //------------------------------------------------------------------------------
  //! @return number of times stagerrmAsRoot() has been called
  //------------------------------------------------------------------------------
  uint64_t getNbCallsToStagerrmAsRoot() const;

private:

  //----------------------------------------------------------------------------
  //! Mutex protecting this dummy object representing access to the MGM
  //----------------------------------------------------------------------------
  mutable std::mutex m_mutex;

  //----------------------------------------------------------------------------
  //! Map from EOS space name to the minimum number of free bytes for that space
  //----------------------------------------------------------------------------
  std::map<std::string, uint64_t> m_spaceToMinFreeBytes;

  //----------------------------------------------------------------------------
  //! Number of times getSpaceConfigMinFreeBytes() has been called
  //----------------------------------------------------------------------------
  uint64_t m_nbCallsToGetSpaceConfigMinFreeBytes;

  //----------------------------------------------------------------------------
  //! Number of times fileInNamespaceAndNotScheduledForDeletion() has been
  //! called
  //----------------------------------------------------------------------------
  uint64_t m_nbCallsToFileInNamespaceAndNotScheduledForDeletion;

  //----------------------------------------------------------------------------
  //! Number of times getFileSizeBytes() has been called
  //----------------------------------------------------------------------------
  uint64_t m_nbCallsToGetFileSizeBytes;

  //----------------------------------------------------------------------------
  //! Number of times stagerrmAsRoot() has been called
  //----------------------------------------------------------------------------
  uint64_t m_nbCallsToStagerrmAsRoot;
};

EOSTGCNAMESPACE_END

#endif
