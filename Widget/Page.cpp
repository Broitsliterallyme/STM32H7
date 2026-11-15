#include "Page.hpp"
#include <math.h>

// ============================================================================
//  Part 1: GestureTracker
// ============================================================================

static int16_t iabs(int16_t v) { return v < 0 ? -v : v; }

static float dist(Point a, Point b)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

// THE function: one raw scan in -> at most one event out.
GestureEvent GestureTracker::process(const RawTouch &now)
{
    if (state == State::IDLE)
        return (now.n > 0) ? on_down(now) : GestureEvent{};

    return (now.n > 0) ? on_held(now) : on_up();
}

GestureEvent GestureTracker::on_down(const RawTouch &now)
{
    state = State::HELD;
    start = last = now.p[0];
    zoom_scale = 1;

    GestureEvent e;
    e.type = EventType::PRESS;
    e.start = e.pos = start;
    return e;
}

GestureEvent GestureTracker::on_held(const RawTouch &now)
{
    // second finger arrives -> pinch mode
    if (now.n >= 2 && state == State::HELD)
    {
        zoom_start_dist = dist(now.p[0], now.p[1]);
        if (zoom_start_dist >= GestureCfg::ZOOM_MIN_DIST)
            state = State::ZOOMING;
    }

    if (state == State::ZOOMING && now.n >= 2)
        zoom_scale = dist(now.p[0], now.p[1]) / zoom_start_dist;

    Point cur = now.p[0];
    if (cur.x == last.x && cur.y == last.y && zoom_scale == 1)
        return GestureEvent{}; // nothing changed

    GestureEvent e;
    e.type = EventType::MOVE;
    e.start = start;
    e.pos = cur;
    e.delta = {(int16_t)(cur.x - last.x), (int16_t)(cur.y - last.y)};
    e.scale = zoom_scale;
    last = cur;
    return e;
}

// Finger lifted. The touch chip reports no coordinates on release, so the
// release point is the last position we saw. Classified by start vs end.
GestureEvent GestureTracker::on_up()
{
    GestureEvent e;
    e.type = EventType::RELEASE;
    e.start = start;
    e.pos = last;
    e.scale = zoom_scale;

    int16_t dx = last.x - start.x;
    int16_t dy = last.y - start.y;
    int16_t ax = iabs(dx), ay = iabs(dy);

    if (state == State::ZOOMING)
    {
        e.result = Result::ZOOM;
    }
    else if (ax <= GestureCfg::TAP_RADIUS && ay <= GestureCfg::TAP_RADIUS)
    {
        e.result = Result::TAP;
    }
    else if (ax >= GestureCfg::SWIPE_MIN || ay >= GestureCfg::SWIPE_MIN)
    {
        e.result = Result::SWIPE;
        if (ax >= ay)
            e.dir = (dx > 0) ? SwipeDir::RIGHT : SwipeDir::LEFT;
        else
            e.dir = (dy > 0) ? SwipeDir::DOWN : SwipeDir::UP;
    }
    // else: Result::NONE (in-between distance: ignored on purpose)

    state = State::IDLE;
    return e;
}

// ============================================================================
//  Part 2: Page
// ============================================================================

bool Page::add(Widget *w)
{
    if (!w || count >= MAX_WIDGETS)
        return false;
    widgets[count++] = w;
    return true;
}

void Page::update(const RawTouch &now)
{
    handle(tracker.process(now));

    for (int i = 0; i < count; i++)
        widgets[i]->tick();
}

bool Page::draw()
{
    bool drew = false;
    for (int i = 0; i < count; i++)
        drew |= widgets[i]->render();
    return drew;
}

void Page::invalidate()
{
    for (int i = 0; i < count; i++)
        widgets[i]->mark_dirty();
}

// Topmost touchable widget under the finger (last added = on top).
Widget *Page::hit_test(Point p) const
{
    for (int i = count - 1; i >= 0; i--)
    {
        Widget *w = widgets[i];
        if (w->policy != TouchPolicy::NONE && w->is_visible() && w->contains(p.x, p.y))
            return w;
    }
    return nullptr;
}

void Page::handle(const GestureEvent &e)
{
    switch (e.type)
    {
    case EventType::PRESS:   on_press(e);   break;
    case EventType::MOVE:    on_move(e);    break;
    case EventType::RELEASE: on_release(e); break;
    case EventType::NONE:                   break;
    }
}

// PRESS: pick the owner for the whole touch.
void Page::on_press(const GestureEvent &e)
{
    owner = hit_test(e.start);
    if (owner)
        owner->touch_down(e.pos);
}

// MOVE: a CAPTURE widget (slider) gets every movement immediately = live.
void Page::on_move(const GestureEvent &e)
{
    if (!owner)
        return; // page-owned touch: classified on release

    if (owner->policy == TouchPolicy::CAPTURE)
    {
        owner->touch_move(e.pos, e.delta);
        return;
    }

    // TAP_ONLY widget: finger left the tap radius -> widget lets go and the
    // page takes over (so a page swipe can start on top of a button).
    if (iabs(e.pos.x - e.start.x) > GestureCfg::TAP_RADIUS ||
        iabs(e.pos.y - e.start.y) > GestureCfg::TAP_RADIUS)
    {
        owner->touch_cancel();
        owner = nullptr;
    }
}

void Page::on_release(const GestureEvent &e)
{
    if (owner)
    {
        // Widget-owned touch: the page hooks are NOT called, whatever the motion.
        if (owner->policy == TouchPolicy::CAPTURE)
            owner->touch_up(e.pos);
        else if (e.result == Result::TAP && owner->contains(e.pos.x, e.pos.y))
            owner->touch_tap();
        else
            owner->touch_cancel();

        owner = nullptr;
        return;
    }

    // Page-owned touch
    switch (e.result)
    {
    case Result::TAP:   if (on_tap_empty) on_tap_empty(this, e.pos); break;
    case Result::SWIPE: if (on_swipe)     on_swipe(this, e.dir);     break;
    case Result::ZOOM:  if (on_zoom)      on_zoom(this, e.scale);    break;
    case Result::NONE:                                         break;
    }
}