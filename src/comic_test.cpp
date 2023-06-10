#include "comic.hpp"

#include <benchmark/benchmark.h>
#include <wx/image.h>

#define runner(X)                                                \
	void BM_##X(benchmark::State& state) {                       \
		for (auto _ : state) {                                   \
			X("testdata/cover.jpg", "testdata/cover_small.jpg"); \
		}                                                        \
	}                                                            \
	BENCHMARK(BM_##X);
