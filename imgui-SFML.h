#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>

namespace sf
{
    class Event;
    class RenderTarget;
    class RenderWindow;
    class Sprite;
    class Texture;
    class Window;
}

namespace ImGui
{
namespace SFML
{
    void Init(sf::Window& window, sf::RenderTarget& target);
    void Init(sf::RenderWindow& window); // for convenience
    void ProcessEvent(const sf::Event& event);
    void Update(sf::Time dt);
    void Shutdown();

    void SetRenderTarget(sf::RenderTarget& target);
    void SetWindow(sf::Window& window);
}

// custom ImGui widgets for SFML stuff

// Image overloads
    void Image(const sf::Texture& texture,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);
    void Image(const sf::Texture& texture, const sf::Vector2f& size,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);
    void Image(const sf::Texture& texture, const sf::FloatRect& textureRect,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);
    void Image(const sf::Texture& texture, const sf::Vector2f& size, const sf::FloatRect& textureRect,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);

    void Image(const sf::Sprite& sprite,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);
    void Image(const sf::Sprite& sprite, const sf::Vector2f& size,
        const sf::Color& tintColor = sf::Color::White,
        const sf::Color& borderColor = sf::Color::Transparent);

// ImageButton overloads
    bool ImageButton(const sf::Texture& texture, const int framePadding = -1,
                     const sf::Color& bgColor = sf::Color::Transparent,
                     const sf::Color& tintColor = sf::Color::White);
    bool ImageButton(const sf::Texture& texture, const sf::Vector2f& size, const int framePadding = -1,
                     const sf::Color& bgColor = sf::Color::Transparent, const sf::Color& tintColor = sf::Color::White);

    bool ImageButton(const sf::Sprite& sprite, const int framePadding = -1,
                     const sf::Color& bgColor = sf::Color::Transparent,
                     const sf::Color& tintColor = sf::Color::White);
    bool ImageButton(const sf::Sprite& sprite, const sf::Vector2f& size, const int framePadding = -1,
                     const sf::Color& bgColor = sf::Color::Transparent,
                     const sf::Color& tintColor = sf::Color::White);

// Draw_list overloads. All positions are in relative coordinates (relative to top-left of the current window)
    void DrawLine(const sf::Vector2f& a, const sf::Vector2f& b, const sf::Color& col, float thickness = 1.0f);
    void DrawRect(const sf::FloatRect& rect, const sf::Color& color, float rounding = 0.0f, int rounding_corners = 0x0F, float thickness = 1.0f);
    void DrawRectFilled(const sf::FloatRect& rect, const sf::Color& color, float rounding = 0.0f, int rounding_corners = 0x0F);
}
