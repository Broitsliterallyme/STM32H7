#pragma once
#include "Page.hpp"

// ============================================================================
//  PageManager.hpp
//
//  Holds the pages you swipe through and shows exactly one at a time.
//    swipe LEFT  -> next page
//    swipe RIGHT -> previous page
//  Swipes that START on a CAPTURE widget (slider, log) never reach here.
//
//  NOTE: PageManager takes over each added page's on_swipe hook.
//        on_zoom / on_tap_empty / on_enter / on_leave stay yours to use.
// ============================================================================
class PageManager
{
public:
    static constexpr int MAX_PAGES = 8;

    bool add(Page *p);

    void begin();                      // show page 0 (call once after adding pages)
    void update(const RawTouch &now);  // call every loop
    bool draw();                       // call every loop, after update()

    void go_to(int index);             // jump to a page (applied safely, see below)
    void next();
    void prev();

    int current() const { return cur; }
    int page_count() const { return count; }

    bool wrap = false;                 // true: last page -> first, and back
    uint32_t bg_color = WHITE;         // screen is cleared to this on page change

    void (*on_page_changed)(int index) = nullptr;

private:
    Page *pages[MAX_PAGES] = {};
    int count = 0;
    int cur = 0;
    int pending = -1; // page change requested during a touch; applied after it

    void apply_pending();
    void show(int index);

    static void swipe_thunk(Page *p, SwipeDir dir);
};