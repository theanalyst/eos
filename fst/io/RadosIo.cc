//------------------------------------------------------------------------------
// File: RadosIo.cc
// Author: Joaquim Rocha <joaquim.rocha@cern.ch>
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2011-2015 CERN/Switzerland                             *
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

/*----------------------------------------------------------------------------*/
#include <cstdlib>

#include "fst/XrdFstOfsFile.hh"
#include "fst/io/RadosIo.hh"
/*----------------------------------------------------------------------------*/
#ifndef __APPLE__
#include <xfs/xfs.h>
#endif

/*----------------------------------------------------------------------------*/

EOSFSTNAMESPACE_BEGIN

ConfRadosFsMap RadosFsManager::mFsMap;
RadosFsFileInodeMap RadosFsManager::mFileInodeMap;

RadosFsManager::RadosFsManager()
{}

RadosFsManager::~RadosFsManager()
{}

std::shared_ptr<radosfs::FileInode>
RadosFsManager::getInode(const std::string &path)
{
  std::shared_ptr<radosfs::FileInode> inode;

  auto mapIt = mFileInodeMap.find(path);
  if (mapIt != mFileInodeMap.end())
  {
    eos_debug("Found FileInode in map: %s", (*mapIt).first.c_str());
    return (*mapIt).second;
  }

  std::string pool, inodeName;
  int ret = processPath(path, pool, inodeName);

  if (ret == SFS_OK)
  {
    auto fs = getFilesystem();

    if (!fs)
    {
      eos_err("Cannot get radosfs::Filesystem instance when making "
              "inode %s, %p, %p", path.c_str(), this, fs.get());
      errno = ENODEV;
      return inode;
    }

    inode = std::make_shared<radosfs::FileInode>(fs.get(), pool, inodeName);
    mFileInodeMap[path] = inode;
    eos_debug("Instantiating a new FileInode '%s' from %s", inodeName.c_str(),
              pool.c_str());
  }

  return inode;
}

std::shared_ptr<radosfs::Filesystem>
RadosFsManager::getFilesystem()
{
  char* cephConfPath = getenv("CEPH_CONF");
  return getFilesystem(cephConfPath);
}

std::shared_ptr<radosfs::Filesystem>
RadosFsManager::getFilesystem(const std::string &cephConfPath)
{
  std::shared_ptr<radosfs::Filesystem> fs;

  if (cephConfPath.empty() && mFsMap.empty())
  {
    eos_info("No configuration for RadosFs found. Please set the CEPH_CONF "
             "env var.");
    return fs;
  }

  if (mFsMap.count(cephConfPath) == 0)
  {
    eos_info("Adding a new RadosFs instance: %s %p", cephConfPath.c_str(),
             this);
    char* cephUser = getenv("CEPH_USER");
    fs = std::make_shared<radosfs::Filesystem>();
    int ret = fs->init(cephUser ? cephUser : "", cephConfPath);

    if (ret != 0)
    {
      eos_err("Cannot initialize radosfs::Filesystem with conf file '%s' "
              "and user name '%s'", cephConfPath.c_str(),
              (cephUser ? cephUser : ""));
      errno = ret;
      fs.reset();
    }
    else
    {
      mFsMap[cephConfPath] = fs;
    }
  }
  else
  {
    fs = mFsMap[cephConfPath];
  }

  return fs;
}

bool
RadosFsManager::parsePoolsFromPath(const std::string &path, std::string &pool,
                                   std::string &inode)
{
  std::vector<std::string> tokens;
  eos::common::StringConversion::Tokenize(path, tokens, ":");

  if (tokens.size() < 3)
    return false;

  pool = tokens[1];
  inode = tokens[2];

  eos_debug("Tokens from path '%s': %s|%s|%s|", path.c_str(), tokens[0].c_str(),
            tokens[1].c_str(), tokens[2].c_str());
  return true;
}

