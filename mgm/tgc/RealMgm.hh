// ----------------------------------------------------------------------
// File: RealMgm.hh
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

#ifndef __EOSMGMTGC_REALMGM_HH__
#define __EOSMGMTGC_REALMGM_HH__

#include "mgm/Namespace.hh"
#include "mgm/tgc/IMgm.hh"
#include "mgm/XrdMgmOfs.hh"
#include "namespace/interface/IFileMD.hh"

#include <stdint.h>
#include <string>

/*----------------------------------------------------------------------------*/
/**
 * @file RealMgm.hh
 *
 * @brief Implements access to the real EOS MGM
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Implements access to the real EOS MGM
//------------------------------------------------------------------------------
class RealMgm: public IMgm {
public:

  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param ofs The XRootD OFS plugin implementing the metadata handling of EOS
  //----------------------------------------------------------------------------
  RealMgm(XrdMgmOfs &ofs);

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  RealMgm(const RealMgm &) = delete;

  //----------------------------------------------------------------------------
  //! Delete move constructor
  //----------------------------------------------------------------------------
  RealMgm(const RealMgm &&) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  RealMgm &operator=(const RealMgm &) = delete;

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
  //! \param fid The file identifier
  //! \return stagerrm result
  //----------------------------------------------------------------------------
  console::ReplyProto stagerrmAsRoot(const IFileMD::id_t fid) override;

private:

  /// The XRootD OFS plugin implementing the metadata handling of EOS
  XrdMgmOfs &m_ofs;
};

EOSTGCNAMESPACE_END

#endif
