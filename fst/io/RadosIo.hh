//------------------------------------------------------------------------------
//! @file RadosIo.hh
//! @author Joaquim Rocha <joaquim.rocha@cern.ch>
//! @brief Class used for doing local IO operations
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

#ifndef __EOSFST_RADOSFILEIO__HH__
#define __EOSFST_RADOSFILEIO__HH__

/*----------------------------------------------------------------------------*/
#include <libradosfs.hh>

#include "fst/io/FileIo.hh"

/*----------------------------------------------------------------------------*/

EOSFSTNAMESPACE_BEGIN

typedef std::map<std::string, std::shared_ptr<radosfs::Filesystem>> ConfRadosFsMap;
typedef std::map<std::string, std::shared_ptr<radosfs::FileInode>> RadosFsFileInodeMap;

class RadosFsManager : public eos::common::LogId {
public:
  RadosFsManager();
  virtual ~RadosFsManager();

  std::shared_ptr<radosfs::FileInode> getInode(const std::string &name);
  std::shared_ptr<radosfs::Filesystem> getFilesystem();
  std::shared_ptr<radosfs::Filesystem> getFilesystem(const std::string &cephConfPath);

private:
  static ConfRadosFsMap mFsMap;
  static RadosFsFileInodeMap mFileInodeMap;

  bool parsePoolsFromPath(const std::string &path, std::string &pool,
                          std::string &inode);

  int processPath(const std::string &path, std::string &pool, std::string &inode);
};

//------------------------------------------------------------------------------
//! Class used for doing local IO operations
//------------------------------------------------------------------------------
class RadosIo : public FileIo {
public:
  //--------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param handle to logical file
  //! @param client security entity
  //!
  //--------------------------------------------------------------------------
  RadosIo (XrdFstOfsFile* file,
           const XrdSecEntity* client);


  //--------------------------------------------------------------------------
  //! Destructor
  //--------------------------------------------------------------------------
  virtual ~RadosIo ();


