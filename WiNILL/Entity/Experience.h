#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

// Runtime XP is stored in thousandths. Conversion happens only when XP is
// produced or shown; drops and level progress move exact integer units.
using XpAmount = std::int64_t;
inline constexpr XpAmount XP_SCALE = 1000;

inline XpAmount XpFromWhole(std::int64_t value) {
    if (value <= 0) return 0;
    if (value > std::numeric_limits<XpAmount>::max() / XP_SCALE)
        return std::numeric_limits<XpAmount>::max();
    return value * XP_SCALE;
}

inline XpAmount XpAdd(XpAmount current, XpAmount earned) {
    if (earned <= 0) return current;
    if (current > std::numeric_limits<XpAmount>::max() - earned)
        return std::numeric_limits<XpAmount>::max();
    return current + earned;
}

inline XpAmount XpFromReward(double value) {
    if (!std::isfinite(value) || value <= 0.0) return 0;
    const double maximum = static_cast<double>(std::numeric_limits<XpAmount>::max() / XP_SCALE);
    if (value >= maximum) return std::numeric_limits<XpAmount>::max();
    return static_cast<XpAmount>(std::llround(value * XP_SCALE));
}

inline double XpToDouble(XpAmount value) {
    return static_cast<double>(value) / XP_SCALE;
}

inline std::string FormatXp(XpAmount value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << XpToDouble(value);
    return out.str();
}

// Keep sub-milli XP between frames; return only whole milli-XP to grant.
inline XpAmount AccumulateTimedXp(double& remainingMilli, double xpPerSecond,
                                  double multiplier, double seconds) {
    if (!std::isfinite(xpPerSecond) || !std::isfinite(multiplier) ||
        !std::isfinite(seconds) || xpPerSecond <= 0.0 ||
        multiplier <= 0.0 || seconds <= 0.0) return 0;
    remainingMilli += xpPerSecond * multiplier * seconds * XP_SCALE;
    if (!std::isfinite(remainingMilli) ||
        remainingMilli >= static_cast<double>(std::numeric_limits<XpAmount>::max())) {
        remainingMilli = 0.0;
        return std::numeric_limits<XpAmount>::max();
    }
    // Float stat multipliers (for example 1.03f) can land just below a
    // mathematical milli-XP boundary after many frames.
    const XpAmount grant = static_cast<XpAmount>(std::floor(remainingMilli + 1e-3));
    remainingMilli -= grant;
    return grant;
}
