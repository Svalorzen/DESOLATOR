#ifndef DESOLATOR_RANDOM_HEADER_FILE
#define DESOLATOR_RANDOM_HEADER_FILE

#include <random>

namespace Desolator {
    class RandomInt {
        public:
            static int get(int min, int max);
        private:
            RandomInt();

            std::default_random_engine generator_;
    };

}

#endif
