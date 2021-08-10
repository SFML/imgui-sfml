#include "imgui-SFML.h"
#include <imgui.h>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
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
#include <cmath> // abs
#include <cstddef> // offsetof, NULL
#include <cstring> // memcpy

#ifdef ANDROID
#ifdef USE_JNI

#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#include <jni.h>

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

    jfieldID fid = env->GetStaticFieldID(context, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject svcstr = env->GetStaticObjectField(context, fid);

    jmethodID getss =
        env->GetMethodID(natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    jclass imm_cls = env->GetObjectClass(imm_obj);
    jmethodID toggleSoftInput = env->GetMethodID(imm_cls, "toggleSoftInput", "(II)V");

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

    jfieldID fid = env->GetStaticFieldID(context, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject svcstr = env->GetStaticObjectField(context, fid);

    jmethodID getss =
        env->GetMethodID(natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    jclass imm_cls = env->GetObjectClass(imm_obj);
    jmethodID toggleSoftInput = env->GetMethodID(imm_cls, "toggleSoftInput", "(II)V");

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

#if __cplusplus >= 201103L // C++11 and above
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
static sf::Texture* s_fontTexture = NULL; // owning pointer to internal font atlas which is used
                                          // if user doesn't set custom sf::Texture.

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
ImColor toImColor(sf::Color c);
ImVec2 getTopLeftAbsolute(const sf::FloatRect& rect);
ImVec2 getDownRightAbsolute(const sf::FloatRect& rect);

ImTextureID convertGLTextureHandleToImTextureID(GLuint glTextureHandle);
GLuint convertImTextureIDToGLTextureHandle(ImTextureID textureID);

void RenderDrawLists(ImDrawData* draw_data); // rendering callback function prototype

// Default mapping is XInput gamepad mapping
void initDefaultJoystickMapping();

// Returns first id of connected joystick
unsigned int getConnectedJoystickId();

void updateJoystickActionState(ImGuiIO& io, ImGuiNavInput_ action);
void updateJoystickDPadState(ImGuiIO& io);
void updateJoystickLStickState(ImGuiIO& io);

// clipboard functions
void setClipboardText(void* userData, const char* text);
const char* getClipboardText(void* userData);
std::string s_clipboardText;

// mouse cursors
void loadMouseCursor(ImGuiMouseCursor imguiCursorType, sf::Cursor::Type sfmlCursorType);
void updateMouseCursor(sf::Window& window);

sf::Cursor* s_mouseCursors[ImGuiMouseCursor_COUNT];
bool s_mouseCursorLoaded[ImGuiMouseCursor_COUNT];

} // end of anonymous namespace

namespace ImGui {
namespace SFML {
void Init(sf::RenderWindow& window, bool loadDefaultFont) {
    Init(window, window, loadDefaultFont);
}

void Init(sf::Window& window, sf::RenderTarget& target, bool loadDefaultFont) {
    Init(window, static_cast<sf::Vector2f>(target.getSize()), loadDefaultFont);
}

void Init(sf::Window& window, const sf::Vector2f& displaySize, bool loadDefaultFont) {
#if __cplusplus < 201103L // runtime assert when using earlier than C++11 as no
                          // static_assert support
    assert(sizeof(GLuint) <= sizeof(ImTextureID)); // ImTextureID is not large enough to fit
                                                   // GLuint.
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
    io.DisplaySize = ImVec2(displaySize.x, displaySize.y);

    // clipboard
    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;

    // load mouse cursors
    for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
        s_mouseCursorLoaded[i] = false;
    }

    loadMouseCursor(ImGuiMouseCursor_Arrow, sf::Cursor::Arrow);
    loadMouseCursor(ImGuiMouseCursor_TextInput, sf::Cursor::Text);
    loadMouseCursor(ImGuiMouseCursor_ResizeAll, sf::Cursor::SizeAll);
    loadMouseCursor(ImGuiMouseCursor_ResizeNS, sf::Cursor::SizeVertical);
    loadMouseCursor(ImGuiMouseCursor_ResizeEW, sf::Cursor::SizeHorizontal);
    loadMouseCursor(ImGuiMouseCursor_ResizeNESW, sf::Cursor::SizeBottomLeftTopRight);
    loadMouseCursor(ImGuiMouseCursor_ResizeNWSE, sf::Cursor::SizeTopLeftBottomRight);
    loadMouseCursor(ImGuiMouseCursor_Hand, sf::Cursor::Hand);

    if (s_fontTexture) { // delete previously created texture
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
        case sf::Event::MouseButtonPressed: // fall-through
        case sf::Event::MouseButtonReleased: {
            int button = event.mouseButton.button;
            if (event.type == sf::Event::MouseButtonPressed && button >= 0 && button < 3) {
                s_mousePressed[event.mouseButton.button] = true;
            }
        } break;
        case sf::Event::TouchBegan: // fall-through
        case sf::Event::TouchEnded: {
            s_mouseMoved = false;
            int button = event.touch.finger;
            if (event.type == sf::Event::TouchBegan && button >= 0 && button < 3) {
                s_touchDown[event.touch.finger] = true;
            }
        } break;
        case sf::Event::MouseWheelScrolled:
            if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel ||
                (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel && io.KeyShift)) {
                io.MouseWheel += event.mouseWheelScroll.delta;
            } else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
                io.MouseWheelH += event.mouseWheelScroll.delta;
            }
            break;
        case sf::Event::KeyPressed: // fall-through
        case sf::Event::KeyReleased: {
            int key = event.key.code;
            if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown)) {
                io.KeysDown[key] = (event.type == sf::Event::KeyPressed);
            }
            io.KeyCtrl = event.key.control;
            io.KeyAlt = event.key.alt;
            io.KeyShift = event.key.shift;
            io.KeySuper = event.key.system;
        } break;
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
            if (s_joystickId == event.joystickConnect.joystickId) { // used gamepad was
                                                                    // disconnected
                s_joystickId = getConnectedJoystickId();
            }
            break;
        default:
            break;
        }
    }

    switch (event.type) {
    case sf::Event::LostFocus: {
        // reset all input - SFML doesn't send KeyReleased
        // event when window goes out of focus
        ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); ++i) {
            io.KeysDown[i] = false;
        }
        io.KeyCtrl = false;
        io.KeyAlt = false;
        io.KeyShift = false;
        io.KeySuper = false;

        s_windowHasFocus = false;
    } break;
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
        if (sf::Touch::isDown(0)) s_touchPos = sf::Touch::getPosition(0, window);

        Update(s_touchPos, static_cast<sf::Vector2f>(target.getSize()), dt);
    } else {
        Update(sf::Mouse::getPosition(window), static_cast<sf::Vector2f>(target.getSize()), dt);
    }

    if (ImGui::GetIO().MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it
        window.setMouseCursorVisible(false);
    }
}

