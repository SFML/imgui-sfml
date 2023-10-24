#include <imgui.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("IM_VEC2_CLASS_EXTRA") {
    SECTION("From sf::Vector2f") {
        const auto imvec = ImVec2(sf::Vector2f(1.2f, 3.4f));
        CHECK(imvec.x == 1.2f);
        CHECK(imvec.y == 3.4f);
    }

    SECTION("From sf::Vector2i") {
        const auto imvec = ImVec2(sf::Vector2i(1, 2));
        CHECK(imvec.x == 1);
        CHECK(imvec.y == 2);
    }

    SECTION("To sf::Vector2f") {
        CHECK(sf::Vector2i(ImVec2(1.1f, 2.2f)) == sf::Vector2i(1, 2));
        CHECK(sf::Vector2f(ImVec2(1.1f, 2.2f)) == sf::Vector2f(1.1f, 2.2f));
    }
}

TEST_CASE("IM_VEC4_CLASS_EXTRA") {
    SECTION("From sf::Color") {
        const auto imvec = ImVec4(sf::Color(12, 34, 56, 78));
        CHECK(imvec.x == 12.f / 255);
        CHECK(imvec.y == 34.f / 255);
        CHECK(imvec.z == 56.f / 255);
        CHECK(imvec.w == 78.f / 255);
    }

    SECTION("To sf::Color") {
        const sf::Color color = ImVec4(0, .25f, .5f, .75f);
        CHECK(+color.r == 0);
        CHECK(+color.g == 63);
        CHECK(+color.b == 127);
        CHECK(+color.a == 191);
    }
}
