#include "imgui-SFML.h"
#include <imgui.h>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Touch.hpp>
#include <SFML/Window/Window.hpp>

#include <cassert>
#include <cmath>    // abs
#include <cstddef>  // offsetof, NULL
#include <cstring>  // memcpy

#ifdef ANDROID
#ifdef USE_JNI

#include <android/native_activity.h>
#include <jni.h>
#include <SFML/System/NativeActivity.hpp>

static bool s_wantTextInput = false;

int openKeyboardIME() {
    ANativeActivity* activity = sf::getNativeActivity();
    JavaVM* vm = activity->vm;
    JNIEnv* env = activity->env;
    JavaVMAttachArgs attachargs;
    attachargs.version = JNI_VERSION_1_6;
    attachargs.name = "NativeThread";
    attachargs.group = NULL;
    jint res = vm->AttachCurrentThread(&env, &attachargs);
    if (res == JNI_ERR) return EXIT_FAILURE;

    jclass natact = env->FindClass("android/app/NativeActivity");
    jclass context = env->FindClass("android/content/Context");

    jfieldID fid = env->GetStaticFieldID(context, "INPUT_METHOD_SERVICE",
                                         "Ljava/lang/String;");
    jobject svcstr = env->GetStaticObjectField(context, fid);

    jmethodID getss = env->GetMethodID(
        natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    jclass imm_cls = env->GetObjectClass(imm_obj);
    jmethodID toggleSoftInput =
        env->GetMethodID(imm_cls, "toggleSoftInput", "(II)V");

    env->CallVoidMethod(imm_obj, toggleSoftInput, 2, 0);

    env->DeleteLocalRef(imm_obj);
    env->DeleteLocalRef(imm_cls);
    env->DeleteLocalRef(svcstr);
    env->DeleteLocalRef(context);
    env->DeleteLocalRef(natact);

    vm->DetachCurrentThread();

    return EXIT_SUCCESS;
}

int closeKeyboardIME() {
    ANativeActivity* activity = sf::getNativeActivity();
    JavaVM* vm = activity->vm;
    JNIEnv* env = activity->env;
    JavaVMAttachArgs attachargs;
    attachargs.version = JNI_VERSION_1_6;
    attachargs.name = "NativeThread";
    attachargs.group = NULL;
    jint res = vm->AttachCurrentThread(&env, &attachargs);
    if (res == JNI_ERR) return EXIT_FAILURE;

    jclass natact = env->FindClass("android/app/NativeActivity");
    jclass context = env->FindClass("android/content/Context");

    jfieldID fid = env->GetStaticFieldID(context, "INPUT_METHOD_SERVICE",
                                         "Ljava/lang/String;");
    jobject svcstr = env->GetStaticObjectField(context, fid);

    jmethodID getss = env->GetMethodID(
        natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    jclass imm_cls = env->GetObjectClass(imm_obj);
    jmethodID toggleSoftInput =
        env->GetMethodID(imm_cls, "toggleSoftInput", "(II)V");

    env->CallVoidMethod(imm_obj, toggleSoftInput, 1, 0);

    env->DeleteLocalRef(imm_obj);
    env->DeleteLocalRef(imm_cls);
    env->DeleteLocalRef(svcstr);
    env->DeleteLocalRef(context);
    env->DeleteLocalRef(natact);

    vm->DetachCurrentThread();

    return EXIT_SUCCESS;
}

#endif
#endif

#if __cplusplus >= 201103L  // C++11 and above
static_assert(sizeof(GLuint) <= sizeof(ImTextureID),
              "ImTextureID is not large enough to fit GLuint.");
#endif

namespace {
// data
static bool s_windowHasFocus = false;
static bool s_mousePressed[3] = {false, false, false};
static bool s_touchDown[3] = {false, false, false};
static bool s_mouseMoved = false;
static sf::Vector2i s_touchPos;
static sf::Texture* s_fontTexture =
    NULL;  // owning pointer to internal font atlas which is used if user
           // doesn't set custom sf::Texture.

static const unsigned int NULL_JOYSTICK_ID = sf::Joystick::Count;
static unsigned int s_joystickId = NULL_JOYSTICK_ID;

static const unsigned int NULL_JOYSTICK_BUTTON = sf::Joystick::ButtonCount;
static unsigned int s_joystickMapping[ImGuiNavInput_COUNT];

struct StickInfo {
    sf::Joystick::Axis xAxis;
    sf::Joystick::Axis yAxis;

    bool xInverted;
    bool yInverted;

    float threshold;
};

StickInfo s_dPadInfo;
StickInfo s_lStickInfo;

// various helper functions
ImVec2 getTopLeftAbsolute(const sf::FloatRect& rect);
ImVec2 getDownRightAbsolute(const sf::FloatRect& rect);

ImTextureID convertGLTextureHandleToImTextureID(GLuint glTextureHandle);
GLuint convertImTextureIDToGLTextureHandle(ImTextureID textureID);

void RenderDrawLists(
    ImDrawData* draw_data);  // rendering callback function prototype

// Implementation of ImageButton overload
bool imageButtonImpl(const sf::Texture& texture,
                     const sf::FloatRect& textureRect, const sf::Vector2f& size,
                     const int framePadding, const sf::Color& bgColor,
                     const sf::Color& tintColor);

// Default mapping is XInput gamepad mapping
void initDefaultJoystickMapping();

// Returns first id of connected joystick
unsigned int getConnectedJoystickId();

void updateJoystickActionState(ImGuiIO& io, ImGuiNavInput_ action);
void updateJoystickDPadState(ImGuiIO& io);
void updateJoystickLStickState(ImGuiIO& io);

// clipboard functions
void setClipboardText(void* userData, const char* text);
const char* getClipboadText(void* userData);
std::string s_clipboardText;

// mouse cursors
void loadMouseCursor(ImGuiMouseCursor imguiCursorType,
                     sf::Cursor::Type sfmlCursorType);
void updateMouseCursor(sf::Window& window);

sf::Cursor* s_mouseCursors[ImGuiMouseCursor_COUNT];
bool s_mouseCursorLoaded[ImGuiMouseCursor_COUNT];

}  // namespace

namespace ImGui {
namespace SFML {

void Init(sf::RenderWindow& window, bool loadDefaultFont) {
    Init(window, window, loadDefaultFont);
}

void Init(sf::Window& window, sf::RenderTarget& target, bool loadDefaultFont) {
    Init(window, static_cast<sf::Vector2f>(target.getSize()), loadDefaultFont);
}

void Init(sf::Window& window, const sf::Vector2f& displaySize, bool loadDefaultFont) {
#if __cplusplus < 201103L  // runtime assert when using earlier than C++11 as no
                           // static_assert support
    assert(
        sizeof(GLuint) <=
        sizeof(ImTextureID));  // ImTextureID is not large enough to fit GLuint.
#endif

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // tell ImGui which features we support
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_sfml";

    // init keyboard mapping
    io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
    io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
    io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
    io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
    io.KeyMap[ImGuiKey_PageUp] = sf::Keyboard::PageUp;
    io.KeyMap[ImGuiKey_PageDown] = sf::Keyboard::PageDown;
    io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
    io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
    io.KeyMap[ImGuiKey_Insert] = sf::Keyboard::Insert;
#ifdef ANDROID
    io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::Delete;
#else
    io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
    io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::BackSpace;
#endif
    io.KeyMap[ImGuiKey_Space] = sf::Keyboard::Space;
    io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Return;
    io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
    io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
    io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
    io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
    io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
    io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
    io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

    s_joystickId = getConnectedJoystickId();

    for (unsigned int i = 0; i < ImGuiNavInput_COUNT; i++) {
        s_joystickMapping[i] = NULL_JOYSTICK_BUTTON;
    }

    initDefaultJoystickMapping();

    // init rendering
    io.DisplaySize = displaySize;

    // clipboard
    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboadText;

    // load mouse cursors
    for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
        s_mouseCursorLoaded[i] = false;
    }

    loadMouseCursor(ImGuiMouseCursor_Arrow, sf::Cursor::Arrow);
    loadMouseCursor(ImGuiMouseCursor_TextInput, sf::Cursor::Text);
    loadMouseCursor(ImGuiMouseCursor_ResizeAll, sf::Cursor::SizeAll);
    loadMouseCursor(ImGuiMouseCursor_ResizeNS, sf::Cursor::SizeVertical);
    loadMouseCursor(ImGuiMouseCursor_ResizeEW, sf::Cursor::SizeHorizontal);
    loadMouseCursor(ImGuiMouseCursor_ResizeNESW,
                    sf::Cursor::SizeBottomLeftTopRight);
    loadMouseCursor(ImGuiMouseCursor_ResizeNWSE,
                    sf::Cursor::SizeTopLeftBottomRight);
    loadMouseCursor(ImGuiMouseCursor_Hand, sf::Cursor::Hand);

    if (s_fontTexture) {  // delete previously created texture
        delete s_fontTexture;
    }
    s_fontTexture = new sf::Texture;

    if (loadDefaultFont) {
        // this will load default font automatically
        // No need to call AddDefaultFont
        UpdateFontTexture();
    }

    s_windowHasFocus = window.hasFocus();
}

void ProcessEvent(const sf::Event& event) {
    if (s_windowHasFocus) {
        ImGuiIO& io = ImGui::GetIO();

        switch (event.type) {
            case sf::Event::MouseMoved:
                s_mouseMoved = true;
                break;
            case sf::Event::MouseButtonPressed:  // fall-through
            case sf::Event::MouseButtonReleased: {
                int button = event.mouseButton.button;
                if (event.type == sf::Event::MouseButtonPressed &&
                    button >= 0 && button < 3) {
                    s_mousePressed[event.mouseButton.button] = true;
                }
            } break;
            case sf::Event::TouchBegan:  // fall-through
            case sf::Event::TouchEnded: {
                s_mouseMoved = false;
                int button = event.touch.finger;
                if (event.type == sf::Event::TouchBegan && button >= 0 &&
                    button < 3) {
                    s_touchDown[event.touch.finger] = true;
                }
            } break;
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel ||
                    (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel &&
                    io.KeyShift)) {
                    io.MouseWheel += event.mouseWheelScroll.delta;
                } else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
                    io.MouseWheelH += event.mouseWheelScroll.delta;
                }
                break;
            case sf::Event::KeyPressed:  // fall-through
            case sf::Event::KeyReleased:
                io.KeysDown[event.key.code] =
                    (event.type == sf::Event::KeyPressed);
                break;
            case sf::Event::TextEntered:
                // Don't handle the event for unprintable characters
                if (event.text.unicode < ' ' || event.text.unicode == 127) {
                    break;
                }
                io.AddInputCharacter(event.text.unicode);
                break;
            case sf::Event::JoystickConnected:
                if (s_joystickId == NULL_JOYSTICK_ID) {
                    s_joystickId = event.joystickConnect.joystickId;
                }
                break;
            case sf::Event::JoystickDisconnected:
                if (s_joystickId ==
                    event.joystickConnect
                        .joystickId) {  // used gamepad was disconnected
                    s_joystickId = getConnectedJoystickId();
                }
                break;
            default:
                break;
        }
    }

    switch (event.type) {
        case sf::Event::LostFocus:
            s_windowHasFocus = false;
            break;
        case sf::Event::GainedFocus:
            s_windowHasFocus = true;
            break;
        default:
            break;
    }
}

void Update(sf::RenderWindow& window, sf::Time dt) {
    Update(window, window, dt);
}

void Update(sf::Window& window, sf::RenderTarget& target, sf::Time dt) {
    // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
    updateMouseCursor(window);

    if (!s_mouseMoved) {
        if (sf::Touch::isDown(0))
            s_touchPos = sf::Touch::getPosition(0, window);

        Update(s_touchPos, static_cast<sf::Vector2f>(target.getSize()), dt);
    } else {
        Update(sf::Mouse::getPosition(window),
               static_cast<sf::Vector2f>(target.getSize()), dt);
    }

    if (ImGui::GetIO().MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it
        window.setMouseCursorVisible(false);
    }
}

void Update(const sf::Vector2i& mousePos, const sf::Vector2f& displaySize,
            sf::Time dt) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = displaySize;
    io.DeltaTime = dt.asSeconds();

    if (s_windowHasFocus) {
        if (io.WantSetMousePos) {
            sf::Vector2i mousePos(static_cast<int>(io.MousePos.x),
                                  static_cast<int>(io.MousePos.y));
            sf::Mouse::setPosition(mousePos);
        } else {
            io.MousePos = mousePos;
        }
        for (unsigned int i = 0; i < 3; i++) {
            io.MouseDown[i] = s_touchDown[i] || sf::Touch::isDown(i) ||
                              s_mousePressed[i] ||
                              sf::Mouse::isButtonPressed((sf::Mouse::Button)i);
            s_mousePressed[i] = false;
            s_touchDown[i] = false;
        }
    }

    // Update Ctrl, Shift, Alt, Super state
    io.KeyCtrl = io.KeysDown[sf::Keyboard::LControl] ||
                 io.KeysDown[sf::Keyboard::RControl];
    io.KeyAlt =
        io.KeysDown[sf::Keyboard::LAlt] || io.KeysDown[sf::Keyboard::RAlt];
    io.KeyShift =
        io.KeysDown[sf::Keyboard::LShift] || io.KeysDown[sf::Keyboard::RShift];
    io.KeySuper = io.KeysDown[sf::Keyboard::LSystem] ||
                  io.KeysDown[sf::Keyboard::RSystem];

#ifdef ANDROID
#ifdef USE_JNI
    if (io.WantTextInput && !s_wantTextInput) {
        openKeyboardIME();
        s_wantTextInput = true;
    }

    if (!io.WantTextInput && s_wantTextInput) {
        closeKeyboardIME();
        s_wantTextInput = false;
    }
#endif
#endif

    assert(io.Fonts->Fonts.Size > 0);  // You forgot to create and set up font
                                       // atlas (see createFontTexture)

    // gamepad navigation
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) &&
        s_joystickId != NULL_JOYSTICK_ID) {
        updateJoystickActionState(io, ImGuiNavInput_Activate);
        updateJoystickActionState(io, ImGuiNavInput_Cancel);
        updateJoystickActionState(io, ImGuiNavInput_Input);
        updateJoystickActionState(io, ImGuiNavInput_Menu);

        updateJoystickActionState(io, ImGuiNavInput_FocusPrev);
        updateJoystickActionState(io, ImGuiNavInput_FocusNext);

        updateJoystickActionState(io, ImGuiNavInput_TweakSlow);
        updateJoystickActionState(io, ImGuiNavInput_TweakFast);

        updateJoystickDPadState(io);
        updateJoystickLStickState(io);
    }

    ImGui::NewFrame();
}

