// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockwindow.h"

#include "views/alarmpage.h"
#include "views/clockpage.h"
#include "views/stopwatchpage.h"
#include "views/timerpage.h"
#include "views/worldclockpage.h"

#include "base/i18n.h"
#include "ui/application.h"
#include "ui/color_id.h"
#include "ui/color_provider.h"
#include "ui/icon.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/theme.h"
#include "ui/toast.h"
#include "ui/window_header.h"

#include <algorithm>
#include <functional>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace Clock {

namespace {

constexpr double kHeaderHeight = 44;
constexpr double kSidebarWidth = 200;
constexpr double kNavItemHeight = 48;
constexpr double kMargin = 16;

} // namespace

// A single navigation item in the sidebar.
class NavigationItem : public View
{
public:
    NavigationItem(const std::string& iconName, const std::string& text)
        : m_icon(addChild<ThemeIcon>(iconName, 20))
        , m_text(addChild<Label>(text))
    {
        setName("navigationItem");
        m_text->setFontSize(13);
        m_text->setWeight(500);
    }

    void setSelected(bool selected)
    {
        if (m_selected == selected) return;
        m_selected = selected;
        invalidate();
    }

    std::function<void()> onClicked;

protected:
    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const RectF rect = localRect();

        if (m_selected) {
            painter.fillRect(rect, theme.surfaceVariant);
            painter.fillRect({0, 0, 3, rect.height}, theme.accent);
        } else if (m_hovered) {
            painter.fillRect(rect, theme.surfaceVariant.withAlpha(0.5));
        }
    }

    void layout() override
    {
        const RectF rect = localRect();
        const double iconSize = 20;
        const double leftPadding = 16;
        const double iconTextGap = 12;

        m_icon->setGeometry({leftPadding, (rect.height - iconSize) / 2, iconSize, iconSize});

        const double textX = leftPadding + iconSize + iconTextGap;
        const SizeF textSize = m_text->sizeHint();
        m_text->setGeometry({textX, (rect.height - textSize.height) / 2,
                             std::max(0.0, rect.width - textX - 16), textSize.height});
    }

    bool pointerPress(const PointerEvent& event) override
    {
        if (event.button == PointerButton::Primary) {
            m_pressed = true;
            return true;
        }
        return false;
    }

    void pointerRelease(const PointerEvent& event) override
    {
        if (m_pressed && event.button == PointerButton::Primary) {
            m_pressed = false;
            if (localRect().contains(event.position) && onClicked) {
                onClicked();
            }
        }
    }

    void pointerEnter() override
    {
        m_hovered = true;
        invalidate();
        setCursor(CursorShape::Pointer);
    }

    void pointerLeave() override
    {
        m_hovered = false;
        m_pressed = false;
        invalidate();
        setCursor(CursorShape::Default);
    }

private:
    ThemeIcon* m_icon = nullptr;
    Label* m_text = nullptr;
    bool m_selected = false;
    bool m_hovered = false;
    bool m_pressed = false;
};

// The sidebar containing all navigation items.
class Sidebar : public View
{
public:
    Sidebar()
    {
        setName("sidebar");
    }

    NavigationItem* addItem(const std::string& iconName, const std::string& text)
    {
        auto item = addChild<NavigationItem>(iconName, text);
        m_items.push_back(item);
        return item;
    }

    void setCurrentIndex(int index)
    {
        if (index < 0 || index >= int(m_items.size())) return;
        if (m_currentIndex == index) return;

        if (m_currentIndex >= 0 && m_currentIndex < int(m_items.size())) {
            m_items[m_currentIndex]->setSelected(false);
        }

        m_currentIndex = index;
        m_items[index]->setSelected(true);
    }

    int currentIndex() const { return m_currentIndex; }

    std::function<void(int)> onItemSelected;

protected:
    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const RectF rect = localRect();
        painter.fillRect(rect, theme.surface);
        painter.drawVerticalLine(rect.width - 1, 0, rect.height,
                                 theme.text.withAlpha(0.06), painter.borderWidth(1));
    }

    void layout() override
    {
        const RectF rect = localRect();
        double y = 12;

        for (auto item : m_items) {
            item->setGeometry({0, y, rect.width, kNavItemHeight});
            y += kNavItemHeight;
        }
    }

private:
    std::vector<NavigationItem*> m_items;
    int m_currentIndex = -1;
};

// The window's title, before the window buttons.
class ClockWindow::HeaderContent : public View
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

// The header, the sidebar and the pages.
class ClockWindow::Root : public View
{
public:
    Root()
        : sidebar(addChild<Sidebar>())
        , contentArea(addChild<View>())
        , header(contentArea->addChild<WindowHeader>(kHeaderHeight))
        , clock(contentArea->addChild<ClockPage>())
        , alarm(contentArea->addChild<AlarmPage>())
        , world(contentArea->addChild<WorldClockPage>())
        , stopwatch(contentArea->addChild<StopwatchPage>())
        , timer(contentArea->addChild<TimerPage>())
    {
        setName("clockRoot");
        sidebar->setName("sidebar");
        contentArea->setName("contentArea");
        header->setName("header");
        headerContent = header->setContent<HeaderContent>();
        header->setButtonMargin(4);
        header->setButtonsCentered(true);
    }

