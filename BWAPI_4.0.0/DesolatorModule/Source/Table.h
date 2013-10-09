#ifndef TABLE_HEADER_FILE
#define TABLE_HEADER_FILE

#include <string>
#include <array>
#include <tuple>

#include "UnitState.h"

class Table {
    public:
        // Since we have to actions, we record how many times we did one or the other in a particular MDPState-MDPState transition.
        // In addition we also record the total rewards obtained in doing so. A1, A2, A1rew, A2rew.
        typedef std::tuple<long unsigned, long unsigned, double, double> EntryType;
        typedef std::array<EntryType, MDPState::statesNumber> TransitionType;
        typedef std::array<TransitionType, MDPState::statesNumber> TableType;

        Table();

        bool isValid();

        bool load(const std::string & filename);
        bool save(std::string filename = "");
        void update(const MDPState & before, const Action & action, const MDPState & after, double reward);
    private:
        std::string lastFilename_;
        bool isValid_;

        TableType table_;
};

#endif