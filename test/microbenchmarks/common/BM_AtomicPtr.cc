#include "common/concurrency/AtomicUniquePtr.h"
#include "benchmark/benchmark.h"
#include <memory>

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

BENCHMARK(BM_AtomicUniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK(BM_UniquePtrGet)->ThreadRange(1, 64)->UseRealTime();
BENCHMARK_MAIN();