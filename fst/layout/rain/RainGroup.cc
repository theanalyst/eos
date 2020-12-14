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
  mGroupSize = num_data_blocks * mStripeWidth;
  mGroupOffset = (grp_offset / mGroupSize) * mGroupSize;
  mRowSize = mDataFiles.size() * mStripeWidth;

  for (auto file_id = 0ull; file_id < total_files; ++file_id) {
    uint64_t block_off = (mGroupOffset / mGroupSize) *
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

//------------------------------------------------------------------------------
// Write data in the group
//------------------------------------------------------------------------------
bool
RainGroup::Write(uint64_t l_offset, uint32_t l_length, const char* buffer)
{
  if ((l_offset < mGroupOffset) ||
      (l_offset + l_length >= (mGroupOffset + mGroupSize))) {
    eos_static_err("msg=\"write requests not in the current rain group\" "
                   "grp_off=%llu grp_len=%llu req_off=%llu req_len=%lu",
                   mGroupOffset, mGroupSize, l_offset, l_length);
    return false;
  }

  const char* ptr = buffer;
  auto requests = GetBlockPos(l_offset, l_length);

  for (const auto& req : requests) {
    auto& block = mDataBlocks[req.mColumnId][req.mRowId];
    block.StoreData(ptr, req.mFileOff, req.mFileLen);
    ptr += req.mFileLen;

    if (block.IsComplete() && !block.IsFlushed()) {
      block.Write();
    }
  }

  return true;
}

//------------------------------------------------------------------------------
// Check if group data is complete - all the data blocks are complete
//------------------------------------------------------------------------------
bool
RainGroup::IsDataComplete() const
{
  bool is_complete = true;

  for (size_t stripe_id = 0; stripe_id < mDataFiles.size(); ++stripe_id) {
    for (size_t block_indx = 0; block_indx < mDataFiles.size(); ++block_indx) {
      if (!mDataBlocks[stripe_id][block_indx].IsComplete()) {
        is_complete = false;
        break;
      }
    }
  }

  return is_complete;
}

//------------------------------------------------------------------------------
// Convert a (logical) position in the initial file to a list of file stripe
// positions inside the individual stripes.
//------------------------------------------------------------------------------
std::list<RainGroup::StripeRequest>
RainGroup::GetBlockPos(uint64_t l_offset, uint32_t l_length)
{
  std::list<StripeRequest> requests;

  if ((l_offset < mGroupOffset) ||
      (l_offset + l_length > mGroupOffset + mGroupSize)) {
    return requests;
  }

  // Stripe base offset
  int column_id, row_id;
  uint64_t s_off; // Offset in the current stripe
  uint32_t s_len; // Length in the current stripe block
  uint64_t b_end_off; // End offset of the current stripe block

  while (l_length) {
    column_id = (l_offset / mStripeWidth) % mDataFiles.size();
    row_id = (l_offset / mRowSize) / mDataFiles.size();
    s_off = (l_offset / mRowSize) * mStripeWidth + l_offset % mStripeWidth;
    b_end_off = (l_offset / mStripeWidth) * mStripeWidth + mStripeWidth;

    if (s_off + l_length > b_end_off) {
      s_len = b_end_off - s_off;
    } else {
      s_len = l_length;
    }

    requests.emplace_back(column_id, row_id, s_off, s_len);
    l_length -= s_len;
    l_offset += s_len;
  }

  return requests;
}

EOSFSTNAMESPACE_END