void Render(sf::RenderTarget& target) {
    target.resetGLStates();
    ImGui::Render();
    RenderDrawLists(ImGui::GetDrawData());
}

void Render() {
    ImGui::Render();
    RenderDrawLists(ImGui::GetDrawData());
}

void Shutdown() {
    ImGui::GetIO().Fonts->TexID = (ImTextureID)NULL;

    if (s_fontTexture) {  // if internal texture was created, we delete it
        delete s_fontTexture;
        s_fontTexture = NULL;
    }

    for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
        if (s_mouseCursorLoaded[i]) {
            delete s_mouseCursors[i];
            s_mouseCursors[i] = NULL;

            s_mouseCursorLoaded[i] = false;
        }
    }

    ImGui::DestroyContext();
}

void UpdateFontTexture() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    sf::Texture& texture = *s_fontTexture;
    texture.create(width, height);
    texture.update(pixels);

    io.Fonts->TexID =
        convertGLTextureHandleToImTextureID(texture.getNativeHandle());
}

sf::Texture& GetFontTexture() { return *s_fontTexture; }

void SetActiveJoystickId(unsigned int joystickId) {
    assert(joystickId < sf::Joystick::Count);
    s_joystickId = joystickId;
}

void SetJoytickDPadThreshold(float threshold) {
    assert(threshold >= 0.f && threshold <= 100.f);
    s_dPadInfo.threshold = threshold;
}

