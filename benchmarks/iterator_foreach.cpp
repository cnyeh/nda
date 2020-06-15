#include "./bench_common.hpp"

const int N1 = 1000, N2 = 1000;

// -------------------------------- 1d ---------------------------------------

static void for1(benchmark::State &state) {
  nda::array<double, 1> a(N1);

  const long l0 = a.indexmap().lengths()[0];

  while (state.KeepRunning()) {
    //   for (long i=0; i <a.indexmap().lengths()[0]; ++i) { benchmark::DoNotOptimize(a(i) = 10 * i); }
    for (long i = 0; i < l0; ++i) { benchmark::DoNotOptimize(a(i) = 10 * i); }
  }
}
BENCHMARK(for1);

static void foreach1(benchmark::State &state) {
  nda::array<double, 1> a(N1);

  while (state.KeepRunning()) {
    nda::for_each(a.shape(), [&a](auto x0) { benchmark::DoNotOptimize(a(x0) = 10 * x0); });
  }
}
BENCHMARK(foreach1);

static void iterators1(benchmark::State &state) {
  nda::array<double, 1> a(N1);

  while (state.KeepRunning()) {
    long i = 0;
    for (auto it = a.begin(); it != a.end(); ++it, ++i) { benchmark::DoNotOptimize(*it = 10 * i); }
  }
}
BENCHMARK(iterators1);

static void pointer_1A(benchmark::State &state) {
  nda::array<double, 1> a(N1);

  const long l0 = a.indexmap().lengths()[0];

  while (state.KeepRunning()) {
    double *__restrict__ p = &(a(0));
    for (long i0 = 0; i0 < l0; ++i0) benchmark::DoNotOptimize(p[i0 * a.indexmap().strides()[0]] = 10 * i0);
  }
}
BENCHMARK(pointer_1A);

static void pointer_1B(benchmark::State &state) {
  nda::array<double, 1> a(N1);

  //const long s0 = a.indexmap().strides()[0];
  const long l0 = a.indexmap().lengths()[0];

  while (state.KeepRunning()) {
    double *__restrict__ p = &(a(0));
    for (long i0 = 0; i0 < l0; ++i0) benchmark::DoNotOptimize(p[i0] = 10 * i0);
  }
}
BENCHMARK(pointer_1B);

// ------------------------------- 2d ----------------------------------------

static void for2(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);
  const long l0 = a.indexmap().lengths()[0];
  const long l1 = a.indexmap().lengths()[1];

  while (state.KeepRunning()) {
    for (long i = 0; i < l0; ++i)
      for (long j = 0; j < l1; ++j) { benchmark::DoNotOptimize(a(i, j) = 10); }
  }
}
BENCHMARK(for2);

static void foreach2(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);

  while (state.KeepRunning()) {
    nda::for_each(a.shape(), [&a](auto x0, auto x1) { benchmark::DoNotOptimize(a(x0, x1) = 10); });
  }
}
BENCHMARK(foreach2);

static void iterators2(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);
  while (state.KeepRunning()) {
    // quite slower
    //for (auto it = a.begin(); it != a.end(); ++it) { benchmark::DoNotOptimize(*it = 10); }
    for (auto &x : a) { benchmark::DoNotOptimize(x = 10); }
  }
}
BENCHMARK(iterators2);

static void iterators2_strided(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);
  nda::basic_array_view<double, 2, nda::C_stride_layout> v(a);
  while (state.KeepRunning()) {
    for (auto &x : v) { benchmark::DoNotOptimize(x = 10); }
  }
}
BENCHMARK(iterators2_strided);

//static void iterators2strided(benchmark::State &state) {
//nda::array<double, 2> a(2*N1, 2*N2);
//auto v = a (range(0, -1, 2), range (0, -1, 2));
//while (state.KeepRunning()) {
//for (auto & x: v) { benchmark::DoNotOptimize(x = 10); }
//}
//}
//BENCHMARK(iterators2strided);

static void pointer_2A(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);

  const long l0 = a.indexmap().lengths()[0];
  const long l1 = a.indexmap().lengths()[1];
  // auto len =  a.indexmap().lengths();
  auto str = a.indexmap().strides();
  while (state.KeepRunning()) {
    double *p = &(a(0, 0));
    for (long i0 = 0; i0 < l0; ++i0)
      for (long i1 = 0; i1 < l1; ++i1) benchmark::DoNotOptimize(p[i0 * str[0] + i1 * str[1]] = 10);
  }
}
BENCHMARK(pointer_2A);

static void pointer_2B(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);

  const long s0 = a.indexmap().strides()[0];
  const long s1 = a.indexmap().strides()[1];
  const long l0 = a.indexmap().lengths()[0];
  const long l1 = a.indexmap().lengths()[1];

  while (state.KeepRunning()) {
    double *p = &(a(0, 0));
    for (long i0 = 0; i0 < l0; ++i0)
      for (long i1 = 0; i1 < l1; ++i1) benchmark::DoNotOptimize(p[i0 * s0 + i1 * s1] = 10);
  }
}
BENCHMARK(pointer_2B);

static void pointer_2C(benchmark::State &state) {
  nda::array<double, 2> a(N1, N2);

  const long l0   = a.indexmap().lengths()[0];
  const long l1   = a.indexmap().lengths()[1];
  const long l0l1 = l0 * l1;

  while (state.KeepRunning()) {
    double *p = &(a(0, 0));
    for (long i = 0; i < l0l1; ++i) benchmark::DoNotOptimize(p[i] = 10);
  }
}
BENCHMARK(pointer_2C);