void Update(const sf::Vector2i& mousePos, const sf::Vector2f& displaySize, sf::Time dt) {
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(displaySize.x, displaySize.y);
    io.DeltaTime = dt.asSeconds();

    if (s_windowHasFocus) {
        if (io.WantSetMousePos) {
            sf::Vector2i newMousePos(static_cast<int>(io.MousePos.x),
                                     static_cast<int>(io.MousePos.y));
            sf::Mouse::setPosition(newMousePos);
        } else {
            io.MousePos = ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        }
        for (unsigned int i = 0; i < 3; i++) {
            io.MouseDown[i] = s_touchDown[i] || sf::Touch::isDown(i) || s_mousePressed[i] ||
                              sf::Mouse::isButtonPressed((sf::Mouse::Button)i);
            s_mousePressed[i] = false;
            s_touchDown[i] = false;
        }
    }

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

    assert(io.Fonts->Fonts.Size > 0); // You forgot to create and set up font
                                      // atlas (see createFontTexture)

    // gamepad navigation
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) && s_joystickId != NULL_JOYSTICK_ID) {
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
    target.pushGLStates();
    ImGui::Render();
    RenderDrawLists(ImGui::GetDrawData());
    target.popGLStates();
}

void Render() {
    ImGui::Render();
    RenderDrawLists(ImGui::GetDrawData());
}

