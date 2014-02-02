#ifndef AI_TOOLBOX_MDP_MODEL_HEADER_FILE
#define AI_TOOLBOX_MDP_MODEL_HEADER_FILE

#include <utility>
#include <random>

#include <AIToolbox/Types.hpp>

namespace AIToolbox {
    namespace MDP {
        /**
         * @brief This class represents a Markov Decision Process.
         */
        class Model {
            public:
                using TransitionTable   = Table3D;
                using RewardTable       = Table3D;

                /**
                 * @brief This function checks whether the supplied table is a correct transition table.
                 *
                 * This function verifies basic probability conditions on the
                 * supplied container. The sum of all transitions from a
                 * state action pair to all states must be 1.
                 *
                 * The container needs to support data access through
                 * operator[]. In addition, the dimensions of the
                 * container must match the ones provided as arguments
                 * (for three dimensions: s,s,a).
                 *
                 * This is important, as this function DOES NOT perform
                 * any size checks on the external containers.
                 *
                 * Internal values of the container will be converted to double,
                 * so that convertion must be possible.
                 *
                 * This function is provided so that it is easy to plug
                 * this library into existing code-bases.
                 *
                 * @tparam T The external transition container type.
                 * @param s The number of states of the world.
                 * @param a The number of actions available to the agent.
                 * @param t The external transitions container.
                 *
                 * @return True if the container statisfies probability constraints,
                 *         and false otherwise.
                 */
                template <typename T>
                static bool mdpCheck(size_t s, size_t a, T t);

                /**
                 * @brief Basic constructor.
                 *
                 * This constructor initializes the Model so that all
                 * transitions happen with probability 0 but for transitions
                 * that bring back to the same state, no matter the action.
                 *
                 * All rewards are set to 0.
                 *
                 * @param s The number of states of the world.
                 * @param a The number of actions available to the agent.
                 */
                Model(size_t s, size_t a);

                /**
                 * @brief Basic constructor.
                 *
                 * This constructor takes two arbitrary three dimensional
                 * containers and tries to copy their contents into the
                 * transitions and rewards tables respectively.
                 *
                 * The containers need to support data access through
                 * operator[]. In addition, the dimensions of the
                 * containers must match the ones provided as arguments
                 * (for three dimensions: s,s,a).
                 *
                 * This is important, as this constructor DOES NOT perform
                 * any size checks on the external containers.
                 *
                 * Internal values of the containers will be converted to double,
                 * so these convertions must be possible.
                 *
                 * In addition, the transition container must respect
                 * the constraint described in the mdpCheck() function.
                 *
                 * \sa copyTable3D()
                 *
                 * @tparam T The external transition container type.
                 * @tparam R The external rewards container type.
                 * @param s The number of states of the world.
                 * @param a The number of actions available to the agent.
                 * @param t The external transitions container.
                 * @param r The external rewards container.
                 */
                template <typename T, typename R>
                Model(size_t s, size_t a, T t, R r);

                /**
                 * @brief This function replaces the Model transition function with the one provided.
                 *
                 * This function will throw a std::invalid_argument if the table provided
                 * does not respect the constraints specified in the mdpCheck() function.
                 *
                 * The container needs to support data access through
                 * operator[]. In addition, the dimensions of the
                 * container must match the ones provided as arguments
                 * (for three dimensions: s,s,a).
                 *
                 * This is important, as this constructor DOES NOT perform
                 * any size checks on the external container.
                 *
                 * Internal values of the container will be converted to double,
                 * so that convertion must be possible.
                 *
                 * @tparam T The external transition container type.
                 * @param t The external transitions container.
                 */
                template <typename T>
                void setTransitionFunction(T t);

                /**
                 * @brief This function replaces the Model reward function with the one provided.
                 *
                 * The container needs to support data access through
                 * operator[]. In addition, the dimensions of the
                 * containers must match the ones provided as arguments
                 * (for three dimensions: s,s,a).
                 *
                 * This is important, as this constructor DOES NOT perform
                 * any size checks on the external containers.
                 *
                 * Internal values of the container will be converted to double,
                 * so that convertion must be possible.
                 *
                 * @tparam T The external transition container type.
                 * @param t The external transitions container.
                 */
                template <typename R>
                void setRewardFunction(R r);

                /**
                 * @brief This function samples the MDP for the specified state action pair.
                 *
                 * This function samples the model for simulate experience. The transition
                 * and reward functions are used to produce, from the state action pair
                 * inserted as arguments, a possible new state with respective reward.
                 * The new state is picked from all possible states that the MDP allows
                 * transitioning to, each with probability equal to the same probability
                 * of the transition in the model. After a new state is picked, the reward
                 * is the corresponding reward contained in the reward function.
                 *
                 * @param s The state that needs to be sampled.
                 * @param a The action that needs to be sampled.
                 *
                 * @return A tuple containing a new state and a reward.
                 */
                std::pair<size_t, double> sample(size_t s, size_t a) const;

                /**
                 * @brief This function returns the number of states of the world.
                 *
                 * @return The total number of states.
                 */
                size_t getS() const;

                /**
                 * @brief This function returns the number of available actions to the agent.
                 *
                 * @return The total number of actions.
                 */
                size_t getA() const;

                /**
                 * @brief This function returns the stored transition probability for the specified transition.
                 *
                 * @param s The initial state of the transition.
                 * @param s1 The final state of the transition.
                 * @param a The action performed in the transition.
                 *
                 * @return The probability of the specified transition.
                 */
                double getTransitionProbability(size_t s, size_t s1, size_t a) const;

                /**
                 * @brief This function returns the stored expected reward for the specified transition.
                 *
                 * @param s The initial state of the transition.
                 * @param s1 The final state of the transition.
                 * @param a The action performed in the transition.
                 *
                 * @return The expected reward of the specified transition.
                 */
                double getExpectedReward(size_t s, size_t s1, size_t a) const;

                /**
                 * @brief This function returns the transition table for inspection.
                 *
                 * @return The rewards table.
                 */
                const TransitionTable & getTransitionFunction() const;

                /**
                 * @brief This function returns the rewards table for inspection.
                 *
                 * @return The rewards table.
                 */
                const RewardTable &     getRewardFunction()     const;

            private:
                size_t S, A;

                TransitionTable transitions_;
                RewardTable rewards_;

                mutable std::default_random_engine rand_;
                mutable std::uniform_real_distribution<double> sampleDistribution_;
        };

        template <typename T>
        bool Model::mdpCheck(size_t s_, size_t a_, T t) {
            for ( size_t s = 0; s < s_; ++s ) {
                for ( size_t a = 0; a < a_; ++a ) {
                    double p = 0.0;
                    for ( size_t s1 = 0; s1 < s; ++s1 ) {
                        p += t[s][s1][a];
                    }
                    if ( p != 1.0 ) return false;
                }
            }
            return true;
        }

        template <typename T, typename R>
        Model::Model(size_t s, size_t a, T t, R r) : S(s), A(a), transitions_(boost::extents[S][S][A]), rewards_(boost::extents[S][S][A]) {
            setTransitionFunction(t);
            setRewardFunction(r);
        }

        template <typename T>
        void Model::setTransitionFunction( T t ) {
            if ( ! mdpCheck(S, A, t) ) throw std::invalid_argument("Input transition table does not contain valid probabilities.");

            copyTable3D(t, transitions_, S, S, A);
        }

        template <typename R>
        void Model::setRewardFunction( R r ) {
            copyTable3D(r, rewards_, S, S, A);
        }

    } // MDP
} // AIToolbox

#endif
