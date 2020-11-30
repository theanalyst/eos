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
    int balance, tempInt, num_log_buffers;

    while (true) {
        while (true) {
            if ((buff=free_buffers) == NULL) break;
            if (free_buffers.compare_exchange_weak(buff, buff->h.next)) break;
        }

        if (buff != NULL) break;

        /* no free buffer, alloc new one if below budget, or wait */
        if ( Logging::log_buffer_total.load() < max_log_buffers ) {
            buff = (struct log_buffer *) malloc(sizeof(struct log_buffer));

                int num_in_queue;
                {
                    std::lock_guard<std::mutex> guard(log_mutex);
                    Logging::log_buffer *bx = active_head;
                    for (num_in_queue=0; bx != NULL; num_in_queue++) bx = bx->h.next;
                }


            do {
                num_log_buffers = Logging::log_buffer_total;
                tempInt = num_log_buffers+1;
            } while(!Logging::log_buffer_total.compare_exchange_weak(num_log_buffers, tempInt));
            
            fprintf(stderr, "\ntotal_log_buffers: %d, balance %d in_queue %d waiters %d\n",
                    Logging::log_buffer_total.load(), Logging::log_buffer_balance.load(), num_in_queue, Logging::log_buffer_waiters);
            break;
        }

        /* wait for a free buffer */
        {
            int waiters = Logging::log_buffer_waiters;
            std::lock_guard<std::mutex> guard(log_mutex);
            Logging::log_buffer *bx = active_head;
            int num_in_queue;
            for (num_in_queue=0; bx != NULL; num_in_queue++) bx = bx->h.next;
            fprintf(stderr, "log_buffer_waiters waiting with %d waiters, total_log_buffers: %d balance %d in queue %d\n", waiters, 
                    Logging::log_buffer_total.load(), Logging::log_buffer_balance.load(), num_in_queue);

            Logging::log_buffer_waiters++;
            log_buffer_shortage.wait(log_mutex);
            Logging::log_buffer_waiters--;
            fprintf(stderr, "log_buffer_waiters again %d\n", Logging::log_buffer_waiters);

            /* retry... */
            continue;
        }

    }

    buff->h.next = NULL;
    buff->h.fanOutBuff = NULL;

    /* In this small window the log_buffer_balance is incorrect! */
    do {
        balance = Logging::log_buffer_balance;
        tempInt = balance+1;
    } while(!Logging::log_buffer_balance.compare_exchange_weak(balance, tempInt));


    return buff;
}


void
Logging::log_return_buffers(Logging::log_buffer *buff) {
    Logging::log_buffer *buff2 = buff, *buff3;
    int balance, balance1;

    int n = 1;
    for (buff2 = buff; (buff3 = buff2->h.next) != NULL; buff2 = buff3) n++;

    while (true) {
        buff2->h.next = free_buffers;
        if (free_buffers.compare_exchange_weak(buff2->h.next, buff)) break;
    }

    /* In this small window the log_buffer_balance is incorrect! */
    do {
        balance = log_buffer_balance;
        balance1 = balance-n;
    } while(!Logging::log_buffer_balance.compare_exchange_weak(balance, balance1));

    if (log_buffer_waiters > 0)         /* this is not checked under a lock - it doesn't really matter */
        log_buffer_shortage.notify_one();

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

    log_cond.notify_one();
}


void
Logging::log_thread() {
    Logging::log_buffer *buff = NULL, *buff_2b_returned=NULL;

    log_mutex.lock();
int notify_counter = 0;
    while(1) {
        if (active_head == NULL || Logging::log_buffer_balance > 100 || Logging::log_buffer_waiters > 0) {
            if (buff_2b_returned != NULL) {
                log_mutex.unlock();
                log_return_buffers(buff_2b_returned);
                log_mutex.lock();

                buff_2b_returned = NULL;
                continue;
            }

            if (active_head == NULL) {
                log_cond.wait(log_mutex);
if (++notify_counter > 100) {
    notify_counter = 0;

    Logging::log_buffer *fb, *free_start;
    int num_free, retries;
    for (retries=0; retries < 4096; retries++) {
        fb = free_buffers;
        num_free = 0;
        for (free_start = fb; (fb != NULL) and (free_start == free_buffers); num_free++)
            fb = fb->h.next;
        if (free_start == free_buffers) break;
    }
        
    fprintf(stderr, "\nlog_buffer queue empty, log_buffer_total: %d, balance %d num_free = %d retries %d waiters %d\n",
                Logging::log_buffer_total.load(), Logging::log_buffer_balance.load(), num_free, retries, log_buffer_waiters);
}

            }
        }

        if (active_head) {
            buff = active_head;
            active_head = active_head->h.next;
            if (active_head == NULL) active_tail = NULL;

            log_mutex.unlock();

            fprintf(stderr, "%s\n", buff->buffer);
            if (active_head == NULL) fflush(stderr);        /* don't flush if there's yet another buffer */

            if (gToSysLog) {
              syslog(buff->h.priority, "%s", buff->h.ptr);
            }

            if (buff->h.fanOutBuff != NULL) {
                if (buff->h.fanOutS != NULL) {
                    fputs(buff->h.fanOutBuff->buffer, buff->h.fanOutS);
                    fflush(buff->h.fanOutS);
                }
                if (buff->h.fanOut != NULL) {
                    fputs(buff->h.fanOutBuff->buffer, buff->h.fanOut);
                    fflush(buff->h.fanOut);
                }

                (buff->h.fanOutBuff)->h.next = buff_2b_returned;
                buff_2b_returned = buff->h.fanOutBuff;
            }
                        
            buff->h.next = buff_2b_returned;
            buff_2b_returned = buff;

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
  assert(logBuffer->h.fanOutBuff == NULL);
  if ( (!silent) && gLogFanOut.size() > 0) {
      logBuffer->h.fanOutBuff = log_alloc_buffer();
  }
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
  XrdSysMutexHelper scope_lock(gMutex);
  va_list args;
  va_start(args, msg);
  gettimeofday(&tv, &tz);
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
      
      // we do log-message fanout
      if (gLogFanOut.count("*")) {
        logBuffer->h.fanOutS = gLogFanOut["*"];
        if (snprintf(logBuffer->h.fanOutBuff->buffer, logmsgbuffersize, "%s\n", logBuffer->buffer) < 0) {
            rc = 42;    /*truncated - results in a warning if not handled */
        }
        //fflush(gLogFanOut["*"]);
      }

      if (gLogFanOut.count(File.c_str())) {
        logBuffer->buffer[15] = 0;
        logBuffer->h.fanOut = gLogFanOut[File.c_str()];
        if (snprintf(logBuffer->h.fanOutBuff->buffer, logmsgbuffersize, "%s %s%s%s %-30s %s \n",
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
          if (snprintf(logBuffer->h.fanOutBuff->buffer, logmsgbuffersize, "%s %s%s%s [%05d/%05d] %16s ::%-16s %s \n",
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
  gLogMemory[priority][(gLogCircularIndex[priority]) % gCircularIndexSize] =
    buffer;
  rptr = gLogMemory[priority][(gLogCircularIndex[priority]) %
                              gCircularIndexSize].c_str();
  gLogCircularIndex[priority]++;

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
