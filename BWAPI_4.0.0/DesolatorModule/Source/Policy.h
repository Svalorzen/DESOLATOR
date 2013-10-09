#ifndef POLICY_HEADER_FILE
#define POLICY_HEADER_FILE

#include <string>
#include <array>
#include <tuple>

#include "MDPState.h"

class Policy {
    public:
        typedef std::array<int, MDPState::statesNumber> PolicyType;

        Policy();

        bool isValid();

        bool load(const std::string & filename);
        int operator[](int state);
    private:
        bool isValid_;

        PolicyType policy_;
};

#endif