void SetJoytickLStickThreshold(float threshold) {
    assert(threshold >= 0.f && threshold <= 100.f);
    s_lStickInfo.threshold = threshold;
}

void SetJoystickMapping(int action, unsigned int joystickButton) {
    assert(action < ImGuiNavInput_COUNT);
    assert(joystickButton < sf::Joystick::ButtonCount);
    s_joystickMapping[action] = joystickButton;
}

void SetDPadXAxis(sf::Joystick::Axis dPadXAxis, bool inverted) {
    s_dPadInfo.xAxis = dPadXAxis;
    s_dPadInfo.xInverted = inverted;
}

void SetDPadYAxis(sf::Joystick::Axis dPadYAxis, bool inverted) {
    s_dPadInfo.yAxis = dPadYAxis;
    s_dPadInfo.yInverted = inverted;
}

void SetLStickXAxis(sf::Joystick::Axis lStickXAxis, bool inverted) {
    s_lStickInfo.xAxis = lStickXAxis;
    s_lStickInfo.xInverted = inverted;
}

void SetLStickYAxis(sf::Joystick::Axis lStickYAxis, bool inverted) {
    s_lStickInfo.yAxis = lStickYAxis;
    s_lStickInfo.yInverted = inverted;
}

}  // end of namespace SFML

