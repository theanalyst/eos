//------------------------------------------------------------------------------
// File: utils.cc
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
#include "../Utils.hh"

TEST(Utils, read_file) {
  const std::string filename("/tmp/fuse-testfile");
  const std::string mystr("The quick brown fox jumps over the lazy dog");

  FILE *out = fopen(filename.c_str(), "w");
  fwrite(mystr.c_str(), 1, mystr.size(), out);
  fclose(out);

  std::string tmp;
  ASSERT_TRUE(readFile(filename, tmp));
  ASSERT_EQ(tmp, mystr);
}
