ImGui + SFML
=======

Library which allows you to use [ImGui](https://github.com/ocornut/imgui) with [SFML](https://github.com/SFML/SFML)

Based on [this repository](https://github.com/Mischa-Alff/imgui-backends) with big improvements and changes.

How-to
----

[**Detailed tutorial on my blog**](https://eliasdaler.github.io/using-imgui-with-sfml-pt1)

Setting up:

- Download [ImGui](https://github.com/ocornut/imgui)
- Add ImGui folder to your include directories
- Add `imgui.cpp` and `imgui_draw.cpp` to your build/project
- Copy the contents of `imconfig-SFML.h` to your `imconfig.h` file. (to be able to cast `ImVec2` to `sf::Vector2f` and vice versa)
- Add a folder which contains `imgui-SFML.h` to your include directories
- Add `imgui-SFML.cpp` to your build/project

In your code:

- Call `ImGui::SFML::Init` and pass your `sf::Window` + `sf::RenderTarget` or `sf::RenderWindow` there. You can create your font atlas and pass the pointer in Init too, otherwise the default internal font atlas will be created for you.
- For each iteration of a game loop:
    - Poll and process events:

        ```c++
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            ...
        }
        ```

    - Call `ImGui::SFML::Update(window, deltaTime)` where `deltaTime` is `sf::Time`. You can also pass mousePosition and displaySize yourself instead of passing the window.
    - Call ImGui functions (`ImGui::Begin()`, `ImGui::Button()`, etc.)
    - Call `ImGui::EndFrame` if you update more than once before rendering (you'll need to include `imgui_internal.h` for that)
    - Call `ImGui::Render()`

- Call `ImGui::SFML::Shutdown()` at the end of your program

**If you only draw ImGui widgets without any SFML stuff, then you'll have to call window.resetGLStates() before rendering anything. You only need to do it once.**

Example code
----

See example file [here](examples/main.cpp)

```c++
#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(640, 480), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();
        window.draw(shape);
        ImGui::Render();
        window.display();
    }

    ImGui::SFML::Shutdown();
}
```

CMake how-to
---
 - Checkout the repository as a submoudle
 - Set IMGUI_ROOT 
 - Modify your builds to copy imgui-SFML and dependencies (sfml) to your project
```CMakeLists
add_subdirectory(repos/imgui-sfml)
include_directories("${IMGUI_SFML_INCLUDE_DIRS}")
add_executable(MY_PROJECT ${IMGUI_SOURCES} ${IMGUI_SFML_SOURCES} ${SRCS})
...
target_link_libraries(MY_PROJECT ${IMGUI_SFML_DEPENDENCIES})
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
