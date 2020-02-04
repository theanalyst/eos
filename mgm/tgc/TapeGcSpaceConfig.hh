// ----------------------------------------------------------------------
// File: TapeGcSpaceConfig.hh
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

#ifndef __EOSMGMTGC_TAPEGCSPACECONFIG_HH__
#define __EOSMGMTGC_TAPEGCSPACECONFIG_HH__

#include "mgm/Namespace.hh"
#include "mgm/tgc/Constants.hh"

#include <ctime>
#include <stdint.h>

/*----------------------------------------------------------------------------*/
/**
 * @file TapeGcSpaceConfig.hh
 *
 * @brief The configuration of a tape-aware garbage collector for a specific EOS
 * space.
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! The configuration of a tape-aware garbage collector for a specific EOSi
//! space.
//------------------------------------------------------------------------------
struct TapeGcSpaceConfig {
  std::time_t freeBytesQueryPeriodSecs;
  uint64_t minFreeBytes;
  std::time_t usedBytesQueryPeriodSecs;
  uint64_t minUsedBytes;

  TapeGcSpaceConfig():
    freeBytesQueryPeriodSecs(TGC_DEFAULT_FREE_BYTES_QRY_PERIOD_SECS),
    minFreeBytes(TGC_DEFAULT_MIN_FREE_BYTES),
    usedBytesQueryPeriodSecs(TGC_DEFAULT_USED_BYTES_QRY_PERIOD_SECS),
    minUsedBytes(TGC_DEFAULT_MIN_USED_BYTES)
  {
  }

  /*
  bool operator==(const TapeGcSpaceConfig &rhs) const noexcept {
    return
      freeBytesQueryPeriodSecs == rhs.freeBytesQueryPeriodSecs &&
      minFreeBytes == rhs.minFreeBytes &&
      usedBytesQueryPeriodSecs == rhs.usedBytesQueryPeriodSecs &&
      minUsedBytes == rhs.minUsedBytes;
  }

  bool operator!=(const TapeGcSpaceConfig &rhs) const noexcept {
    return !operator==(rhs);
  }
   */
};

EOSTGCNAMESPACE_END

#endif
