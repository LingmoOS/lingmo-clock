# SPDX-FileCopyrightText: 2026 LingmoOS Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(clock-model-tests
    test_main.cpp
    model_test.cpp
)
target_link_libraries(clock-model-tests PRIVATE clock-core GTest::gtest)
gtest_discover_tests(clock-model-tests)

add_executable(clock-ui-tests
    test_main.cpp
    ui_test.cpp
)
target_compile_definitions(clock-ui-tests PRIVATE CLOCK_UI_TESTS)
target_link_libraries(clock-ui-tests PRIVATE clock-ui GTest::gtest)
gtest_discover_tests(clock-ui-tests)

# Renders the app headless into PNGs for the reference screenshots:
#   clock-capture OUTDIR SCALE
add_executable(clock-capture capture.cpp)
target_link_libraries(clock-capture PRIVATE clock-ui)
