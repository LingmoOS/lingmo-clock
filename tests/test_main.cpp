// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base/event_loop.h"

#ifdef CLOCK_UI_TESTS
#include "ui/application.h"
#include "ui/theme.h"
#endif

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
#ifdef CLOCK_UI_TESTS
    // The views need the toolkit's process-wide state: fonts, the loop and a
    // theme. The model tests run with a bare loop.
    Lingmo::ui::Application application(argc, argv, Lingmo::ui::Application::Platform::Headless);
    application.setApplicationId("lingmo-clock-tests");
    Lingmo::ui::Theme::setCurrent(Lingmo::ui::Theme::light());
#else
    Lingmo::base::EventLoop loop;
#endif
    return RUN_ALL_TESTS();
}
