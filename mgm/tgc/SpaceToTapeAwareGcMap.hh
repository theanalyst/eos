// ----------------------------------------------------------------------
// File: SpaceToTapeAwareGcMap.hh
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

#ifndef __EOSMGM_SPACETOTAPEAWAREGCMAP_HH__
#define __EOSMGM_SPACETOTAPEAWAREGCMAP_HH__

#include "mgm/tgc/TapeAwareGc.hh"

#include <map>

/*----------------------------------------------------------------------------*/
/**
 * @file SpaceToTapeAwareGcMap.hh
 *
 * @brief Class implementing a thread safe map from EOS space name to tape aware
 * garbage collector
 *
 */
/*----------------------------------------------------------------------------*/
EOSMGMNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Class implementing a thread safe map from EOS space name to tape aware
//! garbage collector
//------------------------------------------------------------------------------
class SpaceToTapeAwareGcMap {
public:

  //----------------------------------------------------------------------------
  //! Constructor.
  //----------------------------------------------------------------------------
  SpaceToTapeAwareGcMap();

  //----------------------------------------------------------------------------
  //! Deletion of copy constructor.
  //----------------------------------------------------------------------------
  SpaceToTapeAwareGcMap(const SpaceToTapeAwareGcMap &) = delete;

  //----------------------------------------------------------------------------
  //! Deletion of move constructor.
  //----------------------------------------------------------------------------
  SpaceToTapeAwareGcMap(const SpaceToTapeAwareGcMap &&) = delete;

  //----------------------------------------------------------------------------
  //! Exception thrown when a tape aware garbage collector already exists.
  //----------------------------------------------------------------------------
  struct GcAlreadyExists: public std::runtime_error {
    GcAlreadyExists(const std::string &msg): std::runtime_error(msg) {}
  };

  //----------------------------------------------------------------------------
  //! Create a tape aware garbage collector for the specified EOS space.
  //!
  //! @param space The name of the EOS space.
  //! @throw GcAlreadyExists If a tape aware garbage collector altready exists
  //! for the specified EOS space.
  //----------------------------------------------------------------------------
  void createGc(const std::string &space);

  //----------------------------------------------------------------------------
  //! Exception thrown when an unknown EOS space is encountered.
  //----------------------------------------------------------------------------
  struct UnknownEOSSpace: public std::runtime_error {
    UnknownEOSSpace(const std::string &msg): std::runtime_error(msg) {}
  };

  //----------------------------------------------------------------------------
  //! Returns the garbage collector associated with the specified EOS space.
  //!
  //! @param space The name of the EOS space.
  //! @return The tape aware garbage collector associated with the specified EOS
  //! space.
  //! @throw UnknownEOSSpace If the specified EOS space is unknown.
  //----------------------------------------------------------------------------
  TapeAwareGc &getGc(const std::string &space);

private:

  //--------------------------------------------------------------------------
  //! Mutex protecting the map
  //--------------------------------------------------------------------------
  mutable std::mutex m_mutex;

  //----------------------------------------------------------------------------
  //! Map from space name to tape aware garbage collector
  //----------------------------------------------------------------------------
  std::map<std::string, std::unique_ptr<TapeAwareGc> > m_gcs;
};

EOSMGMNAMESPACE_END

#endif
