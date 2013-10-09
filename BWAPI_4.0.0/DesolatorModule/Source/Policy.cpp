#include "Policy.h"

#include <fstream>

Policy::Policy() : isValid_(false) {}

bool Policy::isValid() {
    return isValid_;
}

bool Policy::load(const std::string & filename) {
    std::ifstream file(filename.c_str(), std::ifstream::in);

    // This number is due to the way we output the policy file, as
    //
    // StateNumber ActionToTake
    //
    // So we don't really care abou the StateNumber since they are ordered already
    int useless;
    for ( size_t i = 0; i < std::tuple_size<PolicyType>::value; i++ )
        if ( !(file >> useless >> policy_[i] ) ) {
            isValid_ = false;
            return false;
        }
    // Should we verify the data in some way?
    file.close();
    isValid_ = true;
    return true;
}

int Policy::operator[](int state) {
    return policy_[state];
}