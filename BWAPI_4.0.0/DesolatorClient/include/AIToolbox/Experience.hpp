#ifndef AI_TOOLBOX_EXPERIENCE_HEADER_FILE
#define AI_TOOLBOX_EXPERIENCE_HEADER_FILE

#include <iosfwd>

#include <AIToolbox/Types.hpp>
#include <AIToolbox/Utils.hpp>

namespace AIToolbox {
    /**
     * @brief This class keeps track of registered events and rewards.
     *
     * This class is a simple logger of events. It keeps track of both
     * the number of times a particular transition has happened, and the
     * total reward gained in any particular transition. However, it
     * does not record each event separately (i.e. you can't extract
     * the results of a particular transition in the past).
     */
    class Experience {
        public:
            using VisitTable = boost::multi_array<unsigned long,3>;
            using RewardTable = Table3D;

            /**
             * @brief Basic constructor.
             *
             * @param s The number of states of the world.
             * @param a The number of actions available to the agent.
             */
            Experience(size_t s, size_t a);

            /**
             * @brief Compatibility setter.
             *
             * This function takes an arbitrary three dimensional
             * containers and tries to copy its contents into the
             * visits table.
             *
             * The container needs to support data access through
             * operator[]. In addition, the dimensions of the
             * container must match the ones specified during the
             * Experience construction (for three dimensions: S,S,A).
             *
             * This is important, as this function DOES NOT perform
             * any size checks on the external containers.
             *
             * This function is provided so that it is easy to plug
             * this library into existing code-bases.
             *
             * \sa copyTable3D()
             *
             * @tparam V The external visits container type.
             * @param v The external visits container.
             */
            template <typename V>
            void setVisits(V v);

            /**
             * @brief Compatibility setter.
             *
             * This function takes an arbitrary three dimensional
             * containers and tries to copy its contents into the
             * rewards table.
             *
             * The container needs to support data access through
             * operator[]. In addition, the dimensions of the
             * container must match the ones specified during the
             * Experience construction (for three dimensions: S,S,A).
             *
             * This is important, as this function DOES NOT perform
             * any size checks on the external containers.
             *
             * This function is provided so that it is easy to plug
             * this library into existing code-bases.
             *
             * \sa copyTable3D()
             *
             * @tparam R The external rewards container type.
             * @param r The external rewards container.
             */
            template <typename R>
            void setRewards(R r);

            /**
             * @brief Adds a new event to the recordings.
             *
             * @param s     Old state.
             * @param s1    New state.
             * @param a     Performed action.
             * @param rew   Obtained reward.
             */
            void record(size_t s, size_t s1, size_t a, double rew);

            /**
             * @brief This function resets all experienced rewards and transitions.
             */
            void reset();

            /**
             * @brief This function returns the visits table for inspection.
             *
             * @return The visits table.
             */
            const VisitTable & getVisits() const;

            /**
             * @brief This function returns the rewards table for inspection.
             *
             * @return The rewards table.
             */
            const RewardTable & getRewards() const;

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
        private:
            size_t S, A;

            VisitTable visits_;
            RewardTable rewards_;

            friend std::istream& operator>>(std::istream &is, Experience &);
    };

    std::ostream& operator<<(std::ostream &os, const Experience &);
    std::istream& operator>>(std::istream &is, Experience &);

    template <typename V>
    void Experience::setVisits(V v) {
        copyTable3D(v, visits_, S, S, A);
    }

    template <typename R>
    void Experience::setRewards(R r) {
        copyTable3D(r, rewards_, S, S, A);
    }
}

#endif
