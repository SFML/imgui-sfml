ImGui + SFML
=======

Library which allows you to use [ImGui](https://github.com/ocornut/imgui) with [SFML](https://github.com/SFML/SFML)

> Use [ImGui-SFML's v1.0](https://github.com/eliasdaler/imgui-sfml/releases/tag/v.1.0) if you're using ImGui's stable release (v.1.53)! This repo's master will be kept up to date with breaking changes in ImGui's master.

![screenshot](https://i2.wp.com/i.imgur.com/iQibpSk.gif)

Based on [this repository](https://github.com/Mischa-Alff/imgui-backends) with big improvements and changes.

How-to
----

- [**Detailed tutorial on my blog**](https://eliasdaler.github.io/using-imgui-with-sfml-pt1)
- [**Using ImGui with modern C++ and STL**](https://eliasdaler.github.io/using-imgui-with-sfml-pt2/)
- [**Thread on SFML forums**](https://en.sfml-dev.org/forums/index.php?topic=20137.0). Feel free to ask your questions there.

Setting up:

- Download [ImGui](https://github.com/ocornut/imgui)
- Add ImGui folder to your include directories
- Add `imgui.cpp` and `imgui_draw.cpp` to your build/project
- Copy the contents of `imconfig-SFML.h` to your `imconfig.h` file. (to be able to cast `ImVec2` to `sf::Vector2f` and vice versa)
- Add a folder which contains `imgui-SFML.h` to your include directories
- Add `imgui-SFML.cpp` to your build/project
- Link OpenGL if you get linking errors

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
    - Call `ImGui::EndFrame` after the last `ImGui::End` in your update function, if you update more than once before rendering. (e.g. fixed delta game loops)
    - Call `ImGui::SFML::Render(window)`

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
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
```

Fonts how-to
---

Default font is loaded if you don't pass `false` in `ImGui::SFML::Init`. Call `ImGui::SFML::Init(window, false);` if you don't want default font to be loaded.

* Load your fonts like this:

```c++
IO.Fonts->Clear(); // clear fonts if you loaded some before (even if only default one was loaded)
// IO.Fonts->AddFontDefault(); // this will load default font as well
IO.Fonts->AddFontFromFileTTF("font1.ttf", 8.f);
IO.Fonts->AddFontFromFileTTF("font2.ttf", 12.f);

ImGui::SFML::UpdateFontTexture(); // important call: updates font texture
```

* And use them like this:

```c++
ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
ImGui::Button("Look at this pretty button");
ImGui::PopFont();

ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
ImGui::TextUnformatted("IT WORKS!");
ImGui::PopFont();
```

The first loaded font is treated as the default one and doesn't need to be pushed with `ImGui::PushFont`.

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

Keyboard/Gamepad navigation
---
Starting with [ImGui 1.60](https://github.com/ocornut/imgui/releases/tag/v1.60), there's a feature to control ImGui with keyboard and gamepad. To use keyboard navigation, you just need to do this:

```c++
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
```

Gamepad navigation requires more work, unless you have XInput gamepad, in which case the mapping is automatically set for you. But you can still set it up for your own gamepad easily, just take a look how it's done for the default mapping [here](https://github.com/eliasdaler/imgui-sfml/blob/navigation/imgui-SFML.cpp#L697). And then you need to do this:

```c++
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
```
By default, the first active joystick is used for navigation, but you can set joystick id explicitly like this:
```c++
ImGui::SFML::SetActiveJoystickId(5);
```

High DPI screens
----

As SFML is not currently DPI aware, your window/gui may show at the incorrect scale. This is particularly noticeable on Apple systems with Retina displays.

To fix this on macOS, you can create an app bundle (as opposed to just the exe) then modify the info.plist so that "High Resolution Capable" is set to "NO"

License
---

This library is licensed under the MIT License, see LICENSE for more information.
