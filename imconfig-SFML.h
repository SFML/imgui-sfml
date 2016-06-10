// Add this to your imconfig.h

#include <SFML/System/Vector2.hpp>

#define IM_VEC2_CLASS_EXTRA												\
	template <typename T>												\
	ImVec2(const sf::Vector2<T>& v) {									\
		x = static_cast<float>(v.x);									\
		y = static_cast<float>(v.y);									\
	}																	\
	template <typename T>												\
	operator sf::Vector2<T>() const {									\
		return sf::Vector2<T>{sf::Vector2f{x, y}};						\
	}
