#ifndef REPL_HASHHELPER_H
#define REPL_HASHHELPER_H

#include <cstddef>
#include <functional>
#include <utility>

class hash_helper {
public:

    static void hash_combine(std::size_t &seed) {}

    template<typename T, typename ...Args>
    static void hash_combine(std::size_t &seed, const T &first, const Args &...args) {
        seed ^= std::hash<T>()(first) + 0x9e3779b9 + (seed << 6u) + (seed >> 2u);
        hash_combine(seed, args...);
    }

    template<typename ...Args>
    static std::size_t hash_code(const Args &...args) {
        std::size_t seed = 19;
        hash_combine(seed, args...);
        return seed;
    }

};

namespace std {

    template<typename T1, typename T2>
    struct hash<const pair <T1, T2>> {
        size_t operator()(const pair <T1, T2> &p) const {
            return hash_helper::hash_code(p.first, p.second);
        }
    };

}

#endif //REPL_HASHHELPER_H
