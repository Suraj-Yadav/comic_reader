#include "lru.hpp"

#include <benchmark/benchmark.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>

TEST(lru, simple) {
	LRU<int, int> lru(5, 5);
	std::vector<int> evicted;
	lru.addEvictionHook([&evicted](int k) { evicted.emplace_back(k); });

	lru.hit(1);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(2);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(3);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(4);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(5);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(1);
	EXPECT_THAT(evicted, ::testing::SizeIs(0));
	lru.hit(6);
	EXPECT_THAT(evicted, ::testing::ElementsAre(2));
}

void BM_LRU(benchmark::State& state) {
	std::random_device rd;
	const auto cap = int(state.range(0) / 4);
	std::uniform_int_distribution<int> dist(0, cap);
	LRU<int, int> lru(cap, (unsigned int)cap);
	for (auto _ : state) { lru.hit(dist(rd)); }
	state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_LRU)->RangeMultiplier(2)->Range(1 << 10, 1 << 18)->Complexity();
