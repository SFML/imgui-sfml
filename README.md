ImGui + SFML
=======

Library which allows you to use [ImGui](https://github.com/ocornut/imgui) with [SFML](https://github.com/SFML/SFML)
Based on [this repository](https://github.com/Mischa-Alff/imgui-backends) with some improvements / changes.

How-to
----

Setting up:

- Download [ImGui](https://github.com/ocornut/imgui) 
- Add ImGui folder to your include directories
- Add `imgui.cpp` and `imgui_draw.cpp` to your build/project
- Copy the contents of `imconfig-SFML.h` to your `imconfig.h` file. (to be able to cast ImVec2 to sf::Vector2f and vice versa)
- Add a folder which contains `imgui-SFML.h` to your include directories
- Add `imgui-SFML.cpp` to your build/project

In your code:

- Call `ImGui::SFML::Init` and pass your `sf::Window` + `sf::RenderTarget` or `sf::RenderWindow` there
- For each iteration of a game loop:
    - Poll and process events:
    
        ```c++
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            ...
        }
        ```
    
    - Call `ImGui::SFML::Update()`
    - Call ImGui functions (`ImGui::Begin()`, `ImGui::Button()`, etc.)
    - Call `ImGui::EndFrame` if you update more than once before rendering (you'll need to include `imgui_internal.h` for that)
    - Call `ImGui::Render()`

- Call `ImGui::SFML::Shutdown()` at the end of your program

Example code
----

```c++
#include "imgui.h"
#include "imgui-sfml.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();
        ImGui::Render();
        window.display();
    }

    ImGui::SFML::Shutdown();
}
```

SFML related ImGui overloads / new widgets
---

There are some useful overloads implemented for SFML objects (see header for overloads):
```c++
ImGui::Image(const sf::Sprite& sprite);
ImGui::Image(const sf::Texture& texture);
ImGui::ImageButton(const sf::Sprite& sprite);
ImGui::ImageButton(const sf::Texture& texture);
```

License
---

This library is licensed under the MIT License, see LICENSE for more information.