void Shutdown() {
    ImGui::GetIO().Fonts->SetTexID(0);

    if (s_fontTexture) { // if internal texture was created, we delete it
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

    ImTextureID texID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    io.Fonts->SetTexID(texID);
}

sf::Texture& GetFontTexture() {
    return *s_fontTexture;
}

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

} // end of namespace SFML

/////////////// Image Overloads for sf::Texture

void Image(const sf::Texture& texture, const sf::Color& tintColor, const sf::Color& borderColor) {
    Image(texture, static_cast<sf::Vector2f>(texture.getSize()), tintColor, borderColor);
}

void Image(const sf::Texture& texture, const sf::Vector2f& size, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());

    ImGui::Image(textureID, ImVec2(size.x, size.y), ImVec2(0, 0), ImVec2(1, 1),
                 toImColor(tintColor), toImColor(borderColor));
}

/////////////// Image Overloads for sf::RenderTexture
void Image(const sf::RenderTexture& texture, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    Image(texture, static_cast<sf::Vector2f>(texture.getSize()), tintColor, borderColor);
}

void Image(const sf::RenderTexture& texture, const sf::Vector2f& size, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    ImTextureID textureID =
        convertGLTextureHandleToImTextureID(texture.getTexture().getNativeHandle());

    ImGui::Image(textureID, ImVec2(size.x, size.y), ImVec2(0, 1),
                 ImVec2(1, 0), // flipped vertically, because textures in sf::RenderTexture are
                               // stored this way
                 toImColor(tintColor), toImColor(borderColor));
}

/////////////// Image Overloads for sf::Sprite

void Image(const sf::Sprite& sprite, const sf::Color& tintColor, const sf::Color& borderColor) {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    Image(sprite, sf::Vector2f(bounds.width, bounds.height), tintColor, borderColor);
}

void Image(const sf::Sprite& sprite, const sf::Vector2f& size, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    const sf::Texture* texturePtr = sprite.getTexture();
    // sprite without texture cannot be drawn
    if (!texturePtr) {
        return;
    }

    const sf::Texture& texture = *texturePtr;
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());
    const sf::IntRect& textureRect = sprite.getTextureRect();
    ImVec2 uv0(textureRect.left / textureSize.x, textureRect.top / textureSize.y);
    ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x,
               (textureRect.top + textureRect.height) / textureSize.y);

    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());

    ImGui::Image(textureID, ImVec2(size.x, size.y), uv0, uv1, toImColor(tintColor),
                 toImColor(borderColor));
}

/////////////// Image Button Overloads for sf::Texture

bool ImageButton(const sf::Texture& texture, const int framePadding, const sf::Color& bgColor,
                 const sf::Color& tintColor) {
    return ImageButton(texture, static_cast<sf::Vector2f>(texture.getSize()), framePadding, bgColor,
                       tintColor);
}

bool ImageButton(const sf::Texture& texture, const sf::Vector2f& size, const int framePadding,
                 const sf::Color& bgColor, const sf::Color& tintColor) {
    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());

    return ImGui::ImageButton(textureID, ImVec2(size.x, size.y), ImVec2(0, 0), ImVec2(1, 1),
                              framePadding, toImColor(bgColor), toImColor(tintColor));
}

/////////////// Image Button Overloads for sf::RenderTexture

