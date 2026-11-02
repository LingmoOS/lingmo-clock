// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockwindow.h"

#include "base/i18n.h"
#include "ui/application.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifndef CLOCK_VERSION
#define CLOCK_VERSION "1.0"
#endif

static void printHelp(const char* program)
{
    std::printf("Usage: %s [options]\nLingmo Clock\n\nOptions:\n"
                "  -h, --help     Displays help on commandline options.\n"
                "  -v, --version  Displays version information.\n",
                program);
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "-h" || argument == "--help" || argument == "-?") {
            printHelp(argv[0]);
            return 0;
        }
        if (argument == "-v" || argument == "--version") {
            std::printf("lingmo-clock %s\n", CLOCK_VERSION);
            return 0;
        }
        std::fprintf(stderr, "Unknown argument '%s'.\n", argument.c_str());
        printHelp(argv[0]);
        return 1;
    }

    Lingmo::ui::Application app(argc, argv);
    app.setApplicationId("lingmo-clock");
    Lingmo::i18n::setDomain("lingmo-clock");

    Clock::ClockWindow window;
    window.show();
    return app.run();
}
