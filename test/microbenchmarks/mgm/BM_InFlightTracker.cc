#include "benchmark/benchmark.h"
#include "mgm/InFlightTracker.hh"

static void BM_InFlightTrackerCreate(benchmark::State& state)
{
  eos::mgm::InFlightTracker tracker;
  eos::common::VirtualIdentity vid;

  for (auto _: state){
    eos::mgm::InFlightRegistration registerer(tracker,vid);
    benchmark::DoNotOptimize(registerer.IsOK());
  }
  state.counters["frequency"] = benchmark::Counter(state.iterations(),
                                                   benchmark::Counter::kIsRate);

}

static void BM_InFlightTrackerCreateMT(benchmark::State& state)
{
  eos::mgm::InFlightTracker tracker;
  eos::common::VirtualIdentity vid;

  for (auto _: state) {
    for (auto i = 0; i < state.range(0); ++i) {
      vid.uid = i & 2147483647UL;
      vid.gid = vid.uid;
      eos::mgm::InFlightRegistration registerer(tracker,vid);
      benchmark::DoNotOptimize(registerer.IsOK());
      benchmark::ClobberMemory();

    }
  }
  state.counters["frequency"] = benchmark::Counter(state.iterations(),
                                                   benchmark::Counter::kIsRate);
  state.counters["per_thread"] = benchmark::Counter(state.iterations(),
                                                   benchmark::Counter::kAvgThreadsRate);
}

static void BM_InFlightTrackerStallMT(benchmark::State& state)
{
  eos::mgm::InFlightTracker tracker;
  eos::common::VirtualIdentity vid;

  for (auto _: state) {
    for (auto i = 0; i < state.range(0); ++i) {
      uid_t uid  = i & 2147483647UL;
      vid.uid = uid;
      vid.gid = uid;
      bool sat;
      size_t thread_count=1;
      eos::mgm::InFlightRegistration registerer(tracker,vid);
      benchmark::DoNotOptimize(registerer.IsOK());
      benchmark::DoNotOptimize(tracker.ShouldStall(uid, sat,thread_count));
      benchmark::ClobberMemory();

    }
  }
  state.counters["frequency"] = benchmark::Counter(state.iterations(),
                                                   benchmark::Counter::kIsRate);
  state.counters["per_thread"] = benchmark::Counter(state.iterations(),
                                                    benchmark::Counter::kAvgThreadsRate);
}

BENCHMARK(BM_InFlightTrackerCreate);
BENCHMARK(BM_InFlightTrackerCreateMT)->Range(1,512)->ThreadRange(1, 256)->UseRealTime();
BENCHMARK(BM_InFlightTrackerStallMT)->Range(1024,8192)->ThreadRange(1, 256)->UseRealTime();
BENCHMARK_MAIN();