bool ImageButton(const sf::RenderTexture& texture, const int framePadding, const sf::Color& bgColor,
                 const sf::Color& tintColor) {
    return ImageButton(texture, static_cast<sf::Vector2f>(texture.getSize()), framePadding, bgColor,
                       tintColor);
}

bool ImageButton(const sf::RenderTexture& texture, const sf::Vector2f& size, const int framePadding,
                 const sf::Color& bgColor, const sf::Color& tintColor) {
    ImTextureID textureID =
        convertGLTextureHandleToImTextureID(texture.getTexture().getNativeHandle());

    return ImGui::ImageButton(textureID, ImVec2(size.x, size.y), ImVec2(0, 1),
                              ImVec2(1, 0), // flipped vertically, because textures in
                                            // sf::RenderTexture are stored this way
                              framePadding, toImColor(bgColor), toImColor(tintColor));
}

/////////////// Image Button Overloads for sf::Sprite

bool ImageButton(const sf::Sprite& sprite, const int framePadding, const sf::Color& bgColor,
                 const sf::Color& tintColor) {
    sf::FloatRect spriteSize = sprite.getGlobalBounds();
    return ImageButton(sprite, sf::Vector2f(spriteSize.width, spriteSize.height), framePadding,
                       bgColor, tintColor);
}

bool ImageButton(const sf::Sprite& sprite, const sf::Vector2f& size, const int framePadding,
                 const sf::Color& bgColor, const sf::Color& tintColor) {
    const sf::Texture* texturePtr = sprite.getTexture();
    // sprite without texture cannot be drawn
    if (!texturePtr) {
        return false;
    }

    const sf::Texture& texture = *texturePtr;
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());
    const sf::IntRect& textureRect = sprite.getTextureRect();
    ImVec2 uv0(textureRect.left / textureSize.x, textureRect.top / textureSize.y);
    ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x,
               (textureRect.top + textureRect.height) / textureSize.y);

    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());
    return ImGui::ImageButton(textureID, ImVec2(size.x, size.y), uv0, uv1, framePadding,
                              toImColor(bgColor), toImColor(tintColor));
}

/////////////// Draw_list Overloads

void DrawLine(const sf::Vector2f& a, const sf::Vector2f& b, const sf::Color& color,
              float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    draw_list->AddLine(ImVec2(a.x + pos.x, a.y + pos.y), ImVec2(b.x + pos.x, b.y + pos.y),
                       ColorConvertFloat4ToU32(toImColor(color)), thickness);
}

void DrawRect(const sf::FloatRect& rect, const sf::Color& color, float rounding,
              int rounding_corners, float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(getTopLeftAbsolute(rect), getDownRightAbsolute(rect),
                       ColorConvertFloat4ToU32(toImColor(color)), rounding, rounding_corners,
                       thickness);
}

void DrawRectFilled(const sf::FloatRect& rect, const sf::Color& color, float rounding,
                    int rounding_corners) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(getTopLeftAbsolute(rect), getDownRightAbsolute(rect),
                             ColorConvertFloat4ToU32(toImColor(color)), rounding, rounding_corners);
}

} // end of namespace ImGui

namespace {
ImColor toImColor(sf::Color c) {
    return ImColor(static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b),
                   static_cast<int>(c.a));
}
ImVec2 getTopLeftAbsolute(const sf::FloatRect& rect) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    return ImVec2(rect.left + pos.x, rect.top + pos.y);
}
ImVec2 getDownRightAbsolute(const sf::FloatRect& rect) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    return ImVec2(rect.left + rect.width + pos.x, rect.top + rect.height + pos.y);
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

// copied from imgui/backends/imgui_impl_opengl2.cpp
void SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height) {
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor
    // enabled, vertex/texcoord/color pointers, polygon fill.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); //
    // In order to composite our output buffer we need to preserve alpha
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_SMOOTH);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single
    // viewport apps.
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
#ifdef GL_VERSION_ES_CL_1_1
    glOrthof(draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x,
             draw_data->DisplayPos.y + draw_data->DisplaySize.y, draw_data->DisplayPos.y, -1.0f,
             +1.0f);
