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
MultiSpaceTapeGc::MultiSpaceTapeGc() {
}

//------------------------------------------------------------------------------
//! Destructor
//------------------------------------------------------------------------------
MultiSpaceTapeGc::~MultiSpaceTapeGc() {
}

//------------------------------------------------------------------------------
//! Enable garbag collection for the specified EOS space
//------------------------------------------------------------------------------
void MultiSpaceTapeGc::enable(const std::string &space) noexcept {
}

//------------------------------------------------------------------------------
//! Notify GC the specified file has been opened
//------------------------------------------------------------------------------
void MultiSpaceTapeGc::fileOpened(const std::string &space,
  const std::string &path, const IFileMD &fmd) noexcept {
}

//------------------------------------------------------------------------------
//! Notify GC a replica of the specified file has been committed
//------------------------------------------------------------------------------
void MultiSpaceTapeGc::fileReplicaCommitted(const std::string &space,
  const std::string &path, const IFileMD &fmd) noexcept {
}

//------------------------------------------------------------------------------
//! Return the number of files successfully stagerrm'ed since boot
//------------------------------------------------------------------------------
uint64_t MultiSpaceTapeGc::getNbStagerrms(const std::string &space) const {
  return 0;
}

//------------------------------------------------------------------------------
//! Return the size of the LRU queue for the specified EOS space
//------------------------------------------------------------------------------
TapeGcLru::FidQueue::size_type MultiSpaceTapeGc::getLruQueueSize(
  const std::string &space) {
  return 0;
}

//------------------------------------------------------------------------------
//! Return the amount of free bytes in the specified EOS space
//------------------------------------------------------------------------------
uint64_t MultiSpaceTapeGc::getSpaceFreeBytes(const std::string &space) {
  return 0;
}

//------------------------------------------------------------------------------
//! @return the timestamp at which the specified EOS space was queried
//------------------------------------------------------------------------------
time_t MultiSpaceTapeGc::getSpaceFreeSpaceQueryTimestamp(
  const std::string &space) {
  return 0;
}

EOSTGCNAMESPACE_END
