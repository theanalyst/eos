#include "common/concurrency/AtomicUniquePtr.h"
#include "common/concurrency/RCULite.hh"
#include "benchmark/benchmark.h"
#include <memory>
#include <shared_mutex>

static void BM_AtomicUniquePtrGet(benchmark::State& state)
{
  eos::common::atomic_unique_ptr<std::string> p(new std::string("foobar"));
  std::string *x;
  for (auto _ : state) {
    benchmark::DoNotOptimize(x = p.get());
    benchmark::ClobberMemory();
  }
}

static void BM_UniquePtrGet(benchmark::State& state)
{
  std::unique_ptr<int> p(new int(1));
  int *x;
  for (auto _ : state) {
    benchmark::DoNotOptimize( x = p.get());
  }
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
}

BENCHMARK(BM_AtomicUniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_UniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_MutexLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_SharedMutexLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_RCUReadLock)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK_MAIN();