// ----------------------------------------------------------------------
// File: GeoBalancer.cc
// Author: Joaquim Rocha - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2013 CERN/Switzerland                                  *
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

#include "mgm/GeoBalancer.hh"
#include "mgm/XrdMgmOfs.hh"
#include "mgm/FsView.hh"
#include "namespace/interface/IFsView.hh"
#include "namespace/interface/IView.hh"
#include "namespace/Prefetcher.hh"
#include "common/StringConversion.hh"
#include "common/FileId.hh"
#include "common/LayoutId.hh"
#include "XrdSys/XrdSysError.hh"
#include "XrdOuc/XrdOucTrace.hh"
#include "Xrd/XrdScheduler.hh"
#include <random>
#include <cmath>

extern XrdSysError gMgmOfsEroute;
extern XrdOucTrace gMgmOfsTrace;

#define CACHE_LIFE_TIME 300 // seconds

/*----------------------------------------------------------------------------*/
EOSMGMNAMESPACE_BEGIN

/*----------------------------------------------------------------------------*/
GeoBalancer::GeoBalancer(const char* spacename)
  : mThreshold(.5),
    mAvgUsedSize(0)
    /*----------------------------------------------------------------------------*/
    /**
     * @brief Constructor by space name
     *
     * @param spacename name of the associated space
     */
    /*----------------------------------------------------------------------------*/
{
  mSpaceName = spacename;
  mLastCheck = 0;
  mThread.reset(&GeoBalancer::GeoBalance, this);
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::Stop()
/*----------------------------------------------------------------------------*/
/**
 * @brief thread stop function
 */
/*----------------------------------------------------------------------------*/
{
  mThread.join();
}

/*----------------------------------------------------------------------------*/
GeoBalancer::~GeoBalancer()
/*----------------------------------------------------------------------------*/
/**
 * @brief Destructor
 */
/*----------------------------------------------------------------------------*/
{
  Stop();
  clearCachedSizes();
}

/*----------------------------------------------------------------------------*/
GeotagSize::GeotagSize(uint64_t usedBytes, uint64_t capacity)
  : mSize(usedBytes),
    mCapacity(capacity)
    /*----------------------------------------------------------------------------*/
    /**
     * @brief GeotagSize constructor (capacity must be > 0)
     */
    /*----------------------------------------------------------------------------*/
{
  assert(capacity > 0);
}

/*----------------------------------------------------------------------------*/
int
GeoBalancer::getRandom(int max)
/*----------------------------------------------------------------------------*/
/**
 * @brief Gets a random int between 0 and a given maximum
 * @param max the upper bound of the range within which the int will be
 *        generated
 */
/*----------------------------------------------------------------------------*/
{
  return (int) round(max * random() / (double) RAND_MAX);
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::clearCachedSizes()
/*----------------------------------------------------------------------------*/
/**
 * @brief Clears the cache structures
 */
/*----------------------------------------------------------------------------*/
{
  mGeotagFs.clear();
  mFsGeotag.clear();
  std::map<std::string, GeotagSize*>::iterator it;

  for (it = mGeotagSizes.begin(); it != mGeotagSizes.end(); it++) {
    delete(*it).second;
  }

  mGeotagSizes.clear();
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::fillGeotagsByAvg()
/*----------------------------------------------------------------------------*/
/**
 * @brief Fills mGeotagsOverAvg with the objects in mGeotagSizes, in case
 *        they're greater than the current mAvgUsedSize
 */
/*----------------------------------------------------------------------------*/
{
  mGeotagsOverAvg.clear();
  std::map<std::string, GeotagSize*>::const_iterator it;

  for (it = mGeotagSizes.cbegin(); it != mGeotagSizes.cend(); it++) {
    double geotagAvg = (*it).second->filled();

    if (geotagAvg - mAvgUsedSize > mThreshold) {
      mGeotagsOverAvg.push_back((*it).first);
    }
  }
}

static void
printSizes(const std::map<std::string, GeotagSize*>* sizes)
{
  std::map<std::string, GeotagSize*>::const_iterator it;

  for (it = sizes->cbegin(); it != sizes->cend(); it++)
    eos_static_info("geotag=%s average=%.02f", (*it).first.c_str(),
                    (double)(*it).second->filled() * 100.0);
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::populateGeotagsInfo()
/*----------------------------------------------------------------------------*/
/**
 * @brief Fills mGeotagSizes, calculates the mAvgUsedSize and fills
 *        mGeotagsOverAvg
 */
/*----------------------------------------------------------------------------*/
{
  clearCachedSizes();
  const char* spaceName = mSpaceName.c_str();
  eos::common::RWMutexReadLock lock(FsView::gFsView.ViewMutex);
  const FsSpace* spaceView = FsView::gFsView.mSpaceView[spaceName];

  if (spaceView->size() == 0) {
    eos_static_info("No filesystems in space=%s", spaceName);
    return;
  }

  //std::set<eos::common::FileSystem::fsid_t>::const_iterator it;
  eos::mgm::BaseView::const_iterator it;

  for (it = spaceView->cbegin(); it != spaceView->cend(); it++) {
    FileSystem* fs = FsView::gFsView.mIdView.lookupByID(*it);

    if (!fs || fs->GetActiveStatus() != eos::common::ActiveStatus::kOnline) {
      continue;
    }

    eos::common::FileSystem::fs_snapshot_t snapshot;
    fs->SnapShotFileSystem(snapshot, false);

    if (snapshot.mStatus != eos::common::BootStatus::kBooted ||
        snapshot.mConfigStatus < eos::common::ConfigStatus::kRO ||
        snapshot.mGeoTag.empty()) {
      continue;
    }

    mGeotagFs[snapshot.mGeoTag].push_back(*it);
    mFsGeotag[*it] = snapshot.mGeoTag;
    uint64_t capacity = snapshot.mDiskCapacity;
    uint64_t usedBytes = (uint64_t)(capacity - snapshot.mDiskFreeBytes);

    if (mGeotagSizes.count(snapshot.mGeoTag) == 0) {
      mGeotagSizes[snapshot.mGeoTag] = new GeotagSize(usedBytes, capacity);
    } else {
      uint64_t currentUsedBytes = mGeotagSizes[snapshot.mGeoTag]->usedBytes();
      uint64_t currentCapacity = mGeotagSizes[snapshot.mGeoTag]->capacity();
      mGeotagSizes[snapshot.mGeoTag]->setUsedBytes(currentUsedBytes + usedBytes);
      mGeotagSizes[snapshot.mGeoTag]->setCapacity(currentCapacity + capacity);
    }
  }

  mAvgUsedSize = 0;
  std::map<std::string, std::vector<eos::common::FileSystem::fsid_t>>::const_iterator
      git;

  for (git = mGeotagFs.cbegin(); git != mGeotagFs.cend(); git++) {
    const std::string geotag = (*git).first;
    const std::vector<eos::common::FileSystem::fsid_t> fsVector = (*git).second;
    mAvgUsedSize += mGeotagSizes[geotag]->filled();
  }

  mAvgUsedSize /= ((double) mGeotagSizes.size());
  eos_static_info("New average calculated: average=%.02f %%",
                  mAvgUsedSize * 100.0);
  fillGeotagsByAvg();
}

/*----------------------------------------------------------------------------*/
bool
GeoBalancer::fileIsInDifferentLocations(const eos::IFileMD* fmd)
/*----------------------------------------------------------------------------*/
/**
 * @brief Checks if a file is spread in more than one location
 * @param fmd the file metadata object
 * @return whether the file is in more than one location or not
 */
/*----------------------------------------------------------------------------*/
{
  const std::string* geotag = 0;
  eos::IFileMD::LocationVector::const_iterator lociter;
  eos::IFileMD::LocationVector loc_vect = fmd->getLocations();

  for (lociter = loc_vect.begin(); lociter != loc_vect.end(); ++lociter) {
    // ignore filesystem id 0
    if (!(*lociter)) {
      eos_static_err("msg=\"fsid 0 found\" fxid=%08llx", fmd->getId());
      continue;
    }

    if (geotag == 0) {
      geotag = &mFsGeotag[*lociter];
    } else if (geotag->compare(mFsGeotag[*lociter]) != 0) {
      return true;
    }
  }

  return false;
}

/*----------------------------------------------------------------------------*/
std::string
GeoBalancer::getFileProcTransferNameAndSize(eos::common::FileId::fileid_t fid,
    uint64_t* size)
/*----------------------------------------------------------------------------*/
/**
 * @brief Produces a file conversion path to be placed in the proc directory
 *        and also returns its size
 * @param fid the file ID
 * @param size return address for the size of the file
 * @return the file path
 */
/*----------------------------------------------------------------------------*/
{
  char fileName[1024];
  std::shared_ptr<eos::IFileMD> fmd;
  eos::common::LayoutId::layoutid_t layoutid = 0;
  eos::common::FileId::fileid_t fileid = 0;
  {
    eos::Prefetcher::prefetchFileMDWithParentsAndWait(gOFS->eosView, fid);
    eos::common::RWMutexReadLock lock(gOFS->eosViewRWMutex, __FUNCTION__, __LINE__,
                                      __FILE__);

    try {
      fmd = gOFS->eosFileService->getFileMD(fid);
      layoutid = fmd->getLayoutId();
      fileid = fmd->getId();

      if (fmd->getContainerId() == 0) {
        return std::string("");
      }

      if (fmd->getSize() == 0) {
        return std::string("");
      }

      if (fmd->getNumLocation() == 0) {
        return std::string("");
      }

      if (fileIsInDifferentLocations(fmd.get())) {
        eos_static_debug("msg=\"filename=%s fxid=%08llx is already in more than "
                         "one location\"", fmd->getName().c_str(), fileid);
        return std::string("");
      }

      if (size) {
        *size = fmd->getSize();
      }
    } catch (eos::MDException& e) {
      eos_static_debug("msg=\"exception\" ec=%d emsg=\"%s\"", e.getErrno(),
                       e.getMessage().str().c_str());
      return std::string("");
    }

    XrdOucString fileURI = gOFS->eosView->getUri(fmd.get()).c_str();

    if (fileURI.beginswith(gOFS->MgmProcPath.c_str())) {
      // don't touch files in any ../proc/ directory
      return std::string("");
    }

    eos_static_debug("msg=\"found file for transfering\" file=%s",
                     fileURI.c_str());
  }
  snprintf(fileName, 1024, "%s/%016llx:%s#%08lx",
           gOFS->MgmProcConversionPath.c_str(), fileid, mSpaceName.c_str(),
           (unsigned long) layoutid);
  return std::string(fileName);
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::updateTransferList()
/*----------------------------------------------------------------------------*/
/**
 * @brief For each entry in mTransfers, checks if the files' paths exist, if
 *        they don't, they are deleted from the mTransfers
 */
/*----------------------------------------------------------------------------*/
{
  std::map<eos::common::FileId::fileid_t, std::string>::iterator it;

  for (it = mTransfers.begin(); it != mTransfers.end();) {
    eos::common::VirtualIdentity rootvid = eos::common::VirtualIdentity::Root();
    XrdOucErrInfo error;
    const std::string& fileName = (*it).second;
    struct stat buf;

    if (gOFS->_stat(fileName.c_str(), &buf, error, rootvid, "")) {
      mTransfers.erase(it++);
    } else {
      ++it;
    }
  }

  eos_static_info("scheduledtransfers=%d", mTransfers.size());
}

/*----------------------------------------------------------------------------*/
bool
GeoBalancer::scheduleTransfer(eos::common::FileId::fileid_t fid,
                              const std::string& fromGeotag)
/*----------------------------------------------------------------------------*/
/**
 * @brief Creates the conversion file in proc for the file ID, from the given
 *        fromGeotag (updates the cache structures)
 * @param fid the id of the file to be transferred
 * @param fromGeotag the geotag of the location where the file is located
 * @return whether the transfer file was successfully created or not
 */
/*----------------------------------------------------------------------------*/
{
  eos::common::VirtualIdentity rootvid = eos::common::VirtualIdentity::Root();
  XrdOucErrInfo mError;
  uint64_t size = 0;
  std::string fileName = getFileProcTransferNameAndSize(fid, &size);

  if (fileName == "") {
    return false;
  }

  if (!gOFS->_touch(fileName.c_str(), mError, rootvid, 0)) {
    eos_static_info("scheduledfile=%s", fileName.c_str());
  } else {
    eos_static_err("msg=\"failed to schedule transfer\" schedulingfile=\"%s\"",
                   fileName.c_str());
  }

  mTransfers[fid] = fileName.c_str();
  uint64_t usedBytes = mGeotagSizes[fromGeotag]->usedBytes();
  mGeotagSizes[fromGeotag]->setUsedBytes(usedBytes - size);
  fillGeotagsByAvg();
  return true;
}

/*----------------------------------------------------------------------------*/
eos::common::FileId::fileid_t
GeoBalancer::chooseFidFromGeotag(const std::string& geotag)
/*----------------------------------------------------------------------------*/
/**
 * @brief Chooses a random file ID from a random filesystem in the given geotag
 * @param geotag the location's name from which the file id will be chosen
 * @return the chosen file ID
 */
/*----------------------------------------------------------------------------*/
{
  int rndIndex;
  bool found = false;
  uint64_t fsid_size = 0ull;
  eos::common::FileSystem::fsid_t fsid = 0;
  eos::common::RWMutexReadLock vlock(FsView::gFsView.ViewMutex);
  eos::common::RWMutexReadLock lock(gOFS->eosViewRWMutex, __FUNCTION__, __LINE__,
                                    __FILE__);
  std::vector<eos::common::FileSystem::fsid_t>& validFs = mGeotagFs[geotag];
  // TODO(gbitzes): Add prefetching here.

  while (validFs.size() > 0) {
    rndIndex = getRandom(validFs.size() - 1);
    fsid = validFs[rndIndex];
    fsid_size = gOFS->eosFsView->getNumFilesOnFs(fsid);

    if (fsid_size) {
      found = true;
      break;
    }

    validFs.erase(validFs.begin() + rndIndex);
  }

  if (validFs.size() == 0) {
    mGeotagFs.erase(geotag);
    mGeotagSizes.erase(geotag);
    fillGeotagsByAvg();
  }

  if (!found) {
    return -1;
  }

  int attempts = 10;

  while (attempts-- > 0) {
    eos::IFileMD::id_t randomPick;

    if (gOFS->eosFsView->getApproximatelyRandomFileInFs(fsid, randomPick) &&
        mTransfers.count(randomPick) == 0) {
      return randomPick;
    }
  }

  return -1;
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::prepareTransfer()
/*----------------------------------------------------------------------------*/
/**
 * @brief Picks a geotag randomly and schedule a file ID to be transferred
 */
/*----------------------------------------------------------------------------*/
{
  if (mGeotagsOverAvg.size() == 0) {
    eos_static_debug("No geotags over the average!");
    return;
  }

  int attempts = 10;

  while (attempts-- > 0) {
    int rndIndex = getRandom(mGeotagsOverAvg.size() - 1);
    std::vector<std::string>::const_iterator over_it = mGeotagsOverAvg.cbegin();
    std::advance(over_it, rndIndex);
    // TODO: this loop should be improved not to request the file list too
    // many times in a tight loop
    eos::common::FileId::fileid_t fid = chooseFidFromGeotag(*over_it);

    if ((int) fid == -1) {
      eos_static_debug("Couldn't choose any FID to schedule: failedgeotag=%s",
                       (*over_it).c_str());
      continue;
    }

    if (scheduleTransfer(fid, *over_it)) {
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
bool
GeoBalancer::cacheExpired()
/*----------------------------------------------------------------------------*/
/**
 * @brief Check if the sizes cache should be updated (based on the time passed
 *        since they were last updated)
 * @return whether the cache expired or not
 */
/*----------------------------------------------------------------------------*/
{
  time_t currentTime = time(NULL);

  if (difftime(currentTime, mLastCheck) > CACHE_LIFE_TIME) {
    mLastCheck = currentTime;
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/
void
GeoBalancer::prepareTransfers(int nrTransfers)
/*--------------------------------------------------------------------------*/
/**
 * @brief Schedule a pre-defined number of transfers
 */
/*--------------------------------------------------------------------------*/
{
  int allowedTransfers = nrTransfers - mTransfers.size();

  for (int i = 0; i < allowedTransfers; i++) {
    prepareTransfer();
  }

  if (allowedTransfers > 0) {
    printSizes(&mGeotagSizes);
  }
}

//------------------------------------------------------------------------------
//! @brief eternal loop trying to run conversion jobs
//------------------------------------------------------------------------------
void
GeoBalancer::GeoBalance(ThreadAssistant& assistant) noexcept
{
  eos::common::VirtualIdentity rootvid = eos::common::VirtualIdentity::Root();
  XrdOucErrInfo error;
  gOFS->WaitUntilNamespaceIsBooted(assistant);
  assistant.wait_for(std::chrono::seconds(10));

  // Loop forever until cancelled
  while (!assistant.terminationRequested()) {
    bool isSpaceGeoBalancer = true;
    bool isMaster = true;
    int nrTransfers = 0;
    {
      // Extract the current settings if conversion enabled and how many
      // conversion jobs should run
      uint64_t timeout_ns = 100 * 1e6; // 100ms

      // Try to read lock the mutex
      while (!FsView::gFsView.ViewMutex.TimedRdLock(timeout_ns)) {
        if (assistant.terminationRequested()) {
          return;
        }
      }

      FsSpace* space = FsView::gFsView.mSpaceView[mSpaceName.c_str()];

      if (space->GetConfigMember("converter") != "on") {
        eos_static_debug("Converter is off for! It needs to be on "
                         "for the geotag balancer to work. space=%s",
                         mSpaceName.c_str());
        FsView::gFsView.ViewMutex.UnLockRead();
        goto wait;
      }

      isSpaceGeoBalancer = space->GetConfigMember("geobalancer") == "on";
      nrTransfers = atoi(space->GetConfigMember("geobalancer.ntx").c_str());
      mThreshold =
        atof(space->GetConfigMember("geobalancer.threshold").c_str());
      mThreshold /= 100.0;
      FsView::gFsView.ViewMutex.UnLockRead();
    }
    isMaster = gOFS->mMaster->IsMaster();

    if (isMaster && isSpaceGeoBalancer) {
      eos_static_info("geobalancer is enabled ntx=%d ", nrTransfers);
    } else {
      if (isMaster) {
        eos_static_debug("geotag balancer is disabled");
      } else {
        eos_static_debug("geotag balancer is in slave mode");
      }
    }

    if (isMaster && isSpaceGeoBalancer) {
      updateTransferList();

      if ((int) mTransfers.size() >= nrTransfers) {
        goto wait;
      }

      if (cacheExpired()) {
        populateGeotagsInfo();
        printSizes(&mGeotagSizes);
      }

      prepareTransfers(nrTransfers);
    }

wait:
    // Let some time pass or wait for a notification
    assistant.wait_for(std::chrono::seconds(10));

    if (assistant.terminationRequested()) {
      return;
    }
  }
}

EOSMGMNAMESPACE_END
