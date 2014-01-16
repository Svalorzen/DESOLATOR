#ifndef DESOLATOR_RANDOM_HEADER_FILE
#define DESOLATOR_RANDOM_HEADER_FILE

namespace Desolator {
    class RandomInt {
        public:
            static int get(int min, int max);
        private:
            RandomInt();
            static RandomInt * instance_;
    };

}

#endif
