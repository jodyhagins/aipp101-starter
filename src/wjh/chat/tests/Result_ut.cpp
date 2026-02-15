// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/Result.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;

TEST_SUITE("Result")
{
    TEST_CASE("Result with value")
    {
        Result<int> r(42);

        CHECK(r.has_value());
        CHECK(static_cast<bool>(r));
        CHECK(r.value() == 42);
        CHECK(*r == 42);
    }

    TEST_CASE("Result with error")
    {
        Result<int> r = make_error("something went wrong");

        CHECK_FALSE(r.has_value());
        CHECK_FALSE(static_cast<bool>(r));
        CHECK(r.error() == "something went wrong");
    }

    TEST_CASE("Result<void> success")
    {
        Result<void> r;

        CHECK(r.has_value());
        CHECK(static_cast<bool>(r));
    }

    TEST_CASE("Result<void> error")
    {
        Result<void> r = make_error("error occurred");

        CHECK_FALSE(r.has_value());
        CHECK_FALSE(static_cast<bool>(r));
        CHECK(r.error() == "error occurred");
    }

    TEST_CASE("make_error with format")
    {
        Result<int> r = make_error("Error code: {}", 42);

        CHECK_FALSE(r.has_value());
        CHECK(r.error() == "Error code: 42");
    }
}

} // anonymous namespace
