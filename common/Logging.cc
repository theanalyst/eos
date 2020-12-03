// ----------------------------------------------------------------------
// File: Logging.cc
// Author: Andreas-Joachim Peters - CERN
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

#include "common/Namespace.hh"
#include "common/Logging.hh"
#include "XrdSys/XrdSysPthread.hh"
#include <new>
#include <type_traits>
#include <atomic>

EOSCOMMONNAMESPACE_BEGIN

static std::atomic<int> sCounter {0};
static typename std::aligned_storage<sizeof(Logging), alignof(Logging)>::type
logging_buf; ///< Memory for the global logging object
Logging& gLogging = reinterpret_cast<Logging&>(logging_buf);

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
LoggingInitializer::LoggingInitializer()
{
  if (sCounter++ == 0) {
    new(&gLogging) Logging();  // placement new
  }
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
LoggingInitializer::~LoggingInitializer()
{
  if (--sCounter == 0) {
    (&gLogging)->~Logging();
  }
}

//------------------------------------------------------------------------------
// Get singleton instance
//------------------------------------------------------------------------------
Logging&
Logging::GetInstance()
{
  return gLogging;
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
Logging::Logging():
  gLogMask(0), gPriorityLevel(0), gToSysLog(false),  gUnit("none"),
  gShortFormat(0), gRateLimiter(false)
{
  // Initialize the log array and sets the log circular size
  gLogCircularIndex.resize(LOG_DEBUG + 1);
  gLogMemory.resize(LOG_DEBUG + 1);
  gCircularIndexSize = EOSCOMMONLOGGING_CIRCULARINDEXSIZE;

  for (int i = 0; i <= LOG_DEBUG; i++) {
    gLogCircularIndex[i] = 0;
    gLogMemory[i].resize(gCircularIndexSize);
  }

  gZeroVid.name = "-";
  XrdOucString tosyslog;

  if (getenv("EOS_LOG_SYSLOG")) {
    tosyslog = getenv("EOS_LOG_SYSLOG");

    if ((tosyslog == "1" ||
         (tosyslog == "true"))) {
      gToSysLog = true;
    }
  }
}

//------------------------------------------------------------------------------
// Should log function
//------------------------------------------------------------------------------
bool
Logging::shouldlog(const char* func, int priority)
{
  if (priority == LOG_SILENT) {
    return true;
  }

  // short cut if log messages are masked
  if (!((LOG_MASK(priority) & gLogMask))) {
    return false;
  }

  // apply filter to avoid message flooding for debug messages
  if (priority >= LOG_INFO) {
    if (gDenyFilter.Num()) {
      // this is a normal filter by function name
      if (gDenyFilter.Find(func)) {
        return false;
      }
    }
  }

  return true;
}


Logging::log_buffer *
Logging::log_alloc_buffer() {
    Logging::log_buffer *buff = NULL;

    /* log_buffer_balance is incorrect until we really allocated a buffer! */
    Logging::log_buffer_balance.fetch_add(1);

    while (true) {
        while (true) {
            if ((buff=free_buffers) == NULL) break;
            if (free_buffers.compare_exchange_weak(buff, buff->h.next)) break;
        }

        if (buff != NULL) {
            log_buffer_free.fetch_add(-1);
            break;
        }

        /* no free buffer, alloc new one if below budget, or wait */
        if ( Logging::log_buffer_total < Logging::max_log_buffers ) {
            buff = (struct log_buffer *) malloc(sizeof(struct log_buffer));

            std::lock_guard<std::mutex> guard(log_mutex);
            log_buffer_total++;
            
            {
                /* consistency checks, to be removed*/
                int num_in_queue;
                Logging::log_buffer *bx = active_head, *bbx;
                for (num_in_queue=0; bx != NULL; num_in_queue++) {
                    bbx = bx->h.next;
                    if (bx == bbx) {
                        fprintf(stderr, "%s:%d active log_buffer_loop!\n", __FILE__, __LINE__);
                        bx->h.next = NULL;
                        break;
                    }
                    bx = bbx;
                }
                if (num_in_queue != log_buffer_in_q)
                    fprintf(stderr, "%s:%d wrong log_buffer_in_q: %d != %d\n", __FILE__, __LINE__,
                            num_in_queue, log_buffer_in_q);
            
            
                fprintf(stderr, "\ntotal_log_buffers: %d balance %d in_q %d free %d waiters %d\n",
                        log_buffer_total, log_buffer_balance.load(),
                        log_buffer_in_q,
                        log_buffer_free.load(),
                        log_buffer_waiters);
            }
            break;
        }

        /* wait for a free buffer */
        if (free_buffers == NULL and (log_buffer_num_waits++ & 0x1f) == 0)
                fprintf(stderr,
                    "log_buffer_waiters shortage #%d with %d waiters, total_log_buffers %d balance %d in_q %d free %d\n",
                    log_buffer_num_waits, log_buffer_waiters,
                    Logging::log_buffer_total, Logging::log_buffer_balance.load(),
                    log_buffer_in_q, log_buffer_free.load());

        {
            std::lock_guard<std::mutex> guard(log_buffer_shortage_mutex);

            Logging::log_buffer_waiters++;  /* this asks for a wake-up call when a buffer is freed */
            log_buffer_shortage.wait(log_buffer_shortage_mutex);
            Logging::log_buffer_waiters--;

            /* retry... */
            continue;
        }

    }

    buff->h.next = NULL;
    buff->h.fanOutBuffer = NULL;


    return buff;
}


void
Logging::log_return_buffers(Logging::log_buffer *buff) {
    Logging::log_buffer *buff2 = buff, *buff3;

    /* count number of buffers returned */
    int n = 1;
    for (buff2 = buff; (buff3 = buff2->h.next) != NULL; buff2 = buff3) {
        if (buff3 == buff2) {
            fprintf(stderr, "%s:%d log_buffer_loop returning circular buffer list, cut\n", __FILE__, __LINE__);
            buff2->h.next = NULL;
            break;
        }
        n++;
    }

    if (log_buffer_free + n > log_buffer_total) {   /* Something's wrong, check all chains thoroughly */
        fprintf(stderr, "%s:%d log_buffer_loop log_buffer_free %d > log_buffer_total %d, %d buffers returned\n",
                __FILE__, __LINE__, log_buffer_free.load(), log_buffer_total, n);
        buff3->h.next = NULL;
        {
            Logging::log_buffer *buff4, *buff5;
            std::lock_guard<std::mutex> guard(log_mutex);

            for (buff2=buff; (buff3=buff2->h.next) != NULL; buff2 = buff3) { /* buff2 -> buff3 */

                /* the "2bfreed" chain should be short, the following scales n**2 */
                for (buff4 = buff; (buff5 = buff4->h.next) != buff3; buff4 = buff5) {   /* buff4 -> buff5 */
                    /* buff5 == buff3, see if parents match */
                    if (buff2 != buff4) {
                        fprintf(stderr, "%s:%d log_buffer_loop free buffer %p twice on chain, cut\n",  __FILE__, __LINE__, buff3);
                        buff3->h.next = NULL;
                        break;
                    }
                }

                for (buff4 = active_head; (buff5=buff4->h.next) != NULL; buff4 = buff5) {
                    if (buff4 == buff2) {
                        fprintf(stderr, "%s:%d log_buffer_loop free buffer %p on active chain, cut\n",  __FILE__, __LINE__, buff2);
                        buff4->h.next = NULL;
                        break;
                    }
                }
                if (active_tail->h.next != NULL) {
                    fprintf(stderr, "%s:%d log_buffer_loop active_tail->h.next should be NULL, cutting\n",  __FILE__, __LINE__);
                    active_tail->h.next = NULL;
                }
                if (buff4 != NULL && buff4 != active_tail) {
                    fprintf(stderr, "%s:%d log_buffer_loop last buffer does not match active_tail, corrected\n",  __FILE__, __LINE__);
                    active_tail = buff4;
                }

            }
        }
    }


    while (true) {
        buff2->h.next = free_buffers;
        if (free_buffers.compare_exchange_weak(buff2->h.next, buff)) break;
    }

    log_buffer_free.fetch_add(n);

    {
        std::lock_guard<std::mutex> guard(log_buffer_shortage_mutex);

        if (log_buffer_waiters > 0)     /* This is the condition the CV protects */
            log_buffer_shortage.notify_all();
    }

}

void
Logging::log_queue_buffer(Logging::log_buffer *buff) {
    std::lock_guard<std::mutex> guard(log_mutex);

    if (log_thread_p == NULL) {
        log_thread_p = new std::thread([this] { log_thread(); });
    }

    Logging::log_buffer *prev;

    if (active_tail == NULL) {
        /* the following works because offset(next) == 0 */
        prev = (Logging::log_buffer *) &active_tail;
    } else 
        prev = active_tail;

    buff->h.next = NULL;
    prev->h.next = buff;

    active_tail = buff;
    if (!active_head) active_head = buff;

    log_buffer_in_q++;

    /* log_buffer_balance designates buffers between intended allocation and print queueing */
    log_buffer_balance.fetch_add(-1);

    log_cond.notify_one();
}


void
Logging::log_thread() {
    Logging::log_buffer *buff = NULL, *buff_2b_returned=NULL;

    log_mutex.lock();
    int notify_counter = 0;
    while(1) {
        if ( active_head == NULL or log_buffer_balance > 50 or log_buffer_waiters > 0 ) {
            if (buff_2b_returned != NULL) {
                log_mutex.unlock();
                log_return_buffers(buff_2b_returned);
                log_mutex.lock();

                buff_2b_returned = NULL;
                continue;
            }

            if (active_head == NULL) {
                    if (++notify_counter > 100) {
                        notify_counter = 0;

                        fprintf(stderr, "\nlog_buffer queue empty, log_buffer_total: %d balance %d free %d waits %d waiters %d\n",
                            log_buffer_total, log_buffer_balance.load(), log_buffer_free.load(),
                            log_buffer_num_waits, log_buffer_waiters);
                    }
                log_cond.wait(log_mutex);
            }
        }

        if (active_head) {
            buff = active_head;
            active_head = active_head->h.next;
            log_buffer_in_q--;

            if (active_head == NULL) active_tail = NULL;

            log_mutex.unlock();

            fprintf(stderr, "%s\n", buff->buffer);
            if (active_head == NULL) fflush(stderr);        /* don't flush if there's yet another buffer */

            if (gToSysLog) {
              syslog(buff->h.priority, "%s", buff->h.ptr);
            }

            if (buff->h.fanOutBuffer != NULL) {
                if (buff->h.fanOutS != NULL) {
                    fputs(buff->h.fanOutBuffer, buff->h.fanOutS);
                    fflush(buff->h.fanOutS);
                }
                if (buff->h.fanOut != NULL) {
                    fputs(buff->h.fanOutBuffer, buff->h.fanOut);
                    fflush(buff->h.fanOut);
                }
            }

            if (buff_2b_returned != buff) {
                        
            buff->h.next = buff_2b_returned;
            buff_2b_returned = buff;

            } else 
                fprintf(stderr, "%s.%d log_buffer_loop returning returned log_buffer\n", __FILE__, __LINE__);
            

            log_mutex.lock();
        }
    }
}

//------------------------------------------------------------------------------
// Logging function
//------------------------------------------------------------------------------
const char*
Logging::log(const char* func, const char* file, int line, const char* logid,
             const VirtualIdentity& vid, const char* cident, int priority,
             const char* msg, ...)
{

  bool silent = (priority == LOG_SILENT);
  int rc = 0;

  // short cut if log messages are masked
  if (!silent && !((LOG_MASK(priority) & gLogMask))) {
    return "";
  }

  // apply filter to avoid message flooding for debug messages
  if (!silent && priority >= LOG_INFO) {
    if (gAllowFilter.Num()) {
      // if this is a pass-through filter e.g. we want to see exactly this messages
      if (!gAllowFilter.Find(func)) {
        return "";
      }
    } else if (gDenyFilter.Num()) {
      // this is a normal filter by function name
      if (gDenyFilter.Find(func)) {
        return "";
      }
    }
  }

  struct log_buffer *logBuffer = log_alloc_buffer();
  char* buffer = logBuffer->buffer;

  XrdOucString File = file;
  // we show only one hierarchy directory like Acl (assuming that we have only
  // file names like *.cc and *.hh
  File.erase(0, File.rfind("/") + 1);
  File.erase(File.length() - 3);
  static time_t current_time;
  static struct timeval tv;
  static struct timezone tz;
  struct tm tm;
  va_list args;
  va_start(args, msg);
  current_time = tv.tv_sec;
  static char linen[16];
  sprintf(linen, "%d", line);
  static char fcident[1024];
  XrdOucString truncname = vid.name;

  // we show only the last 16 bytes of the name
  if (truncname.length() > 16) {
    truncname.insert("..", 0);
    truncname.erase(0, truncname.length() - 16);
  }

  char sourceline[64];

  gettimeofday(&tv, &tz);

  if (gShortFormat) {
    localtime_r(&current_time, &tm);
    snprintf(sourceline, sizeof(sourceline) - 1, "%s:%s", File.c_str(), linen);
    XrdOucString slog = logid;

    if (slog.beginswith("logid:")) {
      slog.erase(0, 6);
      sprintf(buffer,
              "%02d%02d%02d %02d:%02d:%02d t=%lu.%06lu f=%-16s l=%s %s s=%-24s ",
              tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
              tm.tm_min, tm.tm_sec, current_time, (unsigned long) tv.tv_usec,
              func, GetPriorityString(priority), slog.c_str(), sourceline);
    } else {
      sprintf(buffer,
              "%02d%02d%02d %02d:%02d:%02d t=%lu.%06lu f=%-16s l=%s tid=%016lx s=%-24s ",
              tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
              tm.tm_min, tm.tm_sec, current_time, (unsigned long) tv.tv_usec,
              func, GetPriorityString(priority), (unsigned long) XrdSysThread::ID(),
              sourceline);
    }
  } else {
    sprintf(fcident, "tident=%s sec=%-5s uid=%d gid=%d name=%s geo=\"%s\"", cident,
            vid.prot.c_str(), vid.uid, vid.gid, truncname.c_str(), vid.geolocation.c_str());
    localtime_r(&current_time, &tm);
    snprintf(sourceline, sizeof(sourceline) - 1, "%s:%s", File.c_str(), linen);
    sprintf(buffer,
            "%02d%02d%02d %02d:%02d:%02d time=%lu.%06lu func=%-24s level=%s logid=%s unit=%s tid=%016lx source=%-30s %s ",
            tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
            tm.tm_sec, current_time, (unsigned long) tv.tv_usec, func,
            GetPriorityString(priority), logid, gUnit.c_str(),
            (unsigned long) XrdSysThread::ID(), sourceline, fcident);
  }

  char* ptr = buffer + strlen(buffer);
  // limit the length of the output to buffer-1 length
  vsnprintf(ptr, logmsgbuffersize - (ptr - buffer + 1), msg, args);

  if (!silent && rate_limit(tv, priority, file, line)) {
    log_return_buffers(logBuffer);
    return "";
  }
  

  logBuffer->h.ptr = ptr;

  if (!silent) {
    if (gLogFanOut.size()) {
      logBuffer->h.fanOutS = NULL;
      logBuffer->h.fanOut = NULL;

      logBuffer->h.fanOutBuffer = ptr + strlen(ptr) + 1;
      logBuffer->h.fanOutBufLen = logmsgbuffersize - (logBuffer->h.fanOutBuffer-logBuffer->buffer);
      
      // we do log-message fanout
      if (gLogFanOut.count("*")) {
        logBuffer->h.fanOutS = gLogFanOut["*"];
        if (snprintf(logBuffer->h.fanOutBuffer, logBuffer->h.fanOutBufLen, "%s\n", logBuffer->buffer) < 0) {
            rc = 42;    /*truncated - results in a warning if not handled */
        }
      }

      if (gLogFanOut.count(File.c_str())) {
        logBuffer->buffer[15] = 0;
        logBuffer->h.fanOut = gLogFanOut[File.c_str()];
        if (snprintf(logBuffer->h.fanOutBuffer, logBuffer->h.fanOutBufLen,
                "%s %s%s%s %-30s %s \n",
                logBuffer->buffer,
                GetLogColour(GetPriorityString(priority)),
                GetPriorityString(priority),
                EOS_TEXTNORMAL,
                sourceline,
                logBuffer->h.ptr) < 0 ) {
            /* has been truncated, not an issue */
            rc = 43;
        }
        logBuffer->buffer[15] = ' ';
      } else {
        if (gLogFanOut.count("#")) {
          logBuffer->buffer[15] = 0;
          logBuffer->h.fanOut = gLogFanOut["#"];
          if (snprintf(logBuffer->h.fanOutBuffer, logBuffer->h.fanOutBufLen,
                  "%s %s%s%s [%05d/%05d] %16s ::%-16s %s \n",
                  logBuffer->buffer,
                  GetLogColour(GetPriorityString(priority)),
                  GetPriorityString(priority),
                  EOS_TEXTNORMAL,
                  vid.uid,
                  vid.gid,
                  truncname.c_str(),
                  func,
                  logBuffer->h.ptr
                 ) < 0 ) {
              rc = 44;
          }
          logBuffer->buffer[15] = ' ';
        }
      }

    }

  }

  va_end(args);
  const char* rptr;

  if (silent) {
    priority = LOG_DEBUG;
  }

  // store into global log memory
  { 
    XrdSysMutexHelper scope_lock(gMutex);         

    /* the following copies the buffer, hence it can be queued and vanish anytime after */
    gLogMemory[priority][(gLogCircularIndex[priority]) % gCircularIndexSize] =
      buffer;
    rptr = gLogMemory[priority][(gLogCircularIndex[priority]) %
                              gCircularIndexSize].c_str();
    gLogCircularIndex[priority]++;
  }

  logBuffer->h.priority = priority;
  log_queue_buffer(logBuffer);

  return rptr;
}

bool
Logging::rate_limit(struct timeval& tv, int priority, const char* file,
                    int line)
{
  static bool do_limit = false;
  static std::string last_file = "";
  static int last_line = 0;
  static int last_priority = priority;
  static struct timeval last_tv;

  if (!gRateLimiter) {
    return false;
  }

  if ((line == last_line) &&
      (priority == last_priority) &&
      (last_file == file) &&
      (priority < LOG_WARNING)) {
    float elapsed = (1.0 * (tv.tv_sec - last_tv.tv_sec)) - ((
                      tv.tv_usec - last_tv.tv_usec) / 1000000.0);

    if (elapsed < 5.0) {
      if (!do_limit) {
        fprintf(stderr,
                "                 ---- high rate error messages suppressed ----\n");
      }

      do_limit = true;
    } else {
      do_limit = false;
    }
  } else {
    do_limit = false;
  }

  if (!do_limit) {
    last_tv = tv;
    last_line = line;
    last_file = file;
    last_priority = priority;
  }

  return do_limit;
}

EOSCOMMONNAMESPACE_END
