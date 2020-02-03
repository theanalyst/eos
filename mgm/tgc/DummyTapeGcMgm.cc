// ----------------------------------------------------------------------
// File: TapeGc.cc
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

#include "mgm/tgc/Constants.hh"
#include "mgm/tgc/DummyTapeGcMgm.hh"

EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
DummyTapeGcMgm::DummyTapeGcMgm():
m_nbCallsToGetSpaceConfigQryPeriodSecs(0),
m_nbCallsToGetSpaceConfigMinFreeBytes(0),
m_nbCallsToFileInNamespaceAndNotScheduledForDeletion(0),
m_nbCallsToGetFileSizeBytes(0),
m_nbCallsToStagerrmAsRoot(0)
{
}

//------------------------------------------------------------------------------
// Return The delay in seconds between free space queries for the specified
// space as set in the configuration variables of the space.  If the delay
// cannot be determined for whatever reason then
// TGC_DEFAULT_FREE_SPACE_QRY_PERIOD_SECS is returned.
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getSpaceConfigQryPeriodSecs(const std::string &spaceName) noexcept
{
  try {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nbCallsToGetSpaceConfigQryPeriodSecs++;

    auto itor = m_spaceToQryPeriodSecs.find(spaceName);
    if(itor == m_spaceToQryPeriodSecs.end()) {
      return 0;
    } else {
      return itor->second;
    }
  } catch(...) {
    // Do nothing
  }
  return TGC_DEFAULT_FREE_SPACE_QRY_PERIOD_SECS;
}

//------------------------------------------------------------------------------
// Return the minimum number of free bytes the specified space should have
// as set in the configuration variables of the space.  If the minimum
// number of free bytes cannot be determined for whatever reason then
// TGC_DEFAULT_MIN_FREE_BYTES is returned.
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getSpaceConfigMinFreeBytes(const std::string &spaceName) noexcept
{
  try {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nbCallsToGetSpaceConfigMinFreeBytes++;

    auto itor = m_spaceToMinFreeBytes.find(spaceName);
    if(itor == m_spaceToMinFreeBytes.end()) {
      return 0;
    } else {
      return itor->second;
    }
  } catch(...) {
    // Do nothing
  }
  return TGC_DEFAULT_MIN_FREE_BYTES;
}

//----------------------------------------------------------------------------
// Determine if the specified file exists and is not scheduled for deletion
//----------------------------------------------------------------------------
bool
DummyTapeGcMgm::fileInNamespaceAndNotScheduledForDeletion(const IFileMD::id_t /* fid */)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_nbCallsToFileInNamespaceAndNotScheduledForDeletion++;
  return true;
}

//----------------------------------------------------------------------------
// Return numbers of free and used bytes within the specified space
//----------------------------------------------------------------------------
ITapeGcMgm::FreeAndUsedBytes
DummyTapeGcMgm::getSpaceFreeAndUsedBytes(const std::string &space) {
  return FreeAndUsedBytes();
}

//----------------------------------------------------------------------------
// Return size of the specified file
//----------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getFileSizeBytes(const IFileMD::id_t /* fid */)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_nbCallsToGetFileSizeBytes++;
  return 1;
}

//----------------------------------------------------------------------------
// Execute stagerrm as user root
//----------------------------------------------------------------------------
void
DummyTapeGcMgm::stagerrmAsRoot(const IFileMD::id_t /* fid */)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_nbCallsToStagerrmAsRoot++;
}

//----------------------------------------------------------------------------
// Set the period between free space queries for the specified EOS space
//----------------------------------------------------------------------------
void
DummyTapeGcMgm::setSpaceConfigQryPeriod(const std::string &space,
  const time_t qryPeriodSecs) {
  std::lock_guard<std::mutex> lock(m_mutex);

  m_spaceToQryPeriodSecs[space] = qryPeriodSecs;
}

//----------------------------------------------------------------------------
// Set the minimum number of free bytes for the specified space
//----------------------------------------------------------------------------
void
DummyTapeGcMgm::setSpaceConfigMinFreeBytes(const std::string &space,
  const uint64_t nbFreeBytes) {
  std::lock_guard<std::mutex> lock(m_mutex);

  m_spaceToMinFreeBytes[space] = nbFreeBytes;
}

//------------------------------------------------------------------------------
// Return number of times getSpaceConfigMinFreeBytes() has been called
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getNbCallsToGetSpaceConfigMinFreeBytes() const {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_nbCallsToGetSpaceConfigMinFreeBytes;
}

//------------------------------------------------------------------------------
// Return number of times fileInNamespaceAndNotScheduledForDeletion() has been
// called
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getNbCallsToFileInNamespaceAndNotScheduledForDeletion() const {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_nbCallsToFileInNamespaceAndNotScheduledForDeletion;
}

//------------------------------------------------------------------------------
// Return number of times getFileSizeBytes() has been called
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getNbCallsToGetFileSizeBytes() const {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_nbCallsToGetFileSizeBytes;
}

//------------------------------------------------------------------------------
// Return number of times stagerrmAsRoot() has been called
//------------------------------------------------------------------------------
uint64_t
DummyTapeGcMgm::getNbCallsToStagerrmAsRoot() const {
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_nbCallsToStagerrmAsRoot;
}

EOSTGCNAMESPACE_END
