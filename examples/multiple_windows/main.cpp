#include "imgui.h" // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h

#include "imgui-SFML.h" // for ImGui::SFML::* functions and SFML-specific overloads

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    sf::RenderWindow childWindow(sf::VideoMode(640, 480), "ImGui-SFML Child window");
    childWindow.setFramerateLimit(60);
    if (!ImGui::SFML::Init(childWindow)) return -1;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        // Main window event processing
        sf::Event event{};
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed) {
                if (childWindow.isOpen()) {
                    childWindow.close();
                }
                window.close();
                ImGui::SFML::Shutdown(); // will shutdown all windows
                return 0; // return here so that we don't call Update/Render
            }
        }

        // Child window event processing
        if (childWindow.isOpen()) {
            while (childWindow.pollEvent(event)) {
                ImGui::SFML::ProcessEvent(childWindow, event);
                if (event.type == sf::Event::Closed) {
                    childWindow.close();
                    ImGui::SFML::Shutdown(childWindow);
                }
            }
        }

        // Update
        const sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);
        if (childWindow.isOpen()) {
            ImGui::SFML::Update(childWindow, dt);
        }

        // Add ImGui widgets in the first window
        ImGui::SFML::SetCurrentWindow(window);
        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();
        ImGui::ShowDemoWindow();
        // Add ImGui widgets in the child window
        if (childWindow.isOpen()) {
            ImGui::SFML::SetCurrentWindow(childWindow);
            ImGui::Begin("Works in a second window!");
            ImGui::Button("Example button");
            ImGui::End();
        }

        // Main window drawing
        sf::CircleShape shape(100.f);
        shape.setFillColor(sf::Color::Green);

        window.clear();
        window.draw(shape);
        ImGui::SFML::Render(window);
        window.display();

        // Child window drawing
        if (childWindow.isOpen()) {
            sf::CircleShape shape2(50.f);
            shape2.setFillColor(sf::Color::Red);

            childWindow.clear();
            childWindow.draw(shape2);
            ImGui::SFML::Render(childWindow);
            childWindow.display();
        }
    }
}
