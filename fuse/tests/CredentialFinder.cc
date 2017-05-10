//------------------------------------------------------------------------------
// File: CredentialFinder.cc
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

#include "../CredentialFinder.hh"
#include <gtest/gtest.h>

TEST(Environment, basic_sanity) {
  Environment env;

  std::string envStr = "KEY1=VALUE"; envStr.append(1, '\0');
  envStr += "non-key value entry"; envStr.append(1, '\0');
  envStr += "Key2=SomeValue"; envStr.append(1, '\0');
  envStr += "KEY1=Duplicate"; envStr.append(1, '\0');

  env.fromString(envStr);

  std::vector<std::string> expected = {"KEY1=VALUE", "non-key value entry", "Key2=SomeValue", "KEY1=Duplicate"};
  ASSERT_EQ(env.getAll(), expected);

  ASSERT_EQ(env.get("KEY1"), "VALUE");
  ASSERT_EQ(env.get("Key2"), "SomeValue");

  // now try reading from a file
  const std::string filename("/tmp/fuse-testfile");

  FILE *out = fopen(filename.c_str(), "w");
  fwrite(envStr.c_str(), 1, envStr.size(), out);
  fclose(out);

  Environment env2;
  env2.fromFile(filename);

  ASSERT_EQ(env2.getAll(), expected);
  ASSERT_EQ(env2.get("KEY1"), "VALUE");
  ASSERT_EQ(env2.get("Key2"), "SomeValue");
}
