// ----------------------------------------------------------------------
// File: SpaceToTapeGcMap.cc
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

#include "mgm/tgc/SpaceToTapeGcMap.hh"

#include <memory>
#include <sstream>

/*----------------------------------------------------------------------------*/
/**
 * @file SpaceToTapeAwareGcMap.cc
 *
 * @brief Class implementing a thread safe map from EOS space name to tape
 * ware garbage collector
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//----------------------------------------------------------------------------
//! Constructor
//----------------------------------------------------------------------------
SpaceToTapeGcMap::SpaceToTapeGcMap()
{
}

//----------------------------------------------------------------------------
//! Create a tape aware garbage collector for the specified EOS space.
//----------------------------------------------------------------------------
void
SpaceToTapeGcMap::createGc(const std::string &space)
{
  if(space.empty()) {
    std::ostringstream msg;
    msg << "EOS space passed to " << __FUNCTION__ << " is an empty string";
    throw std::runtime_error(msg.str());
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  auto itor = m_gcs.find(space);
  if(m_gcs.end() != itor) {
    std::ostringstream msg;
    msg << "A tape aware garbage collector already exists for EOS space " << space;
    throw GcAlreadyExists(msg.str());
  }

  m_gcs.emplace(space, std::make_unique<TapeGc>());
}

//----------------------------------------------------------------------------
//! Returns the garbage collector associated with the specified EOS space.
//----------------------------------------------------------------------------
TapeGc
&SpaceToTapeGcMap::getGc(const std::string &space) const
{
  if(space.empty()) {
    std::ostringstream msg;
    msg << "EOS space passed to " << __FUNCTION__ << " is an empty string";
    throw std::runtime_error(msg.str());
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  auto itor = m_gcs.find(space);
  if(m_gcs.end() == itor) {
    std::ostringstream msg;
    msg << "EOS space " << space << " is unknown to " << __FUNCTION__;
    throw UnknownEOSSpace(msg.str());
  }

  auto &gc = itor->second;
  if(!gc) {
    std::stringstream msg;
    msg << "Encountered unexpected nullptr to TapeGc for EOS space " << space;
    throw std::runtime_error(msg.str());
  } 

  return *gc;
}

EOSTGCNAMESPACE_END
