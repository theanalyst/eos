// ----------------------------------------------------------------------
// File: ProcCache.cc
// Author: Geoffray Adde - CERN
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

#define __STDC_FORMAT_MACROS
#include <signal.h>
#define __PROCCACHE__NOGPROCCACHE__
#include "ProcCache.hh"
#include "common/Logging.hh"
#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <XrdSys/XrdSysAtomics.hh>
#include <vector>

//ProcCache gProcCache;
std::vector<ProcCache> gProcCacheV;
int gProcCacheShardSize;

ProcCache gProcCache;

krb5_context ProcReaderKrb5UserName::sKcontext;
bool ProcReaderKrb5UserName::sKcontextOk = (!krb5_init_context(
      &ProcReaderKrb5UserName::sKcontext)) ||
    (!eos_static_crit("error initializing Krb5"));;
eos::common::RWMutex ProcReaderKrb5UserName::sMutex;
bool ProcReaderKrb5UserName::sMutexOk = false;

void ProcReaderKrb5UserName::StaticDestroy()
{
  //if(sKcontextOk) krb5_free_context(sKcontext);
}

bool ProcReaderKrb5UserName::ReadUserName(string& userName)
{
  eos::common::RWMutexWriteLock lock(sMutex);

  if (!sKcontextOk) {
    return false;
  }

  eos_static_debug("starting Krb5 reading");
  bool result = false;
  krb5_principal princ = NULL;
  krb5_ccache cache = NULL;
  int retval;
  size_t where;
  char* ptrusername = NULL;

  // get the credential cache
  if ((retval = krb5_cc_resolve(sKcontext, pKrb5CcFile.c_str(), &cache))) {
    eos_static_err("error resolving Krb5 credential cache%s, error code is %d",
                   pKrb5CcFile.c_str(), (int)retval);
    goto cleanup;
  }

  // get the principal of the cache
  if ((retval = krb5_cc_get_principal(sKcontext, cache, &princ))) {
    eos_static_err("while getting principal of krb5cc %s, error code is %d",
                   pKrb5CcFile.c_str(), (int)retval);
    goto cleanup;
  }

  // get the name of the principal
  // get the name of the principal
  if ((retval = krb5_unparse_name(sKcontext, princ, &ptrusername))) {
    eos_static_err("while getting name of principal of krb5cc %s, error code is %d",
                   pKrb5CcFile.c_str(), (int)retval);
    goto cleanup;
  }

  userName.assign(ptrusername);
  // parse the user name
  where = userName.find('@');

  if (where == std::string::npos) {
    eos_static_err("while parsing username of principal name %s, could not find '@'",
                   userName.c_str());
    goto cleanup;
  }

  userName.resize(where);
  eos_static_debug("parsed user name  %s", userName.c_str());
  result = true;
cleanup:
  eos_static_debug("finishing Krb5 reading");

  if (cache) {
    krb5_cc_close(sKcontext, cache);
  }

  if (princ) {
    krb5_free_principal(sKcontext, princ);
  }

  if (ptrusername) {
    krb5_free_unparsed_name(sKcontext, ptrusername);
  }

  return result;
}

time_t ProcReaderKrb5UserName::GetModifTime()
{
  struct tm* clock;
  struct stat attrib;

  if (pKrb5CcFile.substr(0, 5) != "FILE:") {
    eos_static_err("expecting a credential cache file and got %s",
                   pKrb5CcFile.c_str());
    return 0;
  }

  if (stat(pKrb5CcFile.c_str() + 5, &attrib)) {
    return 0;
  }

  clock = gmtime(&
                 (attrib.st_mtime));      // Get the last modified time and put it into the time structure
  return mktime(clock);
}

bool ProcReaderGsiIdentity::sInitOk = true;

void ProcReaderGsiIdentity::StaticDestroy() {}

bool
ProcReaderGsiIdentity::ReadIdentity(string& sidentity)
{
  bool result = false;
  BIO*          certbio = BIO_new(BIO_s_file());
  X509*         cert = NULL;
  X509_NAME*    certsubject = NULL;
  char*         subj = NULL;

  if (!certbio) {
    eos_static_err("error allocating BIO buffer");
    goto gsicleanup;
  }

  BIO_read_filename(certbio, pGsiProxyFile.c_str());

  if (!(cert = PEM_read_bio_X509(certbio, NULL, 0, NULL))) {
    eos_static_err("error loading cert into memory");
    goto gsicleanup;
  }

  certsubject = X509_NAME_new();

  if (!certsubject) {
    eos_static_err("error initializing certsubject");
    goto gsicleanup;
  }

  subj = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);

  if (!subj) {
    eos_static_err("error reading subject name");
    goto gsicleanup;
  }

  sidentity = subj;
  result = true;
gsicleanup:

  if (certbio) {
    BIO_free_all(certbio);
  }

  if (cert) {
    X509_free(cert);
  }

  if (subj) {
    OPENSSL_free(subj);
  }

  return result;
}

time_t ProcReaderGsiIdentity::GetModifTime()
{
  struct tm* clock;
  struct stat attrib;

  if (stat(pGsiProxyFile.c_str(), &attrib)) {
    return 0;
  }

  clock = gmtime(&
                 (attrib.st_mtime));      // Get the last modified time and put it into the time structure
  return mktime(clock);
}

int ProcCacheEntry::UpdateIfPsChanged()
{
  ProcessInfo current;
  if(!ProcessInfoProvider::retrieveBasic(pid, current)) {
    // Looks like our pid has disappeared.
    return ESRCH;
  }

  eos::common::RWMutexWriteLock lock(pMutex);

  if(pInfo.isEmpty()) {
    // First time this function was called
    pInfo = current;
  }
  else if(!pInfo.updateIfSameProcess(current)) {
    // Looks like a different process with the same pid has replaced ours..
    // Refresh everything.

    ProcessInfo newPInfo;
    if(!ProcessInfoProvider::retrieveFull(pid, newPInfo)) {
      // Shouldn't normally happen..
      return ESRCH;
    }

    pInfo = newPInfo;
  }

  return 0;
}
