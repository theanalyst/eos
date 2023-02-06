#include "common/concurrency/AtomicUniquePtr.h"
#include "common/concurrency/RCULite.hh"
#include "benchmark/benchmark.h"
#include <memory>
#include <shared_mutex>
#include <mutex>

using benchmark::Counter;

static void BM_AtomicUniquePtrGet(benchmark::State& state)
{
  eos::common::atomic_unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    benchmark::DoNotOptimize(x = p.get());
    benchmark::ClobberMemory();
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_UniquePtrGet(benchmark::State& state)
{
  std::unique_ptr<int> p(new int(1));
  int *x;
  for (auto _ : state) {
    benchmark::DoNotOptimize( x = p.get());
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_MutexLock(benchmark::State& state)
{
  std::mutex m;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    std::lock_guard<std::mutex> lock(m);
    benchmark::DoNotOptimize(x=p.get());
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_SharedMutexLock(benchmark::State& state)
{
  std::shared_mutex m;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    std::shared_lock<std::shared_mutex> lock(m);
    benchmark::DoNotOptimize(x=p.get());
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_RCUReadLock(benchmark::State& state)
{
  eos::common::RCUDomain rcu_domain;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    auto tid = rcu_domain.rcu_read_lock();
    benchmark::DoNotOptimize(x=p.get());
    rcu_domain.rcu_read_unlock(tid);
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_RCUVersionReadLock(benchmark::State& state)
{
  eos::common::VersionedRCUDomain rcu_domain;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    auto tid = rcu_domain.rcu_read_lock();
    benchmark::DoNotOptimize(x=p.get());
    rcu_domain.rcu_read_unlock_index(tid);
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);

}

static void BM_MutexRWLock(benchmark::State& state)
{
  std::mutex m;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  auto writer_fn = [&p, &m] {
    for (int i = 0; i < 10000; i++) {
      std::lock_guard<std::mutex> lock(m);
      p.reset(new std::string("foobar2"));
    }
  };

  std::thread writer;
  if (state.thread_index() == 0) {
    writer = std::thread(writer_fn);
  }

  for (auto _ : state) {
    std::lock_guard<std::mutex> lock(m);
    benchmark::DoNotOptimize(x=p.get());
  }

  if (state.thread_index() == 0) {
    writer.join();
  }
  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_SharedMutexRWLock(benchmark::State& state)
{
  std::shared_mutex m;
  std::unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  auto writer_fn = [&p, &m] {
    for (int i = 0; i < 10000; i++) {
      std::unique_lock<std::shared_mutex> lock(m);
      p.reset(new std::string("foobar2"));
    }
  };

  std::thread writer;
  if (state.thread_index() == 0) {
    writer = std::thread(writer_fn);
  }

  for (auto _ : state) {
    std::shared_lock<std::shared_mutex> lock(m);
    benchmark::DoNotOptimize(x=p.get());
  }

  if (state.thread_index() == 0) {
    writer.join();
  }

  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_RCUReadWriteLock(benchmark::State& state)
{
  eos::common::RCUDomain rcu_domain;
  eos::common::atomic_unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  auto writer_fn = [&p, &rcu_domain] {
    for (int i=0; i < 10000; ++i) {
      auto x = p.reset(new std::string("foobar2"));
      rcu_domain.rcu_synchronize();
      delete x;
    }
  };

  std::thread writer;
  if (state.thread_index() == 0) {
    writer = std::thread(writer_fn);
  }

  for (auto _ : state) {
    auto tid = rcu_domain.rcu_read_lock();
    benchmark::DoNotOptimize(x=p.get());
    rcu_domain.rcu_read_unlock();
  }

  if (state.thread_index() == 0) {
    if (writer.joinable())
      writer.join();
  }

  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

static void BM_RCUVersionedReadWriteLock(benchmark::State& state)
{
  eos::common::VersionedRCUDomain rcu_domain;
  eos::common::atomic_unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  auto writer_fn = [&p, &rcu_domain] {
    for (int i=0; i < 10000; ++i) {
      auto x = p.reset(new std::string("foobar2"));
      rcu_domain.rcu_synchronize();
      delete x;
    }
  };

  std::thread writer;
  if (state.thread_index() == 0) {
    writer = std::thread(writer_fn);
  }

  for (auto _ : state) {
    auto index = rcu_domain.rcu_read_lock();
    benchmark::DoNotOptimize(x=p.get());
    rcu_domain.rcu_read_unlock_index(index);
  }

  if (state.thread_index() == 0) {
    if (writer.joinable())
      writer.join();
  }

  state.counters["frequency"] = Counter(state.iterations(),
                                        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_AtomicUniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_UniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_MutexLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_SharedMutexLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_RCUReadLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_RCUVersionReadLock)->ThreadRange(1,64)->UseRealTime();

BENCHMARK(BM_MutexRWLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_SharedMutexRWLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_RCUReadWriteLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_RCUVersionedReadWriteLock)->ThreadRange(1,64)->UseRealTime();
BENCHMARK_MAIN();
