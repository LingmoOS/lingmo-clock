// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockwindow.h"

#include "views/clockpage.h"
#include "views/stopwatchpage.h"
#include "views/timerpage.h"
#include "views/worldclockpage.h"

#include "base/i18n.h"
#include "ui/application.h"
#include "ui/color_id.h"
#include "ui/color_provider.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/segmented_control.h"
#include "ui/theme.h"
#include "ui/toast.h"
#include "ui/window_header.h"

#include <algorithm>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace Clock {

namespace {

constexpr double kHeaderHeight = 44;
constexpr double kMargin = 16;

} // namespace

// The window's title, before the window buttons.
class ClockWindow::HeaderContent : public Lingmo::ui::View
{
public:
    HeaderContent()
        : title(addChild<Label>(tr("Clock")))
    {
        setName("headerContent");
        title->setName("windowTitle");
        title->setFontSize(14.5);
        title->setWeight(600);
        title->setColor(ColorRecipe(ColorId::Text));
        title->setElide(Label::Elide::Right);
    }

    Label* title;

protected:
    void layout() override
    {
        const SizeF size = title->sizeHint();
        title->setGeometry({20, (geometry().height - size.height) / 2,
                            std::max(0.0, geometry().width - 20 - 120), size.height});
    }
};

// The header, the pages' switch and the pages.
class ClockWindow::Root : public Lingmo::ui::View
{
public:
    Root()
        : header(addChild<WindowHeader>(kHeaderHeight))
        , switchControl(addChild<SegmentedControl>(
              std::vector<std::string>{tr("Clock"), tr("World Clock"), tr("Stopwatch"), tr("Timer")},
              SegmentedControl::Style::Segmented))
        , clock(addChild<ClockPage>())
        , world(addChild<WorldClockPage>())
        , stopwatch(addChild<StopwatchPage>())
        , timer(addChild<TimerPage>())
    {
        setName("clockRoot");
        header->setName("header");
        headerContent = header->setContent<HeaderContent>();
        header->setButtonMargin(4);
        header->setButtonsCentered(true);

        switchControl->setName("pageSwitch");
        switchControl->setContentHeight(34);
        switchControl->setFontSize(13.5);
    }

    std::vector<Lingmo::ui::View*> pages() const
    {
        return {clock, world, stopwatch, timer};
    }

    WindowHeader* header;
    HeaderContent* headerContent;
    SegmentedControl* switchControl;
    ClockPage* clock;
    WorldClockPage* world;
    StopwatchPage* stopwatch;
    TimerPage* timer;

protected:
    void layout() override
    {
        const RectF rect = localRect();
        header->setGeometry({0, 0, rect.width, kHeaderHeight});

        const SizeF switchHint = switchControl->sizeHint();
        const double switchWidth = std::min(std::max(switchHint.width, 420.0), std::max(240.0, rect.width - 96));
        const double switchHeight = switchHint.height;
        const double switchY = kHeaderHeight + 12;
        switchControl->setGeometry(
            {std::max(kMargin, (rect.width - switchWidth) / 2), switchY, switchWidth, switchHeight});

        const double pagesTop = switchY + switchHeight + 10;
        const RectF pagesRect{kMargin, pagesTop, std::max(0.0, rect.width - 2 * kMargin),
                              std::max(0.0, rect.height - pagesTop - 10)};
        for (View* page : pages())
            page->setGeometry(pagesRect);
    }

    void paint(Painter& painter) override
    {
        // A hairline under the header, as the toolkit's windows have.
        const Theme& theme = Theme::current();
        painter.drawHorizontalLine(0, kHeaderHeight - 1, geometry().width, theme.text.withAlpha(0.06),
                                   painter.borderWidth(1));
    }
};

ClockWindow::ClockWindow()
{
    m_window.setTitle(tr("Clock"));
    m_window.resize({900, 680});
    m_window.setMinimumSize({720, 560});

    auto root = std::make_unique<Root>();
    m_root = root.get();
    m_window.setContentView(std::move(root));

    m_root->switchControl->onCurrentChanged = [this](int index) { selectPage(index); };
    m_root->timer->setOnFinished([this] {
        showNotification(tr("Time's up!"));
        m_window.activate();
    });

    m_toast = std::make_unique<Toast>(&m_window);
    bindShortcuts();

    m_languageChanged =
        Application::instance()->languageChanged.connect([this] { retranslate(); });
    selectPage(PageClock);
}

