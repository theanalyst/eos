// ----------------------------------------------------------------------
// File: ITapeGcMgm.hh
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

#ifndef __EOSMGMTGC_ITAPEGCMGM_HH__
#define __EOSMGMTGC_ITAPEGCMGM_HH__

#include "mgm/Namespace.hh"
#include "mgm/tgc/TapeGcSpaceConfig.hh"
#include "namespace/interface/IFileMD.hh"

#include <stdint.h>
#include <string>

/*----------------------------------------------------------------------------*/
/**
 * @file ITapeGcMgm.hh
 *
 * @brief Specifies the tape-aware garbage collector's interface to the EOS MGM
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Specifies the tape-aware garbage collector's interface to the EOS MGM
//------------------------------------------------------------------------------
class ITapeGcMgm {
public:
  //----------------------------------------------------------------------------
  //! Default constructor
  //----------------------------------------------------------------------------
  ITapeGcMgm() = default;

  //----------------------------------------------------------------------------
  //! Virtual destructor
  //----------------------------------------------------------------------------
  virtual ~ITapeGcMgm() = 0;

  //----------------------------------------------------------------------------
  //! @return The configuration of a tape-aware garbage collector for the
  //! specified space.
  //! @param spaceName The name of the space
  //----------------------------------------------------------------------------
  virtual TapeGcSpaceConfig getTapeGcSpaceConfig(const std::string &spaceName) = 0;

  struct FreeAndUsedBytes {
    uint64_t freeBytes;
    uint64_t usedBytes;

    FreeAndUsedBytes(): freeBytes(0), usedBytes(0) {}
  };

  //----------------------------------------------------------------------------
  //! @return The numbers of free and used bytes within the specified space
  //! @param space The name of the EOS space to be queried
  //! @throw TapeAwareGcSpaceNotFound when the EOS space named m_spaceName
  //! cannot be found
  //----------------------------------------------------------------------------
  virtual FreeAndUsedBytes getSpaceFreeAndUsedBytes(const std::string &space) = 0;

  //----------------------------------------------------------------------------
  //! @param fid The file identifier
  //! @return The size of the specified file in bytes.  If the file cannot be
  //! found in the EOS namespace then a file size of 0 is returned.
  //----------------------------------------------------------------------------
  virtual uint64_t getFileSizeBytes(const IFileMD::id_t fid) = 0;

  //----------------------------------------------------------------------------
  //! Determine if the specified file exists and is not scheduled for deletion
  //!
  //! @param fid The file identifier
  //! @return True if the file exists in the EOS namespace and is not scheduled
  //! for deletion
  //----------------------------------------------------------------------------
  virtual bool fileInNamespaceAndNotScheduledForDeletion(const IFileMD::id_t fid) = 0;

  //----------------------------------------------------------------------------
  //! Execute stagerrm as user root
  //!
  //! @param fid The file identifier
  //----------------------------------------------------------------------------
  virtual void stagerrmAsRoot(const IFileMD::id_t fid) = 0;
};

EOSTGCNAMESPACE_END

#endif
