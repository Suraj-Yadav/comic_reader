#include <functional>
#include <list>
#include <unordered_map>

template <typename K, typename W> class LRU {
	std::list<std::pair<K, W>> hits;
	std::unordered_map<K, typename std::list<std::pair<K, W>>::iterator>
		positions;
	W currentWeight;
	W maxWeight;
	unsigned int minCount;
	std::list<std::function<void(K)>> evictionHooks;

	void trim() {
		while (hits.size() > minCount && currentWeight > maxWeight) {
			auto del = hits.front();
			hits.pop_front();
			positions.erase(del.first);
			for (auto& hook : evictionHooks) {
				if (hook) { hook(del.first); }
			}
			currentWeight -= del.second;
		}
	}

   public:
	LRU(W maxW, unsigned int minC)
		: currentWeight(0), maxWeight(maxW), minCount(minC) {}

	void addEvictionHook(std::function<void(K)> func) {
		evictionHooks.push_back(func);
	}

	void hit(K key, W weight = 1) {
		auto it = positions.find(key);
		if (it != positions.end()) {
			currentWeight -= it->second->second;
			hits.erase(it->second);
		}
		currentWeight += weight;
		hits.emplace_back(key, weight);
		positions[key] = std::prev(std::end(hits));
		trim();
	}
};
