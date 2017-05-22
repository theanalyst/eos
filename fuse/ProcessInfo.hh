// ----------------------------------------------------------------------
// File: ProcessInfo.hh
// Author: Georgios Bitzes - CERN
// ----------------------------------------------------------------------

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

#ifndef __PROCESS_INFO_HH__
#define __PROCESS_INFO_HH__

#include "Utils.hh"

typedef int64_t Jiffies;

// Holds information about a specific process.
class ProcessInfo {
public:
  ProcessInfo() : /*empty(true),*/ pid(0), ppid(0), sid(0), startTime(-1) {}

  // void fill(pid_t pid, pid_t ppid, pid_t sid, uid_t fsuid, gid_t fsgid, Jiffies startTime, const std::vector<std::string> &cmd) {
  //   if(!empty) THROW("ProcessInfo can only be filled once");
  //   empty = false;
  //
  //   this->pid = pid;
  //   this->ppid = ppid;
  //   this->sid = sid;
  //   this->fsuid = fsuid;
  //   this->fsgid = fsgid;
  //   this->startTime = startTime;
  //   this->cmd = cmd;
  // }
  //
  // bool isEmpty() const {
  //   return empty;
  // }

  pid_t getPid() const {
    return pid;
  }

  pid_t getParentId() const {
    return ppid;
  }

  pid_t getSid() const {
    return sid;
  }

  Jiffies getStartTime() const {
    return startTime;
  }

  const std::vector<std::string> &getCmd() const {
    return cmd;
  }

private:
  // bool empty;

// TODO(gbitzes): Make these private once ProcessInfoProvider is implemented
public:
  pid_t pid;
  pid_t ppid;
  pid_t sid;
  Jiffies startTime;
  std::vector<std::string> cmd;
  std::string cmdStr; // TODO(gbitzes): remove this eventually?
};

#endif
