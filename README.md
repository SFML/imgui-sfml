ImGui + SFML
=======

Library which allows you to use [ImGui](https://github.com/ocornut/imgui) with [SFML](https://github.com/SFML/SFML)
Based on [this repository](https://github.com/Mischa-Alff/imgui-backends) with some improvements / changes.

How-to
----

- Download [imgui](https://github.com/ocornut/imgui) 
- Add imgui folder to your include directories
- Add `imgui.cpp` and `imgui_render.cpp` to your build/project
- Copy the contents of `imconfig-SFML.h` to your `imconfig.h` file.
- Add folder which contains `imgui-SFML.h` to your include directories
- Add `imgui-SFML.cpp` to your build/project

- Call `ImGui::SFML::Init` and pass your `sf::Window` + `sf::RenderTarget` or `sf::RenderWindow` there
- For each iteration of game loop:
    - Update:
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
    -Render:
        - Call ImGui::Render()

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

I've also added some overloads for convenience, so you can render sf::Texture inside ImGui windows like this:
```c++
ImGui::Image(someTexture); // someTexture is sf::Texture
```
