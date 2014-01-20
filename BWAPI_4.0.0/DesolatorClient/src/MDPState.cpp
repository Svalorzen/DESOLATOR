#include <Desolator/MDPState.hpp>

#include <numeric>

namespace Desolator {

    const size_t MDPState::FeatureValues[] = {
        // TOP BITS
        3,      // ENEMY_PROXIMITY      (0: free,       1: enemy range, 2: targeted)
        2,      // FRIEND_PROXIMITY     (0: uncovered,  1: covered)
        2,      // WEAPON_COOLDOWN      (0: false,      1: true)
        2,      // CAN_TARGET           (0: false,      1: true)
        4       // HEALTH               (0: 25%,        1: 50%,         2: 75%,         3: 100%)
        // BOTTOM BITS
    };

    size_t MDPState::getNumberOfStates() {
        return std::accumulate(std::begin(FeatureValues), std::end(FeatureValues), 1, std::multiplies<size_t>());
    }

    MDPState::MDPState() {
        std::fill(std::begin(features_), std::end(features_), 0);
    }

    MDPState::MDPState(size_t x) {
        setState(std::min(x, getNumberOfStates()-1));
    }

    MDPState::operator size_t() const {
        size_t x = 0;
        size_t base = 1;

        //x = feature(BOTTOM) + feature(BOTTOM+1) * feature_BOTTOM_max + feature(BOTTOM+2) * feature_BOTTOM_max * feature_BOTTOM+1_max + ...;
        for ( int i = FEATURE_COUNT-1; i >= 0; --i ) {
            x += features_[i] * base;
            base *= FeatureValues[i];
        }

        return x;
    }

    void MDPState::setState(size_t s) {
        if ( s >= getNumberOfStates() ) return;

        for ( int i = FEATURE_COUNT-1; i >= 0; --i ) {
            features_[i] = s % ( FeatureValues[i] );
            s /= FeatureValues[i];
        }
    }

    void MDPState::setFeatureValue(Feature f, size_t v) {
        features_[f] = std::min(v, FeatureValues[f]);
    }

    size_t MDPState::getFeatureValue(Feature f) const {
        return features_[f];
    }
}
