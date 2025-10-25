#pragma once

#include <cstdint>
#include <vector>

namespace wrr {

    /// selector entry
    struct SelectorEntry {
        int id{};
        uint32_t weight{}; //!< assigned weight
        int64_t current{}; //!< internal counter
    };

    /// weighted round-robin selector
    class Selector {
    public:
        /// create a weighted round-robin selector
        /// @param weights normalized weights for each entry
        Selector(std::vector<uint32_t> weights);

        /// get next entry
        /// @return selected entry id
        int next();
    private:
        std::vector<SelectorEntry> entries;
        uint32_t totalWeight{};
    };

}