/////////////// Image Overloads

void Image(const sf::Texture& texture, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    Image(texture, static_cast<sf::Vector2f>(texture.getSize()), tintColor,
          borderColor);
}

void Image(const sf::Texture& texture, const sf::Vector2f& size,
           const sf::Color& tintColor, const sf::Color& borderColor) {
    ImTextureID textureID =
        convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    ImGui::Image(textureID, size, ImVec2(0, 0), ImVec2(1, 1), tintColor,
                 borderColor);
}

void Image(const sf::Texture& texture, const sf::FloatRect& textureRect,
           const sf::Color& tintColor, const sf::Color& borderColor) {
    Image(
        texture,
        sf::Vector2f(std::abs(textureRect.width), std::abs(textureRect.height)),
        textureRect, tintColor, borderColor);
}

void Image(const sf::Texture& texture, const sf::Vector2f& size,
           const sf::FloatRect& textureRect, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());
    ImVec2 uv0(textureRect.left / textureSize.x,
               textureRect.top / textureSize.y);
    ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x,
               (textureRect.top + textureRect.height) / textureSize.y);

    ImTextureID textureID =
        convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    ImGui::Image(textureID, size, uv0, uv1, tintColor, borderColor);
}

void Image(const sf::Sprite& sprite, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    Image(sprite, sf::Vector2f(bounds.width, bounds.height), tintColor,
          borderColor);
}

