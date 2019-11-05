//------------------------------------------------------------------------------
// File: TapeAwareLruTests.cc
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

#include "mgm/tgc/SpaceToTapeAwareGcMap.hh"

#include <gtest/gtest.h>

class SpaceToTapeAwareGcMapTest : public ::testing::Test {
protected:

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(SpaceToTapeAwareGcMapTest, Constructor)
{
  using namespace eos::mgm;

  const std::string space = "space";
  SpaceToTapeAwareGcMap map();
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(SpaceToTapeAwareGcMapTest, getGc_unknown_eos_space)
{
  using namespace eos::mgm;

  const std::string space = "space";
  SpaceToTapeAwareGcMap map;

  ASSERT_THROW(map.getGc(space), SpaceToTapeAwareGcMap::UnknownEOSSpace);
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(SpaceToTapeAwareGcMapTest, createGc)
{
  using namespace eos::mgm;

  const std::string space = "space";
  SpaceToTapeAwareGcMap map;

  map.createGc(space);

  map.getGc(space);
}

//------------------------------------------------------------------------------
// Test
//------------------------------------------------------------------------------
TEST_F(SpaceToTapeAwareGcMapTest, createGc_already_exists)
{
  using namespace eos::mgm;

  const std::string space = "space";
  SpaceToTapeAwareGcMap map;

  map.createGc(space);

  ASSERT_THROW(map.createGc(space), SpaceToTapeAwareGcMap::GcAlreadyExists);
}
