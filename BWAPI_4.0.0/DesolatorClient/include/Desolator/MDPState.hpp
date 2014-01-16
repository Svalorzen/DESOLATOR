#ifndef DESOLATOR_MDP_STATE_HEADER_FILE
#define DESOLATOR_MDP_STATE_HEADER_FILE

#include <array>

namespace Desolator {

    class MDPState {
        /* Represents the state within the MDP model */
        public:
            MDPState();
            MDPState(size_t);

            // SWAPPING FEATURES WILL CHANGE THE STATE NUMBER
            // Adding at the end should not
            enum Feature {
                ENEMY_PROXIMITY = 0,
                FRIEND_PROXIMITY,
                WEAPON_COOLDOWN,
                CAN_TARGET,
                HEALTH,
                FEATURE_COUNT   // Leave this last
            };

            static const size_t FeatureValues[];

            static size_t getNumberOfStates();

            void setState(size_t);
            void setFeatureValue(Feature, size_t);

            size_t getFeatureValue(Feature) const;

            // Converts the state into a number from 0 to 95 ( so we can use them as unique indexes for arrays and stuff )
            operator size_t() const;
        private:
            std::array<size_t, FEATURE_COUNT> features_;
    };

}

#endif