  //--------------------------------------------------------------------------
  //! Open file
  //!
  //! @param path file path to local file
  //! @param flags open flags
  //! @param mode open mode
  //! @param opaque opaque information
  //! @param timeout timeout value
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Open (const std::string& path,
                    XrdSfsFileOpenMode flags,
                    mode_t mode = 0,
                    const std::string& opaque = "",
                    uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Read from file - sync
  //!
  //! @param offset offset in file
  //! @param buffer where the data is read
  //! @param length read length
  //! @param timeout timeout value
  //!
  //! @return number of bytes read or -1 if error
  //!
  //--------------------------------------------------------------------------
  virtual int64_t Read (XrdSfsFileOffset offset,
                        char* buffer,
                        XrdSfsXferSize length,
                        uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Write to file - sync
  //!
  //! @param offset offset in file
  //! @param buffer data to be written
  //! @param length length
  //! @param timeout timeout value
  //!
  //! @return number of bytes written or -1 if error
  //!
  //--------------------------------------------------------------------------
  virtual int64_t Write (XrdSfsFileOffset offset,
                         const char* buffer,
                         XrdSfsXferSize length,
                         uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Read from file async - falls back to synchrounous mode
  //!
  //! @param offset offset in file
  //! @param buffer where the data is read
  //! @param length read length
  //! @param timeout timeout value
  //!
  //! @return number of bytes read or -1 if error
  //!
  //--------------------------------------------------------------------------
  virtual int64_t ReadAsync (XrdSfsFileOffset offset,
                             char* buffer,
                             XrdSfsXferSize length,
                             bool readahead = false,
                             uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Write to file async - falls back to synchronous mode
  //!
  //! @param offset offset
  //! @param buffer data to be written
  //! @param length length
  //! @param timeout timeout value
  //!
  //! @return number of bytes written or -1 if error
  //!
  //--------------------------------------------------------------------------
  virtual int64_t WriteAsync (XrdSfsFileOffset offset,
                              const char* buffer,
                              XrdSfsXferSize length,
                              uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Truncate
  //!
  //! @param offset truncate file to this value
  //! @param timeout timeout value
  //!
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Truncate (XrdSfsFileOffset offset, uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Allocate file space
  //!
  //! @param length space to be allocated
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Fallocate (XrdSfsFileOffset lenght);


  //--------------------------------------------------------------------------
  //! Deallocate file space
  //!
  //! @param fromOffset offset start
  //! @param toOffset offset end
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Fdeallocate (XrdSfsFileOffset fromOffset,
                           XrdSfsFileOffset toOffset);


  //--------------------------------------------------------------------------
  //! Remove file
  //!
  //! @param timeout timeout value
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Remove (uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Sync file to disk
  //!
  //! @param timeout timeout value
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Sync (uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Close file
  //!
  //! @param timeout timeout value
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Close (uint16_t timeout = 0);


  //--------------------------------------------------------------------------
  //! Get stats about the file
  //!
  //! @param buf stat buffer
  //! @param timeout timeout value
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //!
  //--------------------------------------------------------------------------
  virtual int Stat (struct stat* buf, uint16_t timeout = 0);

  //--------------------------------------------------------------------------
  //! Check for the existance of a file
  //!
  //! @param path to the file
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //--------------------------------------------------------------------------
  virtual int Exists(const char* path);

  //--------------------------------------------------------------------------
  //! Delete a file
  //!
  //! @param path to the file to be deleted
  //!
  //! @return 0 on success, -1 otherwise and error code is set
  //--------------------------------------------------------------------------

  virtual int Delete(const char* path);

  //--------------------------------------------------------------------------
  //! Get pointer to async meta handler object
  //!
  //! @return pointer to async handler, NULL otherwise
  //!
  //--------------------------------------------------------------------------
  virtual void* GetAsyncHandler ();

  //--------------------------------------------------------------------------
  //! Plug-in function to fill a statfs structure about the storage filling
  //! state
  //! @param path to statfs
  //! @param statfs return struct
  //! @return 0 if successful otherwise errno
  //--------------------------------------------------------------------------

  virtual int Statfs (const char* path, struct statfs* statFs)
  {
    //! IMPLEMENT ME PROPERLY!
    eos_info("path=%s", path);
    statFs->f_type = 0xceff;
    statFs->f_bsize = 1 * 1024 * 1024;
    statFs->f_blocks = 4 * 1024 * 1024;
    statFs->f_bfree = 4 * 1024 * 1024;
    statFs->f_bavail = 4 * 1024 * 1024;
    statFs->f_files = 4 * 1024 * 1024;
    statFs->f_ffree = 4 * 1024 * 1024;
    return 0;
  }

  class Attr : public FileIo::Attr {
    // ------------------------------------------------------------------------
    //! Set a binary attribute
    // ------------------------------------------------------------------------

    virtual bool Set (const char* name, const char* value, size_t len)
    {
      return false;
    }

    // ------------------------------------------------------------------------
    //! Set a string attribute
    // ------------------------------------------------------------------------

    virtual bool Set (std::string key, std::string value)
    {
      return false;
    }

    // ------------------------------------------------------------------------
    //! Get a binary attribute by name
    // ------------------------------------------------------------------------

    virtual bool Get (const char* name, char* value, size_t &size)
    {
      return false;
    }


    // ------------------------------------------------------------------------
    //! Get a string attribute by name (name has to start with 'user.' !!!)
    // ------------------------------------------------------------------------

    virtual std::string Get (std::string name)
    {
      return "";
    }

    // ------------------------------------------------------------------------
    //! Non-static Factory function to create an attribute object
    // ------------------------------------------------------------------------

    virtual Attr* OpenAttribute (const char* path)
    {
      return new Attr(path);
    }

    // ------------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------------

    Attr (const char* path)
    {
    }

    // ------------------------------------------------------------------------
    // Destructor
    // ------------------------------------------------------------------------
    virtual ~Attr ();
  };

private:

  RadosFsManager mRadosFsMgr;
  std::shared_ptr<radosfs::FileInode> mInode;

  //--------------------------------------------------------------------------
  //! Disable copy constructor
  //--------------------------------------------------------------------------
  RadosIo (const RadosIo&) = delete;


  //--------------------------------------------------------------------------
  //! Disable assign operator
  //--------------------------------------------------------------------------
  RadosIo& operator = (const RadosIo&) = delete;
};

EOSFSTNAMESPACE_END

#endif  // __EOSFST_LOCALFILEIO_HH__


