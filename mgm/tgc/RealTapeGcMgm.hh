// ----------------------------------------------------------------------
// File: RealTapeGcMgm.hh
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

#ifndef __EOSMGMTGC_REALTAPEGCMGM_HH__
#define __EOSMGMTGC_REALTAPEGCMGM_HH__

#include "mgm/Namespace.hh"
#include "mgm/tgc/ITapeGcMgm.hh"
#include "mgm/XrdMgmOfs.hh"
#include "namespace/interface/IFileMD.hh"

/*----------------------------------------------------------------------------*/
/**
 * @file RealTapeGcMgm.hh
 *
 * @brief Implements access to the real EOS MGM
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Implements access to the real EOS MGM
//------------------------------------------------------------------------------
class RealTapeGcMgm: public ITapeGcMgm {
public:

  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param ofs The XRootD OFS plugin implementing the metadata handling of EOS
  //----------------------------------------------------------------------------
  RealTapeGcMgm(XrdMgmOfs &ofs);

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  RealTapeGcMgm(const RealTapeGcMgm &) = delete;

  //----------------------------------------------------------------------------
  //! Delete move constructor
  //----------------------------------------------------------------------------
  RealTapeGcMgm(const RealTapeGcMgm &&) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  RealTapeGcMgm &operator=(const RealTapeGcMgm &) = delete;

  //----------------------------------------------------------------------------
  //! @return The delay in seconds between free space queries for the specified
  //! space as set in the configuration variables of the space.  If the delay
  //! cannot be determined for whatever reason then
  //! TGC_DEFAULT_FREE_SPACE_QRY_PERIOD_SECS is returned.
  //!
  //! @param spaceName The name of the space
  //----------------------------------------------------------------------------
  uint64_t getSpaceConfigQryPeriodSecs(const std::string &spaceName) noexcept override;

  //----------------------------------------------------------------------------
  //! @return The minimum number of free bytes the specified space should have
  //! as set in the configuration variables of the space.  If the minimum
  //! number of free bytes cannot be determined for whatever reason then 0 is
  //! returned.
  //!
  //! @param spaceName The name of the space
  //----------------------------------------------------------------------------
  uint64_t getSpaceConfigMinFreeBytes(const std::string &spaceName) noexcept override;

  //----------------------------------------------------------------------------
  //! @return The number of free bytes within the specified space
  //! @param space The name of the EOS space to be queried
  //! @return the amount of free space in bytes
  //! @throw TapeAwareGcSpaceNotFound when the EOS space named m_spaceName
  //! cannot be found
  //----------------------------------------------------------------------------
  uint64_t getSpaceFreeBytes(const std::string &space) override;

  //----------------------------------------------------------------------------
  //! @param fid The file identifier
  //! @return The size of the specified file in bytes.  If the file cannot be
  //! found in the EOS namespace then a file size of 0 is returned.
  //----------------------------------------------------------------------------
  uint64_t getFileSizeBytes(const IFileMD::id_t fid) override;

  //----------------------------------------------------------------------------
  //! Determine if the specified file exists and is not scheduled for deletion
  //!
  //! @param fid The file identifier
  //! @return True if the file exists in the EOS namespace and is not scheduled
  //! for deletion
  //----------------------------------------------------------------------------
  bool fileInNamespaceAndNotScheduledForDeletion(const IFileMD::id_t fid) override;

  //----------------------------------------------------------------------------
  //! Execute stagerrm as user root
  //!
  //! @param fid The file identifier
  //----------------------------------------------------------------------------
  void stagerrmAsRoot(const IFileMD::id_t fid) override;

private:

  /// The XRootD OFS plugin implementing the metadata handling of EOS
  XrdMgmOfs &m_ofs;
};

EOSTGCNAMESPACE_END

#endif
