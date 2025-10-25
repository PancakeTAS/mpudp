#include "wrr.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace wrr;

Selector::Selector(std::vector<uint32_t> weights) {
    for (size_t i = 0; i < weights.size(); ++i) {
        this->totalWeight += weights[i];
        this->entries.push_back(SelectorEntry {
            .id = static_cast<int>(i),
            .weight = weights[i],
            .current = 0
        });
    }
}

int Selector::next() {
    SelectorEntry* best{nullptr};
    for (auto& e : this->entries) {
        e.current += e.weight;
        if (!best || e.current > best->current)
            best = &e;
    }

    if (!best) return 0; // should not happen

    best->current -= this->totalWeight;
    return best->id;
}
