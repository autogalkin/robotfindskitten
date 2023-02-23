#pragma once
#include <numbers>
#include <utility>
#include <cmath>


// from the https://easings.net/ and https://github.com/nicolausYes/easing-functions
namespace easing
{
    using std::numbers::pi;

    inline double easeinrange(const double value, const std::pair<double, double> range, const double duration, double (*ease_func)(double))
    {
        static constexpr double limit[2]{0., 1.};
        double t = limit[0] * (1 - value) + limit[1] * value;
        t /= duration;
        const double e_res = ease_func(t);
        return range.second * e_res + range.first * (1 - e_res);
    }
    
    inline double easeInSine(const double t){
        return std::sin(pi / 2 * t);
    }

    inline double easeOutSine(double t){
        std::pair<double, double> i;
        return 1 + std::sin(pi / 2 * (--t));
    }

    inline double easeInOutSine(const double t){
        return 0.5 * (1 + std::sin(pi * (t - 0.5)));
    }

    constexpr double easeInQuad(const double t){
        return t * t;
    }

    constexpr double easeOutQuad(const double t){
        return t * (2 - t);
    }

    constexpr double easeInOutQuad(const double t){
        return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
    }

    constexpr double easeInCubic(double t){
        return t * t * t;
    }

    constexpr double easeOutCubic(double t){
        return 1 + (--t) * t * t;
    }

    constexpr double easeInOutCubic(double t){
        return t < 0.5 ? 4 * t * t * t : 1 + (--t) * (2 * (--t)) * (2 * t);
    }

    constexpr double easeInQuart(const double t){
        return t * t * t * t;
    }

    constexpr double easeOutQuart(double t){
        t = (--t) * t;
        return 1 - t * t;
    }

    constexpr double easeInOutQuart(double t){
        if (t < 0.5){
            t *= t;
            return 8 * t * t;
        }
        t = (--t) * t;
        return 1 - 8 * t * t;
    }

    constexpr double easeInQuint(double t){
        const double t2 = t * t;
        return t * t2 * t2;
    }

    constexpr double easeOutQuint(double t){
        const double t2 = (--t) * t;
        return 1 + t * t2 * t2;
    }

    constexpr double easeInOutQuint(double t){
        double t2;
        if (t < 0.5){
            t2 = t * t;
            return 16 * t * t2 * t2;
        }
        t2 = (--t) * t;
        return 1 + 16 * t * t2 * t2;
    }

    inline double easeInExpo(double t){
        return (std::pow(2, 8 * t) - 1) / 255;
    }

    inline double easeOutExpo(double t){
        return 1 - std::pow(2, -8 * t);
    }

    inline double easeInOutExpo(double t){
        if (t < 0.5){
            return (std::pow(2, 16 * t) - 1) / 510;
        }
        return 1 - 0.5 * std::pow(2, -16 * (t - 0.5));
    }

    inline double easeInCirc(double t){
        return 1 - std::sqrt(1 - t);
    }

    inline double easeOutCirc(double t){
        return std::sqrt(t);
    }

    inline double easeInOutCirc(double t){
        if (t < 0.5){
            return (1 - std::sqrt(1 - 2 * t)) * 0.5;
        }
        return (1 + std::sqrt(2 * t - 1)) * 0.5;
    }

    constexpr double easeInBack(double t){
        return t * t * (2.70158 * t - 1.70158);
    }

    constexpr double easeOutBack(double t){
        return 1 + (--t) * t * (2.70158 * t + 1.70158);
    }

    constexpr double easeInOutBack(double t){
        if (t < 0.5){
            return t * t * (7 * t - 2.5) * 2;
        }
        return 1 + (--t) * t * 2 * (7 * t + 2.5);
    }

    inline double easeInElastic(const double t){
        const double t2 = t * t;
        return t2 * t2 * std::sin(t * pi * 4.5);
    }

    inline double easeOutElastic(const double t){
        const double t2 = (t - 1) * (t - 1);
        return 1 - t2 * t2 * std::cos(t * pi * 4.5);
    }

    inline double easeInOutElastic(const double t){
        double t2;
        if (t < 0.45){
            t2 = t * t;
            return 8 * t2 * t2 * std::sin(t * pi * 9);
        }
        if (t < 0.55)
        {
            return 0.5 + 0.75 * std::sin(t * pi * 4);
        }
        t2 = (t - 1) * (t - 1);
        return 1 - 8 * t2 * t2 * std::sin(t * pi * 9);
    }

    inline double easeInBounce(const double t){
        return std::pow(2, 6 * (t - 1)) * abs(sin(t * pi * 3.5));
    }

    inline double easeOutBounce(const double t){
        return 1 - std::pow(2, -6 * t) * abs(cos(t * pi * 3.5));
    }

    inline double easeInOutBounce(const double t){
        if (t < 0.5){
            return 8 * std::pow(2, 8 * (t - 1)) * abs(sin(t * pi * 7));
        }
        return 1 - 8 * std::pow(2, -8 * t) * abs(sin(t * pi * 7));
    }
}
