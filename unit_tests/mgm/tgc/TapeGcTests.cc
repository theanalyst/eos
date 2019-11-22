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
#include <time.h>

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

  DummyTapeGcMgm mgm;
  TapeGc gc(mgm, space);

  const auto now = time(nullptr);
  const auto stats = gc.getStats();

  ASSERT_EQ(0, stats.nbStagerrms);
  ASSERT_EQ(0, stats.lruQueueSize);
  ASSERT_EQ(0, stats.freeBytes);
  ASSERT_TRUE((now - 5) < stats.freeSpaceQueryTimestamp &&
    stats.freeSpaceQueryTimestamp < (now + 5));
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, enable)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";

  DummyTapeGcMgm mgm;
  TapeGc gc(mgm, space);

  gc.enable();
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(TgcTapeGcTest, tryToGarbageCollectASingleFile)
{
  using namespace eos::mgm::tgc;

  const std::string space = "space";

  DummyTapeGcMgm mgm;
  TestingTapeGc gc(mgm, space);
  gc.tryToGarbageCollectASingleFile();
}
