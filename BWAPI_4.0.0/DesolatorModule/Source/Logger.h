#ifndef LOGGER_HEADER_FILE
#define LOGGER_HEADER_FILE

#include <string>
#include <fstream>

class Logger {
    public:
        Logger(const std::string & filename, std::ios_base::openmode mode = std::ios_base::out);
        ~Logger();

        // No variadic templates in VS2010...
        template <class A>
        void operator()(A a);

        template <class A, class B>
        void operator()(A a, B b);

        template <class A, class B, class C>
        void operator()(A a, B b, C c);

        template <class A, class B, class C, class D>
        void operator()(A a, B b, C c, D d);

        template <class A, class B, class C, class D, class E>
        void operator()(A a, B b, C c, D d, E e);

        std::ofstream stream_;
};

template <class A>
void Logger::operator()(A a) {
    stream_ << a << std::endl;
}

template <class A, class B>
void Logger::operator()(A a, B b) {
    stream_ << a << b << std::endl;
}

template <class A, class B, class C>
void Logger::operator()(A a, B b, C c) {
    stream_ << a << b << c << std::endl;
}

template <class A, class B, class C, class D>
void Logger::operator()(A a, B b, C c, D d) {
    stream_ << a << b << c << d << std::endl;
}

template <class A, class B, class C, class D, class E>
void Logger::operator()(A a, B b, C c, D d, E e) {
    stream_ << a << b << c << d << e << std::endl;
}

#endif