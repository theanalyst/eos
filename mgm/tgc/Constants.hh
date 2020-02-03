// ----------------------------------------------------------------------
// File: Constants.hh
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

#ifndef __EOSMGMTGC_CONSTANTS_HH__
#define __EOSMGMTGC_CONSTANTS_HH__

#include "mgm/Namespace.hh"

#include <stdint.h>

/*----------------------------------------------------------------------------*/
/**
 * @file Constants.hh
 *
 * @brief Constants specific to the implementation of the tape aware garbage
 * collector.
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

/// Default age at which the cached value of queryPeriodSecs for a garbage
/// collector should be renewed
const uint64_t TGC_DEFAULT_QUERY_PERIOD_CACHED_AGE_SECS = 10;

/// Default age at which the cached value of minFreeBytes for a garbage
/// collector should be renewed
const uint64_t TGC_DEFAULT_MIN_FREE_BYTES_CACHE_AGE_SECS = 10;

/// Default delay in seconds between free bytes queries for the tape-aware GC
const uint64_t TGC_DEFAULT_FREE_BYTES_QRY_PERIOD_SECS = 310;

/// Default minimum number of free bytes within an EOS space for the tape-aware GC
const uint64_t TGC_DEFAULT_MIN_FREE_BYTES = 0;

/// Default delay in seconds between used space queries for the tape-aware GC
const uint64_t TGC_DEFAULT_USED_SPACE_QRY_PERIOD_SECS = 310;

/// Default minimum number of used bytes with an EOS space for the tape-aware GC
const uint64_t TGC_DEFAULT_MIN_USED_BYTES = 1000000000000000000UL; // 1 Exabyte

EOSTGCNAMESPACE_END

#endif