int
RadosFsManager::processPath(const std::string &path, std::string &pool,
                            std::string &inode)
{
  errno = 0;
  auto fs = getFilesystem();

  if (!fs)
  {
    eos_err("RadosFs not set...");
    errno = ENODEV;
    return SFS_ERROR;
  }

  if (!parsePoolsFromPath(path, pool, inode))
  {
    eos_err("Cannot parse pool or inode info from path: %s", path.c_str());
    errno = EINVAL;
    return SFS_ERROR;
  }

  if (fs->dataPoolSize(pool) < 0)
  {
    errno = std::abs(fs->addDataPool(pool, "/"));
    if (errno != 0)
    {
      eos_err("Error adding pool: %s (retcode=%d)", pool.c_str(), errno);
      return SFS_ERROR;
    }
  }

  return SFS_OK;
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
RadosIo::RadosIo (XrdFstOfsFile* file,
                  const XrdSecEntity* client)
  : FileIo()
{
  mType = "RadosIO";
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------

RadosIo::~RadosIo ()
{
  //empty
}


//------------------------------------------------------------------------------
// Open file
//------------------------------------------------------------------------------

int
RadosIo::Open (const std::string& path,
               XrdSfsFileOpenMode flags,
               mode_t mode,
               const std::string& opaque,
               uint16_t timeout)
{
  eos_info("path=%s flags=%x", path.c_str(), flags);

  errno = 0;
  mInode = mRadosFsMgr.getInode(path);

  if (!mInode)
    return SFS_ERROR;

  return SFS_OK;
}


//------------------------------------------------------------------------------
// Read from file - sync
//------------------------------------------------------------------------------

int64_t
RadosIo::Read (XrdSfsFileOffset offset,
               char* buffer,
               XrdSfsXferSize length,
               uint16_t timeout)
{
  eos_debug("offset = %lld, length = %lld",
            static_cast<int64_t> (offset),
            static_cast<int64_t> (length));

  return mInode->read(buffer, offset, (size_t) length);
}

//------------------------------------------------------------------------------
// Write to file - sync
//------------------------------------------------------------------------------

int64_t
RadosIo::Write (XrdSfsFileOffset offset,
                const char* buffer,
                XrdSfsXferSize length,
                uint16_t timeout)
{
  eos_debug("offset = %lld, length = %lld",
            static_cast<int64_t> (offset),
            static_cast<int64_t> (length));

  errno = std::abs(mInode->write(buffer, offset, (size_t) length));

  int64_t ret = SFS_ERROR;

  if (errno == 0)
  {
    // We return the length given as a parameter because
    // radosfs::FileInode::writeSync does not return the
    // length of the written data
    ret = length;
  }
  else
  {
    eos_err("Error writing inode: %d", ret);
  }

  return ret;
}


//------------------------------------------------------------------------------
// Read from file async - falls back on synchronous mode
//------------------------------------------------------------------------------

int64_t
RadosIo::ReadAsync (XrdSfsFileOffset offset,
                    char* buffer,
                    XrdSfsXferSize length,
                    bool readahead,
                    uint16_t timeout)
{
  return Read(offset, buffer, length, timeout);
}


//------------------------------------------------------------------------------
// Write to file async - falls back on synchronous mode
//------------------------------------------------------------------------------

int64_t
RadosIo::WriteAsync (XrdSfsFileOffset offset,
                     const char* buffer,
                     XrdSfsXferSize length,
                     uint16_t timeout)
{
  return Write(offset, buffer, length, timeout);
}


//------------------------------------------------------------------------------
// Truncate file
//------------------------------------------------------------------------------

int
RadosIo::Truncate (XrdSfsFileOffset offset, uint16_t timeout)
{
  errno = 0;

  if (!mInode)
  {
    eos_err("Cannot truncate: radosfs::FileInode not instanced.");
    errno = ENOENT;
    return SFS_ERROR;
  }

  eos_info("Truncating %s to %d", mInode->name().c_str(), offset);

  errno = std::abs(mInode->truncate(offset));
  return (errno == 0 ? SFS_OK : SFS_ERROR);
}


//------------------------------------------------------------------------------
// Allocate space for file
//------------------------------------------------------------------------------

int
RadosIo::Fallocate (XrdSfsFileOffset length)
{
  eos_debug("N/A");
  return SFS_OK;
}


//------------------------------------------------------------------------------
// Deallocate space reserved for file
//------------------------------------------------------------------------------

int
RadosIo::Fdeallocate (XrdSfsFileOffset fromOffset,
                      XrdSfsFileOffset toOffset)
{
  eos_debug("N/A");
  return SFS_OK;
}


//------------------------------------------------------------------------------
// Sync file to disk
//------------------------------------------------------------------------------

int
RadosIo::Sync (uint16_t timeout)
{
  errno = 0;
  int ret = SFS_ERROR;

  if (!mInode)
  {
    eos_err("Cannot sync: radosfs::FileInode not instanced.");
    errno = ENOENT;
  }
  else
  {
    eos_debug("Syncing radosfs::FileInode '%s'", mInode->name());
    errno = std::abs(mInode->sync());
  }

  if (errno == 0)
    ret = SFS_OK;

  return ret;
}


//------------------------------------------------------------------------------
// Get stats about the file
//------------------------------------------------------------------------------

int
RadosIo::Stat (struct stat* buf, uint16_t timeout)
{
  errno = 0;
  u_int64_t size = 0;
  int ret = ENOENT;

  if (!mInode)
  {
    errno = ENOENT;
    return SFS_ERROR;
  }

  mInode->sync();
  ret = mInode->getSize(size);

  if (ret != 0)
  {
    errno = std::abs(ret);
    return SFS_ERROR;
  }

  buf->st_size = static_cast<off_t>(size);
  return SFS_OK;
}

//------------------------------------------------------------------------------
// Close file
//------------------------------------------------------------------------------

int
RadosIo::Close (uint16_t timeout)
{
  if (mInode)
  {
    mInode->sync();
  }

  return SFS_OK;
}


//------------------------------------------------------------------------------
// Remove file
//------------------------------------------------------------------------------

int
RadosIo::Remove (uint16_t timeout)
{
  errno = 0;
  if (!mInode)
  {
    eos_err("Cannot remove: radosfs::FileInode not instanced.");
    errno = ENOENT;
    return SFS_ERROR;
  }

  eos_info("Removing %s", mInode->name().c_str());

  errno = std::abs(mInode->remove());
  return (errno == 0 ? SFS_OK : SFS_ERROR);
}


//------------------------------------------------------------------------------
// Get pointer to async meta handler object
//------------------------------------------------------------------------------

void*
RadosIo::GetAsyncHandler ()
{
  return NULL;
}

int
RadosIo::Exists(const char* path)
{
  errno = 0;
  auto inode = mRadosFsMgr.getInode(path);
  if (!inode)
    return SFS_ERROR;

  inode->sync();
  u_int64_t size;
  errno = std::abs(inode->getSize(size));

  if (errno == 0)
  {
    return SFS_OK;
  }

  return SFS_ERROR;
}

int
RadosIo::Delete(const char* path)
{
  errno = 0;

  auto inode = mRadosFsMgr.getInode(path);
  if (!inode)
    return SFS_ERROR;

  u_int64_t size;
  errno = std::abs(inode->remove());

  if (errno == 0)
  {
    return SFS_OK;
  }
  else
  {
    eos_err("Error deleting inode '%s': %s (errno=%d)", path, strerror(errno),
            errno);
  }

  return SFS_ERROR;
}

bool
RadosIo::Attr::Set(const char* name, const char* value, size_t len)
{
  return Set(name, std::string(value, len));
}

bool
RadosIo::Attr::Set(std::string key, std::string value)
{
  errno = 0;

  if (!mInode)
  {
    eos_err("Cannot set attribute: radosfs::FileInode not instanced.");
    errno = ENOENT;
    return false;
  }

  return mInode->setXAttr(key, value) == 0;
}

bool
RadosIo::Attr::Get(const char* name, char* value, size_t &size)
{
  std::string xattrValue = Get(name);

  if (errno == 0)
  {
    memcpy(value, xattrValue.c_str(), size);
    return true;
  }

  return false;
}

std::string
RadosIo::Attr::Get(std::string name)
{
  errno = 0;

  if (!mInode)
  {
    eos_err("Cannot get attribute: radosfs::FileInode not instanced.");
    errno = ENOENT;
    return false;
  }

  std::string xattrValue;
  int ret = mInode->getXAttr(name, xattrValue);

  if (ret < 0)
  {
    errno = std::abs(ret);
  }

  return xattrValue;
}

RadosIo::Attr::Attr(const char* path)
{
  mInode = mRadosFsMgr.getInode(path);

  if (!mInode)
    eos_err("Error getting instance of FileInode for path '%s'", path);
}

RadosIo::Attr::~Attr()
{}

EOSFSTNAMESPACE_END
