// Add this to your imconfig.h

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

#define IM_VEC2_CLASS_EXTRA                                             \
    template <typename T>                                               \
    ImVec2(const sf::Vector2<T>& v) {                                   \
        x = static_cast<float>(v.x);                                    \
        y = static_cast<float>(v.y);                                    \
    }                                                                   \
                                                                        \
    template <typename T>                                               \
    operator sf::Vector2<T>() const {                                   \
        return sf::Vector2<T>(x, y);                                    \
    }

#define IM_VEC4_CLASS_EXTRA                                             \
    ImVec4(const sf::Color& c)                                          \
        : ImVec4(static_cast<float>(c.r), static_cast<float>(c.g),      \
                 static_cast<float>(c.b), static_cast<float>(c.a))      \
    { }                                                                 \
                                                                        \
    operator sf::Color() const {                                        \
        return sf::Color(static_cast<sf::Uint8>(x),                     \
            static_cast<sf::Uint8>(y), static_cast<sf::Uint8>(z),       \
            static_cast<sf::Uint8>(w));                                 \
    }
