#include <benchmark/benchmark.h>
#include <ice/async.h>
#include <ice/context.h>

ice::task context_schedule_task(ice::context& context, benchmark::State& state, bool queue) {
  for (auto _ : state) {
    co_await context.schedule(queue);
  }
  context.stop();
}

static void context_schedule(benchmark::State& state) {
  ice::context context;
  context_schedule_task(context, state, false);
  context.run();
}
BENCHMARK(context_schedule);

static void context_schedule_queue(benchmark::State& state) {
  ice::context context;
  context_schedule_task(context, state, true);
  context.run();
}
BENCHMARK(context_schedule_queue);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
}
