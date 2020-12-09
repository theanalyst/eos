//------------------------------------------------------------------------------
//! @file RainGroup.cc
//! @author Elvin-Alin Sindrilaru - CERN
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2020 CERN/Switzerland                                  *
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

#include "fst/layout/rain/RainGroup.hh"

EOSFSTNAMESPACE_BEGIN

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
RainGroup::RainGroup(const std::vector<FileIo*>& data_files,
                     const std::vector<FileIo*>& parity_files,
                     uint64_t stripe_width, uint64_t grp_offset)
  : mDataFiles(data_files), mParityFiles(parity_files),
    mStripeWidth(stripe_width)
{
  mFiles.insert(mFiles.end(), mDataFiles.begin(), mDataFiles.end());
  mFiles.insert(mFiles.end(), mParityFiles.begin(), mParityFiles.end());
  uint32_t total_files = mFiles.size();
  uint32_t num_data_blocks = mDataFiles.size() * mDataFiles.size();
  uint64_t group_size = num_data_blocks * mStripeWidth;
  mGroupOffset = (grp_offset / group_size) * group_size;

  for (auto file_id = 0ull; file_id < total_files; ++file_id) {
    uint64_t block_off = (mGroupOffset / group_size) *
                         (mDataFiles.size() * mStripeWidth);
    mDataBlocks.emplace_back();

    for (auto row = 0ul; row < mDataFiles.size(); ++row) {
      mDataBlocks.back().emplace_back(mFiles[file_id], block_off, mStripeWidth);
      block_off += mStripeWidth;
    }
  }
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
RainGroup::~RainGroup()
{
  mDataBlocks.clear();
}

EOSFSTNAMESPACE_END
