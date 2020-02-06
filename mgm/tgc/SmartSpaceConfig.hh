// ----------------------------------------------------------------------
// File: SmartSpaceConfig.hh
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

#ifndef __EOSMGM_SMARTSPACECONFIG_HH__
#define __EOSMGM_SMARTSPACECONFIG_HH__

#include "mgm/Namespace.hh"
#include "mgm/tgc/CachedValue.hh"
#include "mgm/tgc/ITapeGcMgm.hh"
#include "mgm/tgc/SpaceConfig.hh"

/*----------------------------------------------------------------------------*/
/**
 * @file SmartSpaceConfig.hh
 *
 * @brief Implements the required caching and logging surrounding the
 * configuration of a tape-ware garbage collector.
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Implements the required caching and logging surrounding the configuration of
//! a tape-ware garbage collector.
//------------------------------------------------------------------------------
class SmartSpaceConfig
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param mgm interface to the EOS MGM
  //! @param spaceName name of the EOS space that this garbage collector will
  //! manage
  //! @param maxConfigCacheAgeSecs maximum age in seconds of a tape-ware garbage
  //! collector's cached configuration
  //----------------------------------------------------------------------------
  SmartSpaceConfig(
    ITapeGcMgm &mgm,
    const std::string &spaceName,
    std::time_t maxConfigCacheAgeSecs);

  //----------------------------------------------------------------------------
  //! Delete copy constructor
  //----------------------------------------------------------------------------
  SmartSpaceConfig(const SmartSpaceConfig &) = delete;

  //----------------------------------------------------------------------------
  //! Delete move constructor
  //----------------------------------------------------------------------------
  SmartSpaceConfig(const SmartSpaceConfig &&) = delete;

  //----------------------------------------------------------------------------
  //! Delete assignment operator
  //----------------------------------------------------------------------------
  SmartSpaceConfig &operator=(const SmartSpaceConfig &) = delete;

  //----------------------------------------------------------------------------
  //! @return the tape-aware garbage collector configuration and log if changed
  //----------------------------------------------------------------------------
  SpaceConfig get() const;

private:

  //----------------------------------------------------------------------------
  //! The cached tape-aware garbage configuration
  //----------------------------------------------------------------------------
  mutable CachedValue<SpaceConfig> m_config;
};

EOSTGCNAMESPACE_END

#endif
