// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockwindow.h"

#include "views/clockpage.h"
#include "views/stopwatchpage.h"
#include "views/timerpage.h"
#include "views/worldclockpage.h"

#include "base/event_loop.h"
#include "ui/application.h"
#include "ui/keys.h"
#include "ui/test_input.h"
#include "ui/theme.h"
#include "ui/window.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <poll.h>
#include <string>

using namespace Lingmo;
using namespace Lingmo::ui;
using namespace std::chrono_literals;

namespace {

void spin(int milliseconds)
{
    base::EventLoop& loop = Application::instance()->loop();
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() < end) {
        pollfd descriptor{loop.fd(), POLLIN, 0};
        ::poll(&descriptor, 1, 10);
        loop.dispatch();
    }
}

void capture(Window& window, const std::string& path)
{
    spin(60);
    window.grab().savePng(path);
    std::printf("%s\n", path.c_str());
}

} // namespace

// Renders the app headless: clock-capture OUTDIR SCALE.
int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s OUTDIR SCALE\n", argv[0]);
        return 1;
    }
    const std::string directory = argv[1];
    const double scale = std::atof(argv[2]);
    std::filesystem::create_directories(directory);

    Application app(argc, argv, Application::Platform::Headless);
    app.setApplicationId("lingmo-clock");

    auto render = [&](bool dark) {
        Theme::setCurrent(dark ? Theme::dark() : Theme::light());
        Clock::ClockWindow window;
        window.window().setScale(scale);
        window.window().resize({900, 680});
        window.show();
        spin(250);

        const std::string suffix = dark ? "dark" : "light";
        capture(window.window(), directory + "/clock-" + suffix + ".png");

        if (dark) {
            window.selectPage(Clock::ClockWindow::PageTimer);
            window.timerPage()->setMinutesSeconds(10, 0);
            window.timerPage()->start();
            spin(3000);
            capture(window.window(), directory + "/timer-dark.png");
            return;
        }

        window.selectPage(Clock::ClockWindow::PageWorldClock);
        window.worldClockPage()->setActive(true);
        spin(200);
        capture(window.window(), directory + "/world-clock-light.png");

        window.selectPage(Clock::ClockWindow::PageStopwatch);
        window.stopwatchPage()->start();
        spin(1250);
        window.stopwatchPage()->addLap();
        spin(700);
        window.stopwatchPage()->addLap();
        spin(430);
        window.stopwatchPage()->addLap();
        window.stopwatchPage()->pause();
        capture(window.window(), directory + "/stopwatch-light.png");

        window.selectPage(Clock::ClockWindow::PageTimer);
        window.timerPage()->setMinutesSeconds(10, 0);
        window.timerPage()->start();
        spin(2600);
        capture(window.window(), directory + "/timer-light.png");
    };

    render(false);
    render(true);
    return 0;
}
