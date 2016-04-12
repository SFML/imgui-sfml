#include "imgui-SFML.h"
#include <imgui.h>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

#include <cstddef> // offsetof

static sf::Window* s_window = nullptr;
static sf::RenderTarget* s_renderTarget = nullptr;
static sf::Texture* s_fontTexture = nullptr;

static sf::Clock s_deltaClock;
static bool s_mousePressed[5] = { false, false, false, false, false };

namespace 
{

ImVec2 getDisplaySize()
{
    assert(s_renderTarget);
    sf::Vector2f size = static_cast<sf::Vector2f>(s_renderTarget->getSize());
    return ImVec2(size);
}

void RenderDrawLists(ImDrawData* draw_data); // rendering callback function prototype

} // anonymous namespace for helper / "private" functions

namespace ImGui
{
namespace SFML
{

void Init(sf::Window& window, sf::RenderTarget& target)
{
    s_window = &window;

    ImGuiIO& io = ImGui::GetIO();

    // init keyboard mapping
    io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
    io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
    io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
    io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
    io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
    io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
    io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
    io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::BackSpace;
    io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Return;
    io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
    io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
    io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
    io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
    io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
    io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
    io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

    s_deltaClock.restart();

    // init rendering
    s_renderTarget = &target;
    io.DisplaySize = getDisplaySize();
    io.RenderDrawListsFn = RenderDrawLists; // set render callback

    // create font texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (s_fontTexture) { // was already created, delete it
        delete s_fontTexture; 
    }

    s_fontTexture = new sf::Texture;
    s_fontTexture->create(width, height);
    s_fontTexture->update(pixels);
    io.Fonts->TexID = (void*)s_fontTexture;

    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();
}

void Init(sf::RenderWindow& window)
{
    Init(window, window);
}

void ProcessEvent(const sf::Event& event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event.type)
    {
        case sf::Event::MouseButtonPressed: // fall-through
        case sf::Event::MouseButtonReleased:
            s_mousePressed[event.mouseButton.button] = (event.type == sf::Event::MouseButtonPressed);
            break;
        case sf::Event::MouseWheelMoved:
            io.MouseWheel += (float)event.mouseWheel.delta;
            break;
        case sf::Event::KeyPressed: // fall-through
        case sf::Event::KeyReleased:
            io.KeysDown[event.key.code] = (event.type == sf::Event::KeyPressed);
            io.KeyCtrl = event.key.control;
            io.KeyShift = event.key.shift;
            io.KeyAlt = event.key.alt;
            break;
        case sf::Event::TextEntered:
            if (event.text.unicode > 0 && event.text.unicode < 0x10000) {
                io.AddInputCharacter(event.text.unicode);
            }
            break;
        default:
            break;
    }
}

void Update()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = getDisplaySize();
    io.DeltaTime = s_deltaClock.restart().asSeconds(); // restart the clock and get delta

    // update mouse
    assert(s_window);
    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(*s_window));
    io.MousePos = ImVec2(mousePos.x, mousePos.y);
    io.MouseDown[0] = s_mousePressed[0] || sf::Mouse::isButtonPressed(sf::Mouse::Left);
    io.MouseDown[1] = s_mousePressed[1] || sf::Mouse::isButtonPressed(sf::Mouse::Right);
    io.MouseDown[2] = s_mousePressed[2] || sf::Mouse::isButtonPressed(sf::Mouse::Middle);
    s_mousePressed[0] = s_mousePressed[1] = s_mousePressed[2] = false;

    ImGui::NewFrame();
}

void Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->TexID = nullptr;
    delete s_fontTexture;

    s_renderTarget = nullptr;
    s_window = nullptr;
    ImGui::Shutdown();
}

void SetWindow(sf::Window& window)
{
    s_window = &window;
}

void SetRenderTarget(sf::RenderTarget& target)
{
    s_renderTarget = &target;
}

} // end of namespace SFML

void Image(const sf::Texture& texture)
{
    Image(texture, texture.getSize());
}

void Image(const sf::Texture& texture, const sf::Vector2u& size)
{
    ImGui::Image((void*)&texture, static_cast<sf::Vector2f>(size));
}

} // end of namespace ImGui

// Rendering callback

namespace
{
    void RenderDrawLists(ImDrawData* draw_data)
    {
        assert(s_renderTarget);
        if (draw_data->CmdListsCount == 0) {
            return;
        }

        s_renderTarget->pushGLStates();

        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnable(GL_TEXTURE_2D);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, -1.0f, +1.0f);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        for (int n = 0; n < draw_data->CmdListsCount; ++n) {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
            const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

            glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, pos)));
            glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, uv)));
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, col)));

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); ++cmd_i) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback) {
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    sf::Texture* texture = (sf::Texture*)pcmd->TextureId;
                    sf::Vector2u win_size = s_renderTarget->getSize();
                    sf::Texture::bind(texture);
                    glScissor((int)pcmd->ClipRect.x, (int)(win_size.y - pcmd->ClipRect.w),
                        (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer);
                }
                idx_buffer += pcmd->ElemCount;
            }
        }

        // Restore modified state
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();

        s_renderTarget->popGLStates();
        s_renderTarget->resetGLStates();
    }

} // end of anonymous namespace