void Image(const sf::Sprite& sprite, const sf::Vector2f& size,
           const sf::Color& tintColor, const sf::Color& borderColor) {
    const sf::Texture* texturePtr = sprite.getTexture();
    // sprite without texture cannot be drawn
    if (!texturePtr) {
        return;
    }

    Image(*texturePtr, size,
          static_cast<sf::FloatRect>(sprite.getTextureRect()), tintColor,
          borderColor);
}

/////////////// Image Button Overloads

bool ImageButton(const sf::Texture& texture, const int framePadding,
                 const sf::Color& bgColor, const sf::Color& tintColor) {
    return ImageButton(texture, static_cast<sf::Vector2f>(texture.getSize()),
                       framePadding, bgColor, tintColor);
}

bool ImageButton(const sf::Texture& texture, const sf::Vector2f& size,
                 const int framePadding, const sf::Color& bgColor,
                 const sf::Color& tintColor) {
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());
    return ::imageButtonImpl(
        texture, sf::FloatRect(0.f, 0.f, textureSize.x, textureSize.y), size,
        framePadding, bgColor, tintColor);
}

bool ImageButton(const sf::Sprite& sprite, const int framePadding,
                 const sf::Color& bgColor, const sf::Color& tintColor) {
    sf::FloatRect spriteSize = sprite.getGlobalBounds();
    return ImageButton(sprite,
                       sf::Vector2f(spriteSize.width, spriteSize.height),
                       framePadding, bgColor, tintColor);
}

bool ImageButton(const sf::Sprite& sprite, const sf::Vector2f& size,
                 const int framePadding, const sf::Color& bgColor,
                 const sf::Color& tintColor) {
    const sf::Texture* texturePtr = sprite.getTexture();
    if (!texturePtr) {
        return false;
    }
    return ::imageButtonImpl(
        *texturePtr, static_cast<sf::FloatRect>(sprite.getTextureRect()), size,
        framePadding, bgColor, tintColor);
}

/////////////// Draw_list Overloads

void DrawLine(const sf::Vector2f& a, const sf::Vector2f& b,
              const sf::Color& color, float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    sf::Vector2f pos = ImGui::GetCursorScreenPos();
    draw_list->AddLine(a + pos, b + pos, ColorConvertFloat4ToU32(color),
                       thickness);
}