    std::vector<View*> pages() const
    {
        return {clock, alarm, world, stopwatch, timer};
    }

    Sidebar* sidebar;
    View* contentArea;
    WindowHeader* header;
    HeaderContent* headerContent;
    ClockPage* clock;
    AlarmPage* alarm;
    WorldClockPage* world;
    StopwatchPage* stopwatch;
    TimerPage* timer;

protected:
    void layout() override
    {
        const RectF rect = localRect();

        sidebar->setGeometry({0, 0, kSidebarWidth, rect.height});

        const RectF contentRect{kSidebarWidth, 0, std::max(0.0, rect.width - kSidebarWidth), rect.height};
        contentArea->setGeometry(contentRect);

        header->setGeometry({0, 0, contentRect.width, kHeaderHeight});

        const RectF pagesRect{kMargin, kHeaderHeight + kMargin,
                              std::max(0.0, contentRect.width - 2 * kMargin),
                              std::max(0.0, contentRect.height - kHeaderHeight - 2 * kMargin)};
        for (View* page : pages())
            page->setGeometry(pagesRect);
    }

    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        painter.fillRect(localRect(), theme.background);
    }
};

ClockWindow::ClockWindow()
{
    m_window.setTitle(tr("Clock"));
    m_window.resize({1100, 680});
    m_window.setMinimumSize({920, 560});

    auto root = std::make_unique<Root>();
    m_root = root.get();
    m_window.setContentView(std::move(root));

    // Add navigation items with system icons.
    m_root->sidebar->addItem("preferences-system-time", tr("Clock"));
    m_root->sidebar->addItem("alarm-clock", tr("Alarm"));
    m_root->sidebar->addItem("x-office-calendar", tr("World Clock"));
    m_root->sidebar->addItem("chronometer", tr("Stopwatch"));
    m_root->sidebar->addItem("appointment-new", tr("Timer"));

    // Wire sidebar selection.
    m_root->sidebar->onItemSelected = [this](int index) { selectPage(index); };

    // Wire navigation items click.
    for (int i = 0; i < 5; ++i) {
        auto item = static_cast<NavigationItem*>(m_root->sidebar->children()[i].get());
        item->onClicked = [this, i]() {
            m_root->sidebar->setCurrentIndex(i);
            if (m_root->sidebar->onItemSelected) {
                m_root->sidebar->onItemSelected(i);
            }
        };
    }

    m_root->timer->setOnFinished([this] {
        showNotification(tr("Time's up!"));
        m_window.activate();
    });
    m_root->alarm->setOnAlarm([this](const std::string& time) {
        showNotification(tr("Alarm ") + time);
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
    return nullptr; // No longer using SegmentedControl
}

ClockPage* ClockWindow::clockPage() const
{
    return m_root->clock;
}

AlarmPage* ClockWindow::alarmPage() const
{
    return m_root->alarm;
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

void ClockWindow::showNotification(const std::string& text)
{
    m_toast->show(text, Toast::Duration::Short);
}

void ClockWindow::selectPage(int index)
{
    if (index < 0 || index >= PageCount) return;
    m_currentPage = index;
    m_root->sidebar->setCurrentIndex(index);
    updateActivePage();
}

void ClockWindow::updateActivePage()
{
    const auto pages = m_root->pages();
    for (int i = 0; i < int(pages.size()); ++i)
        pages[i]->setVisible(i == m_currentPage);
}

void ClockWindow::bindShortcuts()
{
    m_window.addShortcut(Keys::Control | Keys::Key_1, [this] { selectPage(PageClock); });
    m_window.addShortcut(Keys::Control | Keys::Key_2, [this] { selectPage(PageAlarm); });
    m_window.addShortcut(Keys::Control | Keys::Key_3, [this] { selectPage(PageWorldClock); });
    m_window.addShortcut(Keys::Control | Keys::Key_4, [this] { selectPage(PageStopwatch); });
    m_window.addShortcut(Keys::Control | Keys::Key_5, [this] { selectPage(PageTimer); });
}

void ClockWindow::retranslate()
{
    m_window.setTitle(tr("Clock"));
    m_root->headerContent->title->setText(tr("Clock"));

    // Update sidebar item labels
    auto items = m_root->sidebar->children();
    if (items.size() >= 5) {
        static_cast<NavigationItem*>(items[0].get())->children()[1]->takeChild(0);
        static_cast<NavigationItem*>(items[0].get())->addChild<Label>(tr("Clock"));
        static_cast<NavigationItem*>(items[1].get())->children()[1]->takeChild(0);
        static_cast<NavigationItem*>(items[1].get())->addChild<Label>(tr("Alarm"));
        static_cast<NavigationItem*>(items[2].get())->children()[1]->takeChild(0);
        static_cast<NavigationItem*>(items[2].get())->addChild<Label>(tr("World Clock"));
        static_cast<NavigationItem*>(items[3].get())->children()[1]->takeChild(0);
        static_cast<NavigationItem*>(items[3].get())->addChild<Label>(tr("Stopwatch"));
        static_cast<NavigationItem*>(items[4].get())->children()[1]->takeChild(0);
        static_cast<NavigationItem*>(items[4].get())->addChild<Label>(tr("Timer"));
    }
}

} // namespace Clock
