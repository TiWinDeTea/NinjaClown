#ifndef NINJACLOWN_UTILS_VISITOR_HPP
#define NINJACLOWN_UTILS_VISITOR_HPP

namespace utils {
template <typename... Visitors>
struct visitor : Visitors... {
    explicit visitor(Visitors&&... visitors) : Visitors{std::move(visitors)}... {}
    using Visitors::operator()...;
};
}

#endif //NINJACLOWN_UTILS_VISITOR_HPP