void DrawRect(const sf::FloatRect& rect, const sf::Color& color, float rounding,
              int rounding_corners, float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(getTopLeftAbsolute(rect), getDownRightAbsolute(rect),
                       ColorConvertFloat4ToU32(color), rounding,
                       rounding_corners, thickness);
}

void DrawRectFilled(const sf::FloatRect& rect, const sf::Color& color,
                    float rounding, int rounding_corners) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        getTopLeftAbsolute(rect), getDownRightAbsolute(rect),
        ColorConvertFloat4ToU32(color), rounding, rounding_corners);
}

}  // end of namespace ImGui

namespace {

ImVec2 getTopLeftAbsolute(const sf::FloatRect& rect) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    return ImVec2(rect.left + pos.x, rect.top + pos.y);
}
ImVec2 getDownRightAbsolute(const sf::FloatRect& rect) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    return ImVec2(rect.left + rect.width + pos.x,
                  rect.top + rect.height + pos.y);
}

ImTextureID convertGLTextureHandleToImTextureID(GLuint glTextureHandle) {
    ImTextureID textureID = (ImTextureID)NULL;
    std::memcpy(&textureID, &glTextureHandle, sizeof(GLuint));
    return textureID;
}
GLuint convertImTextureIDToGLTextureHandle(ImTextureID textureID) {
    GLuint glTextureHandle;
    std::memcpy(&glTextureHandle, &textureID, sizeof(GLuint));
    return glTextureHandle;
}

// Rendering callback
void RenderDrawLists(ImDrawData* draw_data) {
    ImGui::GetDrawData();
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    assert(io.Fonts->TexID !=
           (ImTextureID)NULL);  // You forgot to create and set font texture

    // scale stuff (needed for proper handling of window resize)
    int fb_width =
        static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height =
        static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

#ifdef GL_VERSION_ES_CL_1_1
    GLint last_program, last_texture, last_array_buffer,
        last_element_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
#else
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

#ifdef GL_VERSION_ES_CL_1_1
    glOrthof(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#else
    glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#endif

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (int n = 0; n < draw_data->CmdListsCount; ++n) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const unsigned char* vtx_buffer =
            (const unsigned char*)&cmd_list->VtxBuffer.front();
        const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                        (void*)(vtx_buffer + offsetof(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                          (void*)(vtx_buffer + offsetof(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
                       (void*)(vtx_buffer + offsetof(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); ++cmd_i) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                GLuint textureHandle =
                    convertImTextureIDToGLTextureHandle(pcmd->TextureId);
                glBindTexture(GL_TEXTURE_2D, textureHandle);
                glScissor((int)pcmd->ClipRect.x,
                          (int)(fb_height - pcmd->ClipRect.w),
                          (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                               GL_UNSIGNED_SHORT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
#ifdef GL_VERSION_ES_CL_1_1
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glDisable(GL_SCISSOR_TEST);
#else
    glPopAttrib();
#endif
}

bool imageButtonImpl(const sf::Texture& texture,
                     const sf::FloatRect& textureRect, const sf::Vector2f& size,
                     const int framePadding, const sf::Color& bgColor,
                     const sf::Color& tintColor) {
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());

    ImVec2 uv0(textureRect.left / textureSize.x,
               textureRect.top / textureSize.y);
    ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x,
               (textureRect.top + textureRect.height) / textureSize.y);

    ImTextureID textureID =
        convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    return ImGui::ImageButton(textureID, size, uv0, uv1, framePadding, bgColor,
                              tintColor);
}

unsigned int getConnectedJoystickId() {
    for (unsigned int i = 0; i < (unsigned int)sf::Joystick::Count; ++i) {
        if (sf::Joystick::isConnected(i)) return i;
    }

    return NULL_JOYSTICK_ID;
}

void initDefaultJoystickMapping() {
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_Activate, 0);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_Cancel, 1);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_Input, 3);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_Menu, 2);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_FocusPrev, 4);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_FocusNext, 5);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_TweakSlow, 4);
    ImGui::SFML::SetJoystickMapping(ImGuiNavInput_TweakFast, 5);

    ImGui::SFML::SetDPadXAxis(sf::Joystick::PovX);
    // D-pad Y axis is inverted on Windows
