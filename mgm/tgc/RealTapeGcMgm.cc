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

#include "mgm/proc/admin/StagerRmCmd.hh"
#include "mgm/FsView.hh"
#include "mgm/tgc/Constants.hh"
#include "mgm/tgc/RealTapeGcMgm.hh"
#include "mgm/tgc/Utils.hh"
#include "namespace/interface/IFileMDSvc.hh"
#include "namespace/Prefetcher.hh"

EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
RealTapeGcMgm::RealTapeGcMgm(XrdMgmOfs &ofs): m_ofs(ofs) {
}

//------------------------------------------------------------------------------
// Return The delay in seconds between free space queries for the specified
// space as set in the configuration variables of the space.  If the delay
// cannot be determined for whatever reason then
// TGC_DEFAULT_FREE_SPACE_QRY_PERIOD_SECS is returned.
//------------------------------------------------------------------------------
uint64_t
RealTapeGcMgm::getSpaceConfigQryPeriodSecs(const std::string &spaceName) noexcept
{
  try {
    std::string valueStr;
    {
      eos::common::RWMutexReadLock lock(FsView::gFsView.ViewMutex);

      const auto spaceItor = FsView::gFsView.mSpaceView.find(spaceName);
      if (FsView::gFsView.mSpaceView.end() != spaceItor && nullptr != spaceItor->second) {
        const auto &space = *(spaceItor->second);
        valueStr = space.GetConfigMember("tapeawaregc.spacequeryperiodsecs");
      }
    }

    return Utils::toUint64(valueStr);
  } catch(...) {
    return TGC_DEFAULT_FREE_SPACE_QRY_PERIOD_SECS;
  }
}

//------------------------------------------------------------------------------
// Return the minimum number of free bytes the specified space should have
// as set in the configuration variables of the space.  If the minimum
// number of free bytes cannot be determined for whatever reason then
// TGC_DEFAULT_MIN_FREE_BYTES is returned.
//------------------------------------------------------------------------------
uint64_t
RealTapeGcMgm::getSpaceConfigMinFreeBytes(const std::string &spaceName) noexcept
{
  try {
    std::string valueStr;
    {
      eos::common::RWMutexReadLock lock(FsView::gFsView.ViewMutex);
      const auto spaceItor = FsView::gFsView.mSpaceView.find(spaceName);
      if (FsView::gFsView.mSpaceView.end() == spaceItor) return 0;
      if (nullptr == spaceItor->second) return 0;
      const auto &space = *(spaceItor->second);
      valueStr = space.GetConfigMember("tgc.minfreebytes");
    }

    if(valueStr.empty()) {
     return 0;
    } else {
      return Utils::toUint64(valueStr);
    }
  } catch(...) {
    return TGC_DEFAULT_MIN_FREE_BYTES;
  }
}

//----------------------------------------------------------------------------
// Determine if the specified file exists and is not scheduled for deletion
//----------------------------------------------------------------------------
bool RealTapeGcMgm::fileInNamespaceAndNotScheduledForDeletion(const IFileMD::id_t fid) {
  // Prefetch before taking lock because metadata may not be in memory
  Prefetcher::prefetchFileMDAndWait(m_ofs.eosView, fid);
  common::RWMutexReadLock lock(m_ofs.eosViewRWMutex);
  const auto fmd = m_ofs.eosFileService->getFileMD(fid);

  // A file scheduled for deletion has a container ID of 0
  return nullptr != fmd && 0 != fmd->getContainerId();
}

//----------------------------------------------------------------------------
// Return size of the specified file
//----------------------------------------------------------------------------
uint64_t RealTapeGcMgm::getFileSizeBytes(const IFileMD::id_t fid) {
  // Prefetch before taking lock because metadata may not be in memory
  Prefetcher::prefetchFileMDAndWait(m_ofs.eosView, fid);
  common::RWMutexReadLock lock(m_ofs.eosViewRWMutex);
  const auto fmd = m_ofs.eosFileService->getFileMD(fid);

  if(nullptr != fmd) {
    return fmd->getSize();
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------
// Execute stagerrm as user root
//----------------------------------------------------------------------------
void
RealTapeGcMgm::stagerrmAsRoot(const IFileMD::id_t fid)
{
  eos::common::VirtualIdentity rootVid = eos::common::VirtualIdentity::Root();

  eos::console::RequestProto req;
  eos::console::StagerRmProto* stagerRm = req.mutable_stagerrm();
  auto file = stagerRm->add_file();
  file->set_fid(fid);

  StagerRmCmd cmd(std::move(req), rootVid);
  auto const result = cmd.ProcessRequest();
  if(result.retc()) {
    throw std::runtime_error(result.std_err());
  }
}

EOSTGCNAMESPACE_END
