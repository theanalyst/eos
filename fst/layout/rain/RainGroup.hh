//------------------------------------------------------------------------------
//! @file RainGroup.hh
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

#pragma once
#include "fst/Namespace.hh"
#include "fst/layout/rain/RainBlock.hh"
#include <memory>
#include <vector>

EOSFSTNAMESPACE_BEGIN

class RainBlock;

//------------------------------------------------------------------------------
//! Class RainGroup which holds a group of buffers needed to compute the parity
//! information. Such an object holds both the data and the parity blocks
//------------------------------------------------------------------------------
class RainGroup
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param data_files vector of file objects with indices corresponding to
  //!        the logical stripe ids for the data stipes
  //! @param parity_files vector of file objects with indices corresponding to
  //!        the logical stripe ids for the parity stripes
  //! @param stripe_width size of each individual stripe block
  //! @param grp_offset offset of the current group
  //----------------------------------------------------------------------------
  RainGroup(const std::vector<FileIo*>& data_files,
            const std::vector<FileIo*>& parity_Files,
            uint64_t stripe_width, uint64_t grp_offset);

  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  ~RainGroup();

  //----------------------------------------------------------------------------
  //! Write data in the group
  //!
  //! @param l_offset offset in the logical file (global)
  //! @param l_length length of the data
  //! @param buffer client buffer holding the information
  //!
  //! @return
  //----------------------------------------------------------------------------
  bool Write(uint64_t l_offset, uint32_t l_length, const char* buffer);

private:

  struct StripeRequest {
    int mColumnId; ///! Column index of the block in mDataBlock matrix
    int mRowId; ///! Rown index of the block in the mDataBlock matric
    uint64_t mFileOff; ///< Offset inside the stripe file
    uint32_t mFileLen; ///< Length inside the stripe file

    StripeRequest(int cid, int rid, uint64_t s_off, uint32_t s_len):
      mColumnId(cid), mRowId(rid), mFileOff(s_off), mFileLen(s_len)
    {}
  };

  //----------------------------------------------------------------------------
  //! Convert a (logical) block from the initial file to list of file stripe
  //! positions inside the individual stripes.
  //!
  //! @param l_offset offset in the logical file
  //! @param l_length length of the request - not bigger than group size
  //!
  //! @return list of StripeRequest object that identify the corresponding
  //!         blocks to be accessed in the mDataBlocks matrix
  //----------------------------------------------------------------------------
  std::list<StripeRequest>
  GetBlockPos(uint64_t l_offset, uint32_t l_length);

  //----------------------------------------------------------------------------
  //! Check if group data is complete - all the data blocks are complete
  //!
  //! @return true if complete, otherwise false
  //----------------------------------------------------------------------------
  bool IsDataComplete() const;

  std::vector<FileIo*> mDataFiles;
  std::vector<FileIo*> mParityFiles;
  std::vector<FileIo*> mFiles;
  uint64_t mStripeWidth; ///< Stripe block size
  uint64_t mGroupOffset; ///< Group offset
  uint64_t mGroupSize; ///< Group size
  uint64_t mRowSize; ///< Total size of a row
  //! Matrix where each column represents a data/parity file
  std::vector<std::vector<eos::fst::RainBlock>> mDataBlocks;
};

EOSFSTNAMESPACE_END
