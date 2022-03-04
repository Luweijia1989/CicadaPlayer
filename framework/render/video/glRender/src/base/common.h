#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_PI_2
#define M_PI_2 (1.57079632679489661923)
#endif

#ifndef M_PI_4
#define M_PI_4 (0.78539816339744830962)
#endif

namespace YPP {
    enum AspectRatioMode {
        IgnoreAspectRatio = 0,
        KeepAspectRatio = 1,
        KeepAspectRatioByExpanding = 2,
    };

    enum class Initialization { Uninitialized };
    static constexpr Initialization Uninitialized = Initialization::Uninitialized;
}// namespace YPP

static inline bool qIsNull(double d)
{
    union U {
        double d;
        unsigned __int64 u;
    };
    U val;
    val.d = d;
    return (val.u & (unsigned long long) 0x7fffffffffffffffLL) == 0;
}

static inline bool qFuzzyCompare(double p1, double p2)
{
    return (std::abs(p1 - p2) * 1000000000000. <= std::min<double>(std::abs(p1), std::abs(p2)));
}
static inline bool qFuzzyCompare(float p1, float p2)
{
    return (std::abs(p1 - p2) * 100000.f <= std::min<float>(std::abs(p1), std::abs(p2)));
}
static inline bool qFuzzyIsNull(double d)
{
    return std::abs(d) <= 0.000000000001;
}
static inline bool qFuzzyIsNull(float f)
{
    return std::abs(f) <= 0.00001f;
}

template<typename T>
inline void qSwap(T &value1, T &value2)
{
    using std::swap;
    swap(value1, value2);
}

inline float qDegreesToRadians(float degrees)
{
    return degrees * float(M_PI / 180);
}
inline double qDegreesToRadians(double degrees)
{
    return degrees * (M_PI / 180);
}
inline float qRadiansToDegrees(float radians)
{
    return radians * float(180 / M_PI);
}
inline double qRadiansToDegrees(double radians)
{
    return radians * (180 / M_PI);
}

inline void split(const std::string &s, std::vector<std::string> &tokens, const std::string &delimiters = "")
{
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

inline void replaceAll(std::string &s, const std::string &search, const std::string &replace)
{
    for (size_t pos = 0;; pos += replace.length()) {
        // Locate the substring to replace
        pos = s.find(search, pos);
        if (pos == std::string::npos) break;
        // Replace by erasing and inserting
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

template<typename... Args>
inline std::string string_format(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;// Extra space for '\0'
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);// We don't want the '\0' inside
}