#ifdef _WIN32
    ImGui::SFML::SetDPadYAxis(sf::Joystick::PovY, true);
#else
    ImGui::SFML::SetDPadYAxis(sf::Joystick::PovY);
#endif

    ImGui::SFML::SetLStickXAxis(sf::Joystick::X);
    ImGui::SFML::SetLStickYAxis(sf::Joystick::Y);

    ImGui::SFML::SetJoytickDPadThreshold(5.f);
    ImGui::SFML::SetJoytickLStickThreshold(5.f);
}

void updateJoystickActionState(ImGuiIO& io, ImGuiNavInput_ action) {
    bool isPressed =
        sf::Joystick::isButtonPressed(s_joystickId, s_joystickMapping[action]);
    io.NavInputs[action] = isPressed ? 1.0f : 0.0f;
}

void updateJoystickDPadState(ImGuiIO& io) {
    float dpadXPos =
        sf::Joystick::getAxisPosition(s_joystickId, s_dPadInfo.xAxis);
    if (s_dPadInfo.xInverted) dpadXPos = -dpadXPos;

    float dpadYPos =
        sf::Joystick::getAxisPosition(s_joystickId, s_dPadInfo.yAxis);
    if (s_dPadInfo.yInverted) dpadYPos = -dpadYPos;

    io.NavInputs[ImGuiNavInput_DpadLeft] =
        dpadXPos < -s_dPadInfo.threshold ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadRight] =
        dpadXPos > s_dPadInfo.threshold ? 1.0f : 0.0f;

    io.NavInputs[ImGuiNavInput_DpadUp] =
        dpadYPos < -s_dPadInfo.threshold ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown] =
        dpadYPos > s_dPadInfo.threshold ? 1.0f : 0.0f;
}

void updateJoystickLStickState(ImGuiIO& io) {
    float lStickXPos =
        sf::Joystick::getAxisPosition(s_joystickId, s_lStickInfo.xAxis);
    if (s_lStickInfo.xInverted) lStickXPos = -lStickXPos;

    float lStickYPos =
        sf::Joystick::getAxisPosition(s_joystickId, s_lStickInfo.yAxis);
    if (s_lStickInfo.yInverted) lStickYPos = -lStickYPos;

    if (lStickXPos < -s_lStickInfo.threshold) {
        io.NavInputs[ImGuiNavInput_LStickLeft] = std::abs(lStickXPos / 100.f);
    }

    if (lStickXPos > s_lStickInfo.threshold) {
        io.NavInputs[ImGuiNavInput_LStickRight] = lStickXPos / 100.f;
    }

    if (lStickYPos < -s_lStickInfo.threshold) {
        io.NavInputs[ImGuiNavInput_LStickUp] = std::abs(lStickYPos / 100.f);
    }

    if (lStickYPos > s_lStickInfo.threshold) {
        io.NavInputs[ImGuiNavInput_LStickDown] = lStickYPos / 100.f;
    }
}

void setClipboardText(void* /*userData*/, const char* text) {
    sf::Clipboard::setString(sf::String::fromUtf8(text, text + std::strlen(text)));
}

const char* getClipboadText(void* /*userData*/) {
    std::basic_string<sf::Uint8> tmp = sf::Clipboard::getString().toUtf8();
    s_clipboardText = std::string(tmp.begin(), tmp.end());
    return s_clipboardText.c_str();
}

void loadMouseCursor(ImGuiMouseCursor imguiCursorType,
                     sf::Cursor::Type sfmlCursorType) {
    s_mouseCursors[imguiCursorType] = new sf::Cursor();
    s_mouseCursorLoaded[imguiCursorType] =
        s_mouseCursors[imguiCursorType]->loadFromSystem(sfmlCursorType);
}

void updateMouseCursor(sf::Window& window) {
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0) {
        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
            window.setMouseCursorVisible(false);
        } else {
            window.setMouseCursorVisible(true);

            sf::Cursor& c = s_mouseCursorLoaded[cursor]
                                ? *s_mouseCursors[cursor]
                                : *s_mouseCursors[ImGuiMouseCursor_Arrow];
            window.setMouseCursor(c);
        }
    }
}

}  // end of anonymous namespace
