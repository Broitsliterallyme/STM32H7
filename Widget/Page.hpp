#pragma once
#include "Widget.hpp"

// ============================================================================
//  Page.hpp
//
//  Part 1: GestureTracker - the ONE place that decides what a touch means.
//  Part 2: Page           - owns widgets, gives each touch to exactly one owner.
//
//      tp_scan() -> RawTouch -> GestureTracker -> Page -> Widget / page hooks
// ============================================================================

// ---- tuning: all thresholds in one place -----------------------------------
namespace GestureCfg
{
    constexpr int16_t TAP_RADIUS = 8;     // press and release this close = tap
    constexpr int16_t SWIPE_MIN = 60;     // must travel this far to be a swipe
    constexpr int16_t ZOOM_MIN_DIST = 20; // ignore pinch if fingers start closer
} // namespace GestureCfg

// What the touch driver gives us each scan (copy from tp_dev).
struct RawTouch
{
    Point p[2]; // first two fingers
    uint8_t n;  // fingers currently down (0 = none)
};

enum class EventType : uint8_t { NONE, PRESS, MOVE, RELEASE };

// What a whole press->release turned out to be (valid on RELEASE only).
enum class Result : uint8_t { NONE, TAP, SWIPE, ZOOM };

enum class SwipeDir : uint8_t { NONE, LEFT, RIGHT, UP, DOWN };

struct GestureEvent
{
    EventType type = EventType::NONE;
    Result result = Result::NONE;
    SwipeDir dir = SwipeDir::NONE;

    Point start;     // where the finger FIRST went down
    Point pos;       // current position (on RELEASE: where it lifted)
    Point delta;     // movement since the previous event
    float scale = 1; // pinch scale: finger distance now / when the pinch started
    bool zooming = false; // true while two fingers are pinching
    Point center;    // midpoint between the two fingers (valid while zooming)
};

class GestureTracker
{
public:
    // Call once per touch scan. Returns at most one event.
    GestureEvent process(const RawTouch &now);

private:
    enum class State : uint8_t { IDLE, HELD, ZOOMING };

    State state = State::IDLE;
    Point start, last;
    float zoom_start_dist = 0;
    float zoom_scale = 1;
    float last_scale = 1; // last pinch values sent, so a still pinch sends nothing
    Point last_center;

    GestureEvent on_down(const RawTouch &now);
    GestureEvent on_held(const RawTouch &now);
    GestureEvent on_up();
};

// ---------------------------------------------------------------------------
class Page
{
public:
    static constexpr int MAX_WIDGETS = 24;

    bool add(Widget *w);

    // Call every loop with the latest touch scan. Does everything input-related.
    void update(const RawTouch &now);

    // Call every loop after update(). Redraws only widgets marked dirty.
    // Returns true if anything was drawn (so you know to lcd_flush()).
    bool draw();

    // Mark everything dirty (use when switching to this page).
    void invalidate();

    // Hooks for touches that belong to the PAGE (not to any widget).
    void (*on_swipe)(Page *, SwipeDir) = nullptr;
    void (*on_zoom)(Page *, float scale) = nullptr; // live pinch on empty space (no zoomable widget under it)
    void (*on_tap_empty)(Page *, Point) = nullptr;

    // Called by PageManager when this page becomes / stops being the visible one.
    void (*on_enter)(Page *) = nullptr;
    void (*on_leave)(Page *) = nullptr;

    void *userdata = nullptr; // free for your own use

private:
    Widget *widgets[MAX_WIDGETS] = {};
    int count = 0;

    GestureTracker tracker;

    // The widget that owns the touch in progress (nullptr = the page).
    // Chosen on PRESS, kept until RELEASE. This is what guarantees a slider
    // drag can never be read as a page swipe.
    Widget *owner = nullptr;

    // Two-finger pinch in progress. zoom_target is the zoomable widget under the
    // pinch centre (nullptr = nothing there, so the page on_zoom hook gets it).
    bool pinching = false;
    Widget *zoom_target = nullptr;

    void handle(const GestureEvent &e);
    Widget *hit_test(Point p) const;
    void on_press(const GestureEvent &e);
    void on_move(const GestureEvent &e);
    void on_pinch(const GestureEvent &e);
    Widget *zoom_hit_test(Point p) const;
    void on_release(const GestureEvent &e);
};