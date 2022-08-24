#include "benchmark/benchmark.h"
#include "common/StringUtils.hh"

static void BM_StringToNumeric(benchmark::State& state)
{
  int val;
  std::string s = std::to_string(state.range(0));
  using namespace eos::common;
  for (auto _: state) {
    benchmark::DoNotOptimize(StringToNumeric(s, val));
  }
}


static void BM_atoi(benchmark::State& state)
{
  int val;
  std::string s = std::to_string(state.range(0));
  using namespace eos::common;
  for (auto _: state) {
    benchmark::DoNotOptimize(val = atoi(s.c_str()));
  }
}

int64_t start = 8;
int64_t end = 1UL<<24;
BENCHMARK(BM_StringToNumeric)->Range(start,end);
BENCHMARK(BM_atoi)->Range(start,end);
BENCHMARK_MAIN();
