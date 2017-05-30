//------------------------------------------------------------------------------
// File: sharded-cache.cc
// Author: Georgios Bitzes - CERN
//------------------------------------------------------------------------------

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

#include <gtest/gtest.h>
#include "../ShardedCache.hh"

TEST(ShardedCache, basic_sanity) {
  ShardedCache<int, int, IdentityHash<int>> cache(128, 10);

  for(size_t i = 0; i < 1000; i++) {
    ASSERT_TRUE(cache.store(i, new int(i*2)));
  }

  ASSERT_EQ(cache.retrieve(1005), nullptr);
  ASSERT_EQ(cache.retrieve(1006), nullptr);

  for(size_t i = 0; i < 1000; i++) {
    ASSERT_EQ(*cache.retrieve(i).get(), i*2);
  }

  // sleep for 30ms while keeping #4 alive
  for(size_t i = 0; i < 30; i++) {
    ASSERT_TRUE(cache.retrieve(4) != nullptr);
    usleep(1 * 1000);
  }
  ASSERT_TRUE(cache.retrieve(4) != nullptr);

  // all entries should be evicted by now, except #4
  for(size_t i = 0; i < 1000; i++) {
    if(i == 4) {
      ASSERT_EQ(*cache.retrieve(i).get(), i*2);
    }
    else {
      ASSERT_EQ(cache.retrieve(i), nullptr) << i;
    }
  }

  // now evict #4
  ASSERT_TRUE(cache.invalidate(4));
  ASSERT_EQ(cache.retrieve(4), nullptr);

  // add some entries again, validate they're present
  for(size_t i = 0; i < 1000; i++) {
    ASSERT_TRUE(cache.store(i, new int(i*3)));
    ASSERT_EQ(*cache.retrieve(i).get(), i*3);
  }

  // try to overwrite them with replace set to false, make sure it fails
  for(size_t i = 0; i < 1000; i++) {
    ASSERT_FALSE(cache.store(i, new int(i*4), false));
    ASSERT_EQ(*cache.retrieve(i).get(), i*3);
  }
}
