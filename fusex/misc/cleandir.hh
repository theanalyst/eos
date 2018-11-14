//------------------------------------------------------------------------------
//! @file cleandir.hh
//! @author Andreas-Joachim Peters CERN
//! @brief Class implementing some convenience functions for filenames
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2016 CERN/Switzerland                                  *
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


#ifndef FUSE_CLEANDIR_HH_
#define FUSE_CLEANDIR_HH_

#include <sys/types.h>
#include <dirent.h>

class cleandir
{
public:
  static int remove(const std::string& path)
  {
    int retc = 0;
    DIR* dirp = ::opendir(path.c_str());
    struct dirent* item;

    if (!dirp) {
      return -1;
    }

    while ((item = ::readdir(dirp)) != NULL) {
      std::string name = item->d_name;
      std::string dpath = path;

      if ((name == ".") ||
          (name == "..")) {
        continue;
      }

      dpath += "/";
      dpath += name;
      retc |= ::unlink(dpath.c_str());
    }

    ::closedir(dirp);
    retc |= ::rmdir(path.c_str());
    return retc;
  }
};

#endif
