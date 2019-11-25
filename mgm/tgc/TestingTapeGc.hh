// ----------------------------------------------------------------------
// File: TestingTapeGc.hh
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

#ifndef __EOSMGM_TESTINGTAPEGC_HH__
#define __EOSMGM_TESTINGTAPEGC_HH__

#include "mgm/tgc/TapeGc.hh"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <stdint.h>
#include <thread>
#include <time.h>

/*----------------------------------------------------------------------------*/
/**
 * @file TestingTapeGc.hh
 *
 * @brief Facilitates the unit testing of the TapeGc class
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Facilitates the unit testing of the TapeGc class
//------------------------------------------------------------------------------
class TestingTapeGc: public TapeGc
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param mgm the interface to the EOS MGM
  //! @param space the name of EOS space that this garbage collector will work
  //! on
  //----------------------------------------------------------------------------
  TestingTapeGc(ITapeGcMgm &mgm, const std::string &space,
    const time_t minFreeBytesMaxAgeSecs = 10):
    TapeGc(mgm, space, minFreeBytesMaxAgeSecs)
  {
  }

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  TestingTapeGc(const TapeGc &) = delete;

  //----------------------------------------------------------------------------
  //! Delete move constructor
  //----------------------------------------------------------------------------
  TestingTapeGc(const TapeGc &&) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  TestingTapeGc &operator=(const TapeGc &) = delete;

  //----------------------------------------------------------------------------
  //! Make enableWithoutStartingWorkerThread public so it can be unit tested
  //----------------------------------------------------------------------------
  using TapeGc::enableWithoutStartingWorkerThread;

  //----------------------------------------------------------------------------
  //! Make tryToGarbageCollectASingleFile() public so it can be unit tested
  //----------------------------------------------------------------------------
  using TapeGc::tryToGarbageCollectASingleFile;
};

EOSTGCNAMESPACE_END

#endif
