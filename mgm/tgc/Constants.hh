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

#include <cstdint>

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

/// Default maximum age in seconds of a tape-ware garbage collector's
/// cached configuration
const std::uint64_t TGC_DEFAULT_MAX_CONFIG_CACHE_AGE_SECS = 10;

/// Name of a space configuration member
constexpr const char * TGC_NAME_QRY_PERIOD_SECS = "tgc.qryperiodsecs";

/// Default delay in seconds between EOS space queries for the tape-aware GC
const std::uint64_t TGC_DEFAULT_QRY_PERIOD_SECS = 310;

/// Name of a space configuration member
constexpr const char * TGC_NAME_MIN_FREE_BYTES = "tgc.minfreebytes";

/// Default minimum number of free bytes within an EOS space for the tape-aware GC
const std::uint64_t TGC_DEFAULT_MIN_FREE_BYTES = 0;

/// Name of a space configuration member
constexpr const char * TGC_NAME_MIN_USED_BYTES = "tgc.minusedbytes";

/// Default min:w
/// imum number of used bytes with an EOS space for the tape-aware GC
const std::uint64_t TGC_DEFAULT_MIN_USED_BYTES = 1000000000000000000UL; // 1 Exabyte

EOSTGCNAMESPACE_END

#endif
