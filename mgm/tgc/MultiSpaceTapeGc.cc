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
MultiSpaceTapeGc::MultiSpaceTapeGc(ITapeGcMgm &mgm): m_gcs(mgm)
{
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
    auto &gc = m_gcs.createGc(space);
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
  const IFileMD::id_t fid) noexcept
{
  const char *const msgFormat =
    "Error handling 'file opened' event space=%s fxid=%08llx path=%s: %s";

  try {
    auto &gc = m_gcs.getGc(space);
    gc.fileOpened(path, fid);
  } catch (SpaceToTapeGcMap::UnknownEOSSpace&) {
    // Ignore events for EOS spaces that do not have an enabled tape-aware GC
  } catch (std::exception &ex) {
    eos_static_err(msgFormat, space.c_str(), fid, path.c_str(), ex.what());
  } catch (...) {
    eos_static_err(msgFormat, space.c_str(), path.c_str(),
      "Caught an unknown exception");
  }
}

//------------------------------------------------------------------------------
//! Return map from EOS space name to tape-aware GC statistics
//------------------------------------------------------------------------------
std::map<std::string, TapeGcStats> MultiSpaceTapeGc::getStats() const
{
  return m_gcs.getStats();
}

EOSTGCNAMESPACE_END
