#ifndef TEST_OS_H__
#define TEST_OS_H__
#include "tinytast.hpp"

template <typename keyT, typename valueT>
std::ostream& operator<<(std::ostream& os, const std::map<keyT, valueT>& m)
{
    os << '{';
    size_t i = 0;
    for (auto& item : m)
    {
        if (i++ > 0)
        {
            os << ", ";
        }
        os << item.first << ':' << item.second;
    }
    os << '}';
    return os;
}

template <typename valueT>
std::ostream& operator<<(std::ostream& os, const std::vector<valueT>& v)
{
    os << '[';
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i > 0)
        {
            os << ", ";
        }
        os << v[i];
    }
    os << ']';
    return os;
}

#endif /* end of include guard: TEST_OS_H__ */