#else
    glOrtho(draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x,
            draw_data->DisplayPos.y + draw_data->DisplaySize.y, draw_data->DisplayPos.y, -1.0f,
            +1.0f);
#endif
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

// Rendering callback
void RenderDrawLists(ImDrawData* draw_data) {
    ImGui::GetDrawData();
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    assert(io.Fonts->TexID != (ImTextureID)NULL); // You forgot to create and set font texture

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    // Backup GL state
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLint last_shade_model;
    glGetIntegerv(GL_SHADE_MODEL, &last_shade_model);
    GLint last_tex_env_mode;
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &last_tex_env_mode);

#ifdef GL_VERSION_ES_CL_1_1
    GLint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
#else
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif

    // Setup desired GL state
    SetupRenderState(draw_data, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos; // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are
                                                     // often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                        (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                          (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
                       (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
                // request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    SetupRenderState(draw_data, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f &&
                    clip_rect.w >= 0.0f) {
                    // Apply scissor/clipping rectangle
                    glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w),
                              (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

                    // Bind texture, Draw
                    GLuint textureHandle = convertImTextureIDToGLTextureHandle(pcmd->TextureId);
                    glBindTexture(GL_TEXTURE_2D, textureHandle);
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                   idx_buffer);
                }
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]);
    glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2],
               (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2],
              (GLsizei)last_scissor_box[3]);
    glShadeModel(last_shade_model);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, last_tex_env_mode);

#ifdef GL_VERSION_ES_CL_1_1
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glDisable(GL_SCISSOR_TEST);
#endif
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
    bool isPressed = sf::Joystick::isButtonPressed(s_joystickId, s_joystickMapping[action]);
    io.NavInputs[action] = isPressed ? 1.0f : 0.0f;
}

void updateJoystickDPadState(ImGuiIO& io) {
    float dpadXPos = sf::Joystick::getAxisPosition(s_joystickId, s_dPadInfo.xAxis);
    if (s_dPadInfo.xInverted) dpadXPos = -dpadXPos;

    float dpadYPos = sf::Joystick::getAxisPosition(s_joystickId, s_dPadInfo.yAxis);
    if (s_dPadInfo.yInverted) dpadYPos = -dpadYPos;

    io.NavInputs[ImGuiNavInput_DpadLeft] = dpadXPos < -s_dPadInfo.threshold ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadRight] = dpadXPos > s_dPadInfo.threshold ? 1.0f : 0.0f;

    io.NavInputs[ImGuiNavInput_DpadUp] = dpadYPos < -s_dPadInfo.threshold ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown] = dpadYPos > s_dPadInfo.threshold ? 1.0f : 0.0f;
}

void updateJoystickLStickState(ImGuiIO& io) {
    float lStickXPos = sf::Joystick::getAxisPosition(s_joystickId, s_lStickInfo.xAxis);
    if (s_lStickInfo.xInverted) lStickXPos = -lStickXPos;

    float lStickYPos = sf::Joystick::getAxisPosition(s_joystickId, s_lStickInfo.yAxis);
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

const char* getClipboardText(void* /*userData*/) {
    std::basic_string<sf::Uint8> tmp = sf::Clipboard::getString().toUtf8();
    s_clipboardText = std::string(tmp.begin(), tmp.end());
    return s_clipboardText.c_str();
}

void loadMouseCursor(ImGuiMouseCursor imguiCursorType, sf::Cursor::Type sfmlCursorType) {
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

            sf::Cursor& c = s_mouseCursorLoaded[cursor] ? *s_mouseCursors[cursor] :
                                                          *s_mouseCursors[ImGuiMouseCursor_Arrow];
            window.setMouseCursor(c);
        }
    }
}

} // end of anonymous namespace
