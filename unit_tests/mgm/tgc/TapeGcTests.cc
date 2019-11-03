//------------------------------------------------------------------------------
// File: TapeGcTests.cc
// Author: Steven Murray <smurray at cern dot ch>
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2018 CERN/Switzerland                                  *
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

#include "mgm/tgc/DummyTapeGcMgm.hh"
#include "mgm/tgc/TestingTapeGc.hh"

#include <gtest/gtest.h>
#include <ctime>

class TgcTapeGcTest : public ::testing::Test {
protected:

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, constructor)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";
  const std::time_t queryPeriodCacheAgeSecs = 0; // Always renew cached value
  const std::time_t minFreeBytesCacheAgeSecs = 0; // Always renew cached value

  DummyTapeGcMgm mgm;
  TapeGc gc(mgm, space, queryPeriodCacheAgeSecs, minFreeBytesCacheAgeSecs);

  const auto stats = gc.getStats();

  ASSERT_EQ(0, stats.nbStagerrms);
  ASSERT_EQ(0, stats.lruQueueSize);
  ASSERT_EQ(0, stats.freeBytes);
  ASSERT_EQ(0, stats.freeSpaceQueryTimestamp);
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, enable)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";
  const std::time_t queryPeriodCacheAgeSecs = 0; // Always renew cached value
  const std::time_t minFreeBytesCacheAgeSecs = 0; // Always renew cached value

  DummyTapeGcMgm mgm;
  TapeGc gc(mgm, space, queryPeriodCacheAgeSecs, minFreeBytesCacheAgeSecs);

  gc.enable();
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, enableWithoutStartingWorkerThread)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";
  const std::time_t queryPeriodCacheAgeSecs = 0; // Always renew cached value
  const std::time_t minFreeBytesCacheAgeSecs = 0; // Always renew cached value

  DummyTapeGcMgm mgm;
  TestingTapeGc gc(mgm, space, queryPeriodCacheAgeSecs, minFreeBytesCacheAgeSecs);

  gc.enableWithoutStartingWorkerThread();
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, tryToGarbageCollectASingleFile)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";
  const std::time_t queryPeriodCacheAgeSecs = 0; // Always renew cached value
  const std::time_t minFreeBytesCacheAgeSecs = 0; // Always renew cached value

  DummyTapeGcMgm mgm;
  TestingTapeGc gc(mgm, space, queryPeriodCacheAgeSecs, minFreeBytesCacheAgeSecs);

  gc.enableWithoutStartingWorkerThread();

  ASSERT_EQ(0, mgm.getNbCallsToGetSpaceConfigMinFreeBytes());

  gc.tryToGarbageCollectASingleFile();

  ASSERT_EQ(1, mgm.getNbCallsToGetSpaceConfigMinFreeBytes());
  ASSERT_EQ(0, mgm.getNbCallsToFileInNamespaceAndNotScheduledForDeletion());
  ASSERT_EQ(0, mgm.getNbCallsToGetFileSizeBytes());
  ASSERT_EQ(0, mgm.getNbCallsToStagerrmAsRoot());

  const std::string path = "the_file_path";
  eos::IFileMD::id_t fid = 1;
  gc.fileOpened(path, fid);

  gc.tryToGarbageCollectASingleFile();

  ASSERT_EQ(2, mgm.getNbCallsToGetSpaceConfigMinFreeBytes());
  ASSERT_EQ(0, mgm.getNbCallsToFileInNamespaceAndNotScheduledForDeletion());
  ASSERT_EQ(0, mgm.getNbCallsToGetFileSizeBytes());
  ASSERT_EQ(0, mgm.getNbCallsToStagerrmAsRoot());

  mgm.setSpaceConfigMinFreeBytes(space, 1);

  gc.tryToGarbageCollectASingleFile();

  ASSERT_EQ(3, mgm.getNbCallsToGetSpaceConfigMinFreeBytes());
  ASSERT_EQ(0, mgm.getNbCallsToFileInNamespaceAndNotScheduledForDeletion());
  ASSERT_EQ(1, mgm.getNbCallsToGetFileSizeBytes());
  ASSERT_EQ(1, mgm.getNbCallsToStagerrmAsRoot());
}
