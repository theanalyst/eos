// ----------------------------------------------------------------------
// File: AutoRepair.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2019 CERN/Switzerland                                  *
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

/**
 * @file   AutoRepair.hh
 * 
 * @brief  Helper class to define autorepair configurations
 * 
 * 
 */

#pragma once

/*----------------------------------------------------------------------------*/
#include "common/Namespace.hh"
/*----------------------------------------------------------------------------*/
#include "XrdSys/XrdSysPthread.hh"
/*----------------------------------------------------------------------------*/
#include <string>
/*----------------------------------------------------------------------------*/

EOSCOMMONNAMESPACE_BEGIN

/*----------------------------------------------------------------------------*/
//! Class to log all commands which include a comment specified on the EOS shell
/*----------------------------------------------------------------------------*/


class AutoRepair {
public:
  AutoRepair() { Reset(); }
  virtual ~AutoRepair() {}

  int Parse(const std::string& params);
  static const char* Usage();
  static const char* Defaults() { return "posc:1,dropall=1,drop=1,scan=1"; }
  void Reset() { posc = dropall = drop = scan = false; }

  bool do_posc() { XrdSysMutexHelper lLock(lock); return posc; }
  bool do_dropall() { XrdSysMutexHelper lLock(lock); return dropall; }
  bool do_drop() { XrdSysMutexHelper lLock(lock); return drop; }
  bool do_scan() { XrdSysMutexHelper lLock(lock); return scan; }

  void set_posc() { XrdSysMutexHelper lLock(lock); posc = true; }
  void set_dropall() { XrdSysMutexHelper lLock(lock); dropall = true; }
  void set_drop() { XrdSysMutexHelper lLock(lock); drop = true; }
  void set_scan() { XrdSysMutexHelper lLock(lock); scan = true; }
private:

  XrdSysMutex lock; // guard for members                                                                                      
  bool posc;    // persistency only on successfull close                                                                      
  bool dropall; // can drop all replicas                                                                                      
  bool drop;    // can drop single replicas                                                                                   
  bool scan;    // scanner calls autorepair                                                                                   
};

EOSCOMMONNAMESPACE_END

