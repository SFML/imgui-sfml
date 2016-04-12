#include <SFML/System/Vector2.hpp>

namespace sf
{
    class Event;
    class RenderTarget;
    class RenderWindow;
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
    void Image(const sf::Texture& texture, const sf::Vector2u& size);
}