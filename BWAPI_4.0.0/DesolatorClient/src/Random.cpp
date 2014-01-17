#include <Desolator/Random.hpp>

#include <chrono>

namespace Desolator {
    int RandomInt::get(int min, int max) {
        static RandomInt instance;

        std::uniform_int_distribution<int> dist(min,max);
        return dist(instance.generator_);
    }

    RandomInt::RandomInt() : generator_(std::chrono::system_clock::now().time_since_epoch().count()) {}

}
