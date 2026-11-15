#include "PageManager.hpp"

bool PageManager::add(Page *p)
{
    if (!p || count >= MAX_PAGES)
        return false;

    p->userdata = this;
    p->on_swipe = &PageManager::swipe_thunk;
    pages[count++] = p;
    return true;
}

void PageManager::begin()
{
    if (count > 0)
        show(0);
}

void PageManager::update(const RawTouch &now)
{
    if (count == 0)
        return;

    pages[cur]->update(now);
    apply_pending(); // page switches happen here, never in the middle of a touch
}

bool PageManager::draw()
{
    return (count > 0) ? pages[cur]->draw() : false;
}

void PageManager::go_to(int index) { pending = index; }

void PageManager::next()
{
    int n = cur + 1;
    if (n >= count)
        n = wrap ? 0 : cur;
    pending = n;
}

void PageManager::prev()
{
    int n = cur - 1;
    if (n < 0)
        n = wrap ? count - 1 : cur;
    pending = n;
}

// finger moved LEFT = bring the next page in from the right (like a phone)
void PageManager::swipe_thunk(Page *p, SwipeDir dir)
{
    PageManager *m = static_cast<PageManager *>(p->userdata);
    if (dir == SwipeDir::LEFT)
        m->next();
    else if (dir == SwipeDir::RIGHT)
        m->prev();
    // UP / DOWN: not a page change
}

void PageManager::apply_pending()
{
    if (pending < 0)
        return;

    int target = pending;
    pending = -1;

    if (target < 0 || target >= count || target == cur)
        return;

    if (pages[cur]->on_leave)
        pages[cur]->on_leave(pages[cur]);

    show(target);
}

// clear the screen, make the new page repaint everything, tell everyone
void PageManager::show(int index)
{
    cur = index;
    lcd_clear(bg_color);
    pages[cur]->invalidate();

    if (pages[cur]->on_enter)
        pages[cur]->on_enter(pages[cur]);
    if (on_page_changed)
        on_page_changed(cur);
}