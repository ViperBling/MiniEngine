#pragma once

#include <cstddef>
#include <functional>

// template<> struct std::hash<MiniEngine::Vertex> {
//     size_t operator()(MiniEngine::Vertex const& vertex) const {
//         return ((hash<glm::vec3>()(vertex.Position) ^
//                 (hash<glm::vec2>()(vertex.Texcoord) << 1)) >> 1);
//     }
// };

template<typename T>
inline void HashCombine(std::size_t& seed, const T& v) {
    seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T, typename... Ts>
inline void HashCombine(std::size_t& seed, const T& v, Ts... rest)
{
    HashCombine(seed, v);
    if constexpr (sizeof...(Ts) > 1)
        HashCombine(seed, rest...);
}