ClockWindow::~ClockWindow() = default;

void ClockWindow::show()
{
    m_window.show();
    updateActivePage();
}

SegmentedControl* ClockWindow::pageSwitch() const
{
    return m_root->switchControl;
}

ClockPage* ClockWindow::clockPage() const
{
    return m_root->clock;
}

WorldClockPage* ClockWindow::worldClockPage() const
{
    return m_root->world;
}

StopwatchPage* ClockWindow::stopwatchPage() const
{
    return m_root->stopwatch;
}

TimerPage* ClockWindow::timerPage() const
{
    return m_root->timer;
}

void ClockWindow::selectPage(int index)
{
    const int clamped = std::clamp(index, 0, PageCount - 1);
    m_currentPage = clamped;
    m_root->switchControl->setCurrentIndex(clamped);
    const std::vector<View*> pages = m_root->pages();
    for (size_t i = 0; i < pages.size(); ++i)
        pages[i]->setVisible(int(i) == clamped);
    updateActivePage();
    if (m_root->headerContent)
        m_root->headerContent->requestLayout();
    m_root->requestLayout();
}

void ClockWindow::updateActivePage()
{
    m_root->clock->setActive(m_currentPage == PageClock);
    m_root->world->setActive(m_currentPage == PageWorldClock);
    m_root->stopwatch->setActive(m_currentPage == PageStopwatch);
    m_root->timer->setActive(m_currentPage == PageTimer);
}

void ClockWindow::showNotification(const std::string& text)
{
    if (m_toast)
        m_toast->show(text);
}

void ClockWindow::retranslate()
{
    m_window.setTitle(tr("Clock"));
    m_root->headerContent->title->setText(tr("Clock"));
    m_root->switchControl->setItems(
        {tr("Clock"), tr("World Clock"), tr("Stopwatch"), tr("Timer")});
    m_root->switchControl->setCurrentIndex(m_currentPage);
    m_root->clock->retranslate();
    m_root->world->retranslate();
    m_root->stopwatch->retranslate();
    m_root->timer->retranslate();
    m_root->requestLayout();
    m_root->requestPaint();
}

void ClockWindow::bindShortcuts()
{
    const auto bind = [this](uint32_t modifiers, uint32_t key, std::function<void()> action) {
        m_window.addShortcut(modifiers, key, std::move(action));
    };
    const auto letter = [](char character) { return key::letter(character); };

    // Ctrl+1 to Ctrl+4 show a page; Ctrl+Tab cycles them.
    for (int i = 0; i < PageCount; ++i) {
        bind(ControlModifier, letter(char('1' + i)), [this, i] { selectPage(i); });
    }
    bind(ControlModifier, key::Tab,
         [this] { selectPage((m_currentPage + 1) % PageCount); });
    bind(ControlModifier | ShiftModifier, key::Tab,
         [this] { selectPage((m_currentPage + PageCount - 1) % PageCount); });

    // The transport keys, on the page they belong to.
    bind(NoModifier, letter('s'), [this] {
        if (m_currentPage == PageStopwatch)
            m_root->stopwatch->toggle();
        else if (m_currentPage == PageTimer)
            m_root->timer->toggle();
    });
    bind(NoModifier, letter('l'), [this] {
        if (m_currentPage == PageStopwatch)
            m_root->stopwatch->addLap();
    });
    bind(NoModifier, letter('r'), [this] {
        if (m_currentPage == PageStopwatch)
            m_root->stopwatch->reset();
        else if (m_currentPage == PageTimer)
            m_root->timer->reset();
    });
    bind(NoModifier, letter('t'), [this] {
        if (m_currentPage == PageClock)
            m_root->clock->toggleTwentyFourHourClock();
    });

    bind(ControlModifier, letter('q'), [] { Application::instance()->quit(); });
}

} // namespace Clock
