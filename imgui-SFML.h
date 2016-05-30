#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>

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
    void Update();
    void Shutdown();

    void SetRenderTarget(sf::RenderTarget& target);
    void SetWindow(sf::Window& window);
}

// custom ImGui widgets for SFML stuff
    void Image(const sf::Texture& texture);
    void Image(const sf::Texture& texture, const sf::Vector2f& size);

    void Image(const sf::Sprite& sprite);
    void Image(const sf::Sprite& sprite, const sf::Vector2f& size);

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
}
