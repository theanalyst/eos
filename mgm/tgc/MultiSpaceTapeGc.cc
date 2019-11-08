// ----------------------------------------------------------------------
// File: MultiSpaceTapeGc.cc
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

#include "common/Logging.hh"
#include "mgm/tgc/MultiSpaceTapeGc.hh"

/*----------------------------------------------------------------------------*/
/**
 * @file MultiSpaceTapeGc.cc
 *
 * @brief Class implementing a tape aware garbage collector that can work over
 * multiple EOS spaces
 *
 */
/*----------------------------------------------------------------------------*/
EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
//! Constructor
//------------------------------------------------------------------------------
MultiSpaceTapeGc::MultiSpaceTapeGc()
{
}

//------------------------------------------------------------------------------
//! Destructor
//------------------------------------------------------------------------------
MultiSpaceTapeGc::~MultiSpaceTapeGc() {
}

//------------------------------------------------------------------------------
//! Enable garbage collection for the specified EOS space
//------------------------------------------------------------------------------
void
MultiSpaceTapeGc::enable(const std::string &space) noexcept
{
  const char *const msgFormat =
    "Unable to enable tape-aware garbage collection space=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    gc.enable();
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), "Caught an unknown exception");
  }
}

//------------------------------------------------------------------------------
//! Notify GC the specified file has been opened
//------------------------------------------------------------------------------
void
MultiSpaceTapeGc::fileOpened(const std::string &space, const std::string &path,
  const IFileMD &fmd) noexcept
{
  const char *const msgFormat =
    "Error handling 'file opened' event space=%s fxid=%08llx path=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    gc.fileOpened(path, fmd);
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), fmd.getId(), path.c_str(),
      ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), path.c_str(),
      "Caught an unknown exception");
  }
}

//------------------------------------------------------------------------------
//! Return the number of files successfully stagerrm'ed since boot
//------------------------------------------------------------------------------
uint64_t
MultiSpaceTapeGc::getNbStagerrms(const std::string &space) const noexcept
{
  const char * const msgFormat =
    "Error getting number of stagerrms space=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    return gc.getNbStagerrms();
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), "Caught an unknown exception");
  }

  return 0;
}

//------------------------------------------------------------------------------
//! Return the size of the LRU queue for the specified EOS space
//------------------------------------------------------------------------------
Lru::FidQueue::size_type
MultiSpaceTapeGc::getLruQueueSize(const std::string &space) const noexcept
{
  const char * const msgFormat =
    "Error getting size of the LRU queue space=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    return gc.getLruQueueSize();
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), "Caught an unknown exception");
  }

  return 0;
}

//------------------------------------------------------------------------------
//! Return the amount of free bytes in the specified EOS space
//------------------------------------------------------------------------------
uint64_t
MultiSpaceTapeGc::getFreeBytes(const std::string &space) const noexcept
{
  const char * const msgFormat =
    "Error getting the amount of free bytes in EOS space space=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    return gc.getFreeBytes();
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), "Caught an unknown exception");
  }

  return 0;
}

//------------------------------------------------------------------------------
//! @return the timestamp at which the specified EOS space was queried
//------------------------------------------------------------------------------
time_t
MultiSpaceTapeGc::getFreeSpaceQueryTimestamp(const std::string &space)
const noexcept
{
  const char * const msgFormat = "Error getting the timestamp at which the "
    "specified EOS space was queried space=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    return gc.getFreeSpaceQueryTimestamp();
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), "Caught an unknown exception");
  }

  return 0;
}

EOSTGCNAMESPACE_END
