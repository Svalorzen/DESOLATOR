#include <Desolator/MDPState.hpp>

#include <numeric>

namespace Desolator {

    const size_t MDPState::FeatureValues[] = {
        3,      // ENEMY_PROXIMITY      (0: free,       1: enemy range, 2: targeted)
        2,      // FRIEND_PROXIMITY     (0: uncovered,  1: covered)
        2,      // WEAPON_COOLDOWN      (0: false,      1: true)
        2,      // CAN_TARGET           (0: false,      1: true)
        4       // HEALTH               (0: 25%,        1: 50%,         2: 75%,         3: 100%)
    };

    MDPState::MDPState() {
        std::fill(std::begin(features_), std::end(features_), 0);
    }

    size_t MDPState::getNumberOfStates() {
        return std::accumulate(std::begin(FeatureValues), std::end(FeatureValues), 1, std::multiplies<size_t>());
    }

    MDPState::MDPState(size_t x) {
        x = std::min(x, getNumberOfStates()-1);

        for ( int i = 0; i < FEATURE_COUNT; i++ ) {
            features_[i] = x % ( FeatureValues[i] );
            x /= FeatureValues[i];
        }

    }

    MDPState::operator size_t() const {
        size_t x = 0;
        size_t base = 1;

        //x = feature(0) + feature(1) * feature_0_max + feature(2) * feature_0_max * feature_1_max;
        for ( int i = 0; i < FEATURE_COUNT; i++ ) {
            x += features_[i] * base;
            base *= FeatureValues[i];
        }

        return x;
    }
}
