#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
extern "C"
{
#include "lcd.h"
}

/**
 * Widget.hpp - the on-screen widgets (buttons, sliders, graphs, etc.)
 *
 * Things to keep in mind when using them:
 * 
 *  - Positions are in pixels, with (0,0) at the top-left of the 800x480 screen.
 *    x goes right, y goes down.
 *  - For colours, use the macros from lcd.h (WHITE, RED, GRAY...) or
 *    RGB565(r, g, b) with each value 0-255.
 *  - font_size can only be 12, 16, 24 or 32. Each character is font_size/2
 *    pixels wide and font_size pixels tall.
 *  - Text is plain ASCII. DONOT use other encodings like utf-8 or EASCII.
 *  - Make widgets static or global. A Page only keeps pointers to them, so
 *    they need to still exist as long as the Page does.
 *
 * Widgets just draw themselves and react to touch. They never look at the
 * raw touch data. Page figures out what the touch means and calls the
 * touch_*() functions below.
 */

/** A point on the screen, in pixels. */
struct Point
{
    int16_t x = 0, y = 0;
};

/** How a widget wants to be treated by Page when the screen is touched. */
enum class TouchPolicy : uint8_t
{
    //What the widget does with the touch is up to it. Page just calls the hooks by seeing the type of Gesuture that occured 
    //NONE,TAP_ONLY,CAPTURE are the three types of touch policies that a widget can have.
    //Page checks which policy a widget has and calls the appropriate touch hook functions on the widget.
    //Eg: If a widget has TouchPolicy::TAP_ONLY, Page will call the touch_tap() function on the widget when a tap gesture is detected on the widget.
    //Eg: If a widget has TouchPolicy::CAPTURE, Page will call the touch_move() function on the widget when a move gesture is detected on the widget.
    //EG: If a widget has TouchPolicy::NONE, Page will not call any touch hook functions on the widget.

    NONE,
    TAP_ONLY,
    CAPTURE
};


class Widget
{
public:
    /**
     * @param x  Left edge, in pixels.
     * @param y  Top edge, in pixels.
     * @param w  Width in pixels.
     * @param h  Height in pixels.
     *
     * Defaults after construction (change them afterwards if you want):
     *   color_fg = BLACK, color_bg = WHITE, color_border = GRAY,
     *   font_size = 16, visible, TouchPolicy::TAP_ONLY.
     */
    Widget(int16_t x, int16_t y, int16_t w, int16_t h);
    virtual ~Widget() {}

    // ---------------------------------------------------------------- drawing

    /**
     * @brief Paint the widget into the framebuffer. Each widget implements this.
     * You normally never call it yourself: Page calls render() every frame.
     */
    virtual void draw() = 0;

    /**
     * @brief Draw the widget ONLY if it is visible and marked dirty.
     * Called by Page every frame.
     * @return true if it actually drew (the screen needs an lcd_flush()),
     *         false if there was nothing to do.
     */
    bool render();

    /** Make the widget visible, mark it dirty and draw it immediately. */
    void show();

    /** Hide the widget: stops it drawing and paints its rectangle with color_bg. */
    void hide();

    /**
     * @brief Request a redraw on the next Page::draw(). Widgets call this
     * themselves whenever their state changes; call it yourself only if you
     * changed a public colour/font field after the widget was first drawn.
     */
    void mark_dirty() { needs_redraw = true; }

    /**
     * @brief Called once per frame by Page::update(). Default does nothing.
     * Override for widgets that must do work every frame (FpsCounter does).
     */
    virtual void tick() {}

    // ------------------------------------------------------------------ touch

    /**
     * How Page treats this widget for touch. Set it once, in the
     * constructor (each widget below already sets the right one).
     * See TouchPolicy for the three modes.
     */
    TouchPolicy policy = TouchPolicy::TAP_ONLY;

    /**
     * @brief Is the point inside this widget's rectangle?
     * @param px  X of the point, in pixels.
     * @param py  Y of the point, in pixels.
     * @return true if inside.
     */
    bool contains(int16_t px, int16_t py) const;

    /** @return true if the widget is currently visible (not hidden). */
    bool is_visible() const { return visible; }

    // The hooks below are called ONLY by Page. A widget overrides the ones it
    // cares about; every other hook keeps this empty default.

    /**
     * @brief Finger landed on this widget.
     * @param p  Where it landed.
     */
    virtual void touch_down(Point p) {}

    /**
     * @brief CAPTURE widgets only: the finger moved, called on every movement.
     * @param p      Current finger position.
     * @param delta  How far it moved since the previous call (p - previous p).
     */
    virtual void touch_move(Point p, Point delta) {}

    /**
     * @brief CAPTURE widgets only: finger lifted.
     * @param p  Last known finger position.
     */
    virtual void touch_up(Point p) {}

    /** @brief TAP_ONLY widgets only: press and release both happened on the widget. */
    virtual void touch_tap() {}

    /**
     * @brief Set to true on widgets that can be pinch-zoomed. A pinch goes to the
     *        zoomable widget under the midpoint of the two fingers.
     */
    bool zoomable = false;

    /**
     * @brief A two-finger pinch just started on this widget (zoomable widgets only).
     *        Remember your current view here: scale is measured from this moment.
     * @param center  Midpoint between the two fingers.
     */
    virtual void touch_zoom_begin(Point center) {}

    /**
     * @brief Live pinch. Called on every change while two fingers are down
     *        (zoomable widgets only). Zoom and pan both come from this one call.
     * @param scale   Finger distance now / when the pinch began. 1 = unchanged,
     *                above 1 = fingers moving apart, below 1 = together.
     * @param center  Midpoint between the two fingers right now. Moving it pans.
     */
    virtual void touch_zoom(float scale, Point center) {}

    /** TAP_ONLY widgets only: the touch was taken away (finger slid off, or
     *  became a swipe), so undo any "pressed" look. */
    virtual void touch_cancel() {}

    // ------------------------------------------------------------------- look
    // Set these AFTER constructing the widget. Changing them later also needs
    // mark_dirty() for the change to show.

    uint32_t color_fg;     ///< Text / foreground colour.
    uint32_t color_bg;     ///< Background fill colour.
    uint32_t color_border; ///< Rectangle outline colour.
    uint8_t font_size;     ///< 12, 16, 24 or 32 (pixels tall).

protected:
    /** Fill the widget rectangle with color_bg and outline it with color_border. */
    void draw_background();

    /**
     * Draw one line of text centred inside the widget rectangle.
     * @param text   Text to draw. NULL or "" draws nothing.
     * @param color  Text colour.
     */
    void draw_label_centered(const char *text, uint32_t color);

    /**
     * Copy text into the widget's own label buffer. The text is COPIED, so a
     * temporary buffer (e.g. from snprintf) is safe to pass.
     * @param text  NULL or any string. Longer than 47 characters is cut off.
     */
    void set_label(const char *text);

    bool needs_redraw = true; ///< Every widget starts dirty so it paints once.
    int16_t x, y, w, h;
    bool visible;
    char label[48];
};

/** Push button with a centred text label. Fires callbacks on press / tap. */
class Button : public Widget
{
public:
    /**
     * @param x      Left edge.
     * @param y      Top edge.
     * @param w      Width.
     * @param h      Height.
     * @param label  Text shown on the button. Copied (max 47 characters).
     */
    Button(int16_t x, int16_t y, int16_t w, int16_t h, const char *label);
    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /**
     * @brief Finger landed on this widget.
     * @param p  Where it landed.
     */
    void touch_down(Point p) override;
    /** @brief Tap completed on the button: clears the pressed look and calls on_release. */
    void touch_tap() override;
    /** @brief Touch was taken away (finger slid off / became a swipe): clears the pressed look. */
    void touch_cancel() override;

    /**
     * Optional. Called the moment the finger LANDS on the button.
     * Signature: void fn(Button *button);   button = the button that was pressed.
     * Default nullptr = do nothing.
     */
    void (*on_press)(Button *button);

    /**
     * Optional. Called when a tap COMPLETES (finger lifted while still on
     * the button). Not called if the finger slid away or turned into a swipe.
     * Use this one for "do something" actions.
     * Signature: void fn(Button *button);
     */
    void (*on_release)(Button *button);

    uint32_t color_pressed; ///< Background colour while held down (default GRAY).

private:
    bool pressed;
};

/** Non-interactive text. TouchPolicy::NONE. */
class Label : public Widget
{
public:
    /**
     * @param x     Left edge.
     * @param y     Top edge.
     * @param w     Width. Text wider than this runs past the edge, so size it
     *              to fit: width >= number_of_characters * (font_size / 2).
     * @param h     Height (>= font_size).
     * @param text  Initial text. Copied (max 47 characters).
     */
    Label(int16_t x, int16_t y, int16_t w, int16_t h, const char *text);

    /**
     * Change the text and redraw. Copied, so a temporary buffer is fine.
     * @param text  New text (max 47 characters).
     */
    void set_text(const char *text);
    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /** Horizontal alignment: 0 = left, 1 = centre (default), 2 = right. */
    uint8_t align;
};

/** Two-state button. Each tap flips it; the colour and text change. */
class ToggleButton : public Widget
{
public:
    /**
     * @param x          Left edge.
     * @param y          Top edge.
     * @param w          Width.
     * @param h          Height.
     * @param label_off  Text while OFF. NOT copied: pass a string literal or
     *                   something that lives forever.
     * @param label_on   Text while ON. NOT copied: same rule.
     */
    ToggleButton(int16_t x, int16_t y, int16_t w, int16_t h,
                 const char *label_off, const char *label_on);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;
    /** @brief Tap: flips the state and calls on_change. */
    void touch_tap() override;

    /** @return true if currently ON. */
    bool get_state() const { return state; }

    /**
     * Set the state from code. Redraws, but does NOT call on_change.
     * @param s  true = ON, false = OFF.
     */
    void set_state(bool s);

    /**
     * Optional. Called when the user taps it (not from set_state()).
     * Signature: void fn(ToggleButton *toggle, bool state);
     *   toggle = the toggle, state = the NEW state (true = ON).
     */
    void (*on_change)(ToggleButton *toggle, bool state);

    uint32_t color_on;  ///< Background when ON  (default GREEN).
    uint32_t color_off; ///< Background when OFF (default GRAY).

private:
    bool state;
    const char *label_on;
    const char *label_off;
};

/**
 * Horizontal slider. The value follows the finger LIVE while you drag, and
 * jumps to wherever you first touch. TouchPolicy::CAPTURE.
 */
class Slider : public Widget
{
public:
    /**
     * @param x        Left edge.
     * @param y        Top edge.
     * @param w        Width (the track runs across it, minus the thumb radius).
     * @param h        Height (the track is drawn at the vertical centre).
     * @param min_val  Value at the far left.
     * @param max_val  Value at the far right (should be > min_val).
     * @param initial  Starting value. Clamped into [min_val, max_val].
     */
    Slider(int16_t x, int16_t y, int16_t w, int16_t h,
           int32_t min_val, int32_t max_val, int32_t initial);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /**
     * @brief Finger landed on this widget.
     * @param p  Where it landed.
     */
    void touch_down(Point p) override;
    /**
     * @brief The finger moved while held on this widget (live).
     * @param p      Current finger position.
     * @param delta  Movement since the previous call.
     */
    void touch_move(Point p, Point delta) override;
    /**
     * @brief Finger lifted.
     * @param p  Last known finger position.
     */
    void touch_up(Point p) override;

    /** @return Current value, between min_val and max_val. */
    int32_t get_value() const { return value; }

    /**
     * Set the value from code. Clamped, redraws, but does NOT call on_change.
     * @param v  New value.
     */
    void set_value(int32_t v);

    /**
     * Optional. Called on EVERY finger movement that changes the value, so
     * you can react live while dragging. Not called by set_value().
     * Signature: void fn(Slider *slider, int32_t value);
     */
    void (*on_change)(Slider *slider, int32_t value);

    uint32_t color_track; ///< Unfilled part of the track (default GRAY).
    uint32_t color_fill;  ///< Filled part, left of the thumb (default BLUE).
    uint32_t color_thumb; ///< The round handle (default WHITE).

private:
    int32_t value, min_val, max_val;

    void follow_finger(int16_t px); // updates value + fires on_change
    int32_t x_to_value(int16_t px) const;
    int16_t value_to_x() const;

    static constexpr int16_t THUMB_R = 6;
};

/** Read-only progress bar, 0-100 %. TouchPolicy::NONE. */
class ProgressBar : public Widget
{
public:
    /**
     * @param x  Left edge.
     * @param y  Top edge.
     * @param w  Width.
     * @param h  Height.
     * Starts at 0 %.
     */
    ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /**
     * @param percent  0..100. Values above 100 are clamped to 100.
     */
    void set_value(uint8_t percent);

    /** @return Current value, 0..100. */
    uint8_t get_value() const { return value; }

    uint32_t color_fill;  ///< Filled part (default GREEN).
    uint32_t color_track; ///< Empty part (default GRAY).

private:
    uint8_t value;
};

/** Square tick-box with a text label drawn to its right. */
class Checkbox : public Widget
{
public:
    /**
     * Width is worked out for you: box_size + 4 + (number of characters * 8).
     * Height equals box_size. The whole box plus label is tappable.
     *
     * @param x         Left edge.
     * @param y         Top edge.
     * @param box_size  Side of the square box, in pixels (e.g. 32).
     * @param label     Text to the right of the box. Copied (max 47 characters).
     *                  Must not be NULL.
     */
    Checkbox(int16_t x, int16_t y, int16_t box_size, const char *label);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;
    /** @brief Tap: flips the tick and calls on_change. */
    void touch_tap() override;

    /** @return true if ticked. */
    bool get_checked() const { return checked; }

    /**
     * Set from code. Redraws, but does NOT call on_change.
     * @param c  true = ticked.
     */
    void set_checked(bool c);

    /**
     * Optional. Called when the user taps it (not from set_checked()).
     * Signature: void fn(Checkbox *box, bool checked);  checked = the NEW state.
     */
    void (*on_change)(Checkbox *box, bool checked);

    uint32_t color_check; ///< Colour of the tick mark (default BLACK).

private:
    bool checked;
    int16_t box_size;
};

/**
 * Scrolling line graph. Newest sample on the RIGHT. Keeps the last MAX_POINTS
 * samples.
 *
 *   Two fingers : pinch to zoom, and slide both fingers to pan back through history.
 *                 The sample under your fingers stays under your fingers.
 *   Tap         : snap back to live (fully zoomed out, newest sample at the right).
 *   One finger  : still swipes the page.
 * While you are panned back, the view stays on the same samples as new ones arrive.
 */
class Graph : public Widget
{
public:
    /** Maximum samples stored. */
    static constexpr int MAX_POINTS = 2048;

    /**
     * @param x  Left edge.
     * @param y  Top edge.
     * @param w  Total width, including the axis-number margin on the left.
     * @param h  Total height, including the title row on top.
     * Starts with auto-scaling Y axis and 4 x 4 grid divisions.
     */
    Graph(int16_t x, int16_t y, int16_t w, int16_t h);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /** @brief Tap on the graph: snap back to live (see reset_view()). */
    void touch_tap() override;

    /**
     * @brief Pinch started: remembers which sample is under the fingers.
     * @param center  Midpoint between the two fingers.
     */
    void touch_zoom_begin(Point center) override;

    /**
     * @brief Live pinch: zooms with the finger distance and pans with the finger midpoint.
     * @param scale   Finger distance now / when the pinch began (above 1 = zoom in).
     * @param center  Midpoint between the two fingers right now.
     */
    void touch_zoom(float scale, Point center) override;

    /** @brief Back to live: fully zoomed out, newest sample at the right edge. */
    void reset_view();

    /** @return true if the view is following the newest sample (not panned back). */
    bool is_live() const { return offset == 0; }

    /**
     * Append one sample (the oldest one drops off once MAX_POINTS is reached).
     * @param value  The sample, any int32 number.
     */
    void push(int32_t value);

    /**
     * Replace all samples at once.
     * @param data   Array of samples, oldest first, newest last.
     * @param count  How many entries. More than MAX_POINTS keeps the first
     *               MAX_POINTS.
     */
    void set_data(const int32_t *data, int count);

    /** Remove all samples. */
    void clear();

    /**
     * Fix the Y axis range.
     * @param min_y  Bottom of the Y axis.
     * @param max_y  Top of the Y axis.
     * Pass the SAME value for both (e.g. 0, 0) to switch back to automatic
     * scaling from the data.
     */
    void set_y_range(int32_t min_y, int32_t max_y);

    /** Optional heading drawn centred above the plot. NOT copied: pass a
     *  string literal. Default nullptr = no title. */
    const char *title;

    /** Reserved for a Y-axis caption. Currently NOT drawn. */
    const char *label_y;

    uint32_t color_line;       ///< Data line (default RED).
    uint32_t color_grid;       ///< Grid lines (default 0x444444).
    uint32_t color_axis;       ///< Plot frame (default WHITE).
    uint8_t grid_divisions_x;  ///< Vertical grid divisions (default 4).
    uint8_t grid_divisions_y;  ///< Horizontal grid divisions (default 4).

private:
    int32_t samples[MAX_POINTS];
    int count;
    int head;

    int32_t y_min, y_max;
    bool auto_scale;

    int16_t px, py, pw, ph;

    void compute_plot_area();
    void compute_y_range();
    int16_t map_y(int32_t v) const;

    bool dirty_full;

    // view window: `window` samples wide (0 = auto: show all that fit), ending
    // `offset` samples back from the newest one (0 = live)
    int window;
    int offset;
    int base_window;  // window when the pinch began
    float anchor_age; // age of the sample that was under the fingers at pinch start

    int effective_window();
    void clamp_offset();

    static constexpr int MIN_WINDOW = 8; // most you can zoom in: 8 samples
};

/**
 * Small "FPS:nn" readout. It counts how often Page::update() runs (one tick
 * per call), which with your 10 ms UI loop is about 100. Assumes a 480 MHz CPU
 * clock. Fixed size 72 x 20, black background, green text.
 */
class FpsCounter : public Widget
{
public:
    /**
     * @param x  Left edge.
     * @param y  Top edge.
     */
    FpsCounter(int16_t x, int16_t y);
    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;
    /** @brief Called once per Page::update(): measures the rate and marks the widget dirty. */
    void tick() override;

private:
    uint32_t last_cycle;
    uint32_t count;
    uint32_t fps;
    char buf[12];
};

/**
 * Tile with a small caption on top and a big value underneath, plus a
 * coloured stripe on the left edge. The stripe and value are RED when the
 * value text is exactly "OFFLINE", GREEN for anything else.
 */
class InfoTile : public Widget
{
public:
    /**
     * @param x           Left edge.
     * @param y           Top edge.
     * @param w           Width.
     * @param h           Height (about 80-100 fits caption + value at the
     *                    default font).
     * @param label_text  Caption (small, grey). Copied (max 47 characters).
     * @param value_text  Initial big value text. Copied (max 15 characters).
     *                    NULL = empty.
     * font_size defaults to 24; the value is drawn 8 pixels larger.
     */
    InfoTile(int16_t x, int16_t y, int16_t w, int16_t h,
             const char *label_text, const char *value_text);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;
    /** @brief Tap: calls on_tap. */
    void touch_tap() override;

    /**
     * Change the big value text and redraw.
     * @param v  New text. Copied, so a temporary buffer is fine. Longer than
     *           15 characters is cut off. NULL = empty.
     */
    void set_value(const char *v);

    /**
     * Optional. Called when the tile is tapped.
     * Signature: void fn(InfoTile *tile);
     */
    void (*on_tap)(InfoTile *tile);

    /** Free pointer for your own use (e.g. to find your data inside on_tap). */
    void *userdata = nullptr;

private:
    char value_buf[16];
};

/** Small pill with a coloured dot (green = healthy, red = not) and a name. */
class StatusChip : public Widget
{
public:
    /**
     * @param x     Left edge.
     * @param y     Top edge.
     * @param w     Width (dot + name must fit).
     * @param h     Height (the dot is h/4 in radius).
     * @param name  Text next to the dot. Copied (max 47 characters).
     * Starts as NOT healthy (red).
     */
    StatusChip(int16_t x, int16_t y, int16_t w, int16_t h, const char *name);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /**
     * @param ok  true = green dot, false = red dot. Redraws only if it changed.
     */
    void set_healthy(bool ok);

private:
    bool healthy;
};

/**
 * Scrolling colour-coded log, newest line at the BOTTOM. Drag your finger
 * DOWN to look at older lines, drag UP to return to the live tail.
 * scroll_offset 0 = live. TouchPolicy::CAPTURE, so scrolling never changes
 * the page.
 */
class LogView : public Widget
{
public:
    /** Number of lines remembered. */
    static constexpr int MAX_LINES = 32;

    /**
     * @param x  Left edge.
     * @param y  Top edge.
     * @param w  Width. Lines longer than about (w / (font_size / 2)) characters
     *           run past the edge.
     * @param h  Height. The number of visible rows is (h - 4) / (font_size + 4).
     * Black background, white text.
     */
    LogView(int16_t x, int16_t y, int16_t w, int16_t h);

    /** @brief Paints the widget. Called for you by Page (via render()) when the widget is dirty. */
    void draw() override;

    /** @brief Finger landed: remembers where, so dragging scrolls relative to it.
     * @param p  Where it landed. */
    void touch_down(Point p) override;
    /**
     * @brief The finger moved while held on this widget (live).
     * @param p      Current finger position.
     * @param delta  Movement since the previous call.
     */
    void touch_move(Point p, Point delta) override;

    /**
     * Add a line at the bottom. The oldest line is dropped once MAX_LINES is
     * reached. If the user has scrolled back, the view stays where it is.
     * @param text   Line text. Copied, so a temporary buffer is fine.
     *               Longer than 63 characters is cut off.
     * @param level  0 = INFO ("[INFO]"), 1 = WARN ("[WARN]"), 2 = ERR ("[ERR]").
     *               Values above 2 are shown as INFO.
     */
    void push_line(const char *text, uint8_t level);

    /** Remove all lines and jump back to the live tail. */
    void clear();

    /**
     * Scroll back from the newest line.
     * @param lines_back  0 = live tail, 1 = one line back, ... Clamped to
     *                    what is actually available.
     */
    void set_scroll_offset(int lines_back);

    /** @return How many lines back from the newest the view is (0 = live). */
    int get_scroll_offset() const { return scroll_offset; }

private:
    char lines[MAX_LINES][64];
    uint8_t levels[MAX_LINES];
    int head;  // next write index
    int count; // valid lines (<= MAX_LINES)

    int scroll_offset; // lines back from the newest; 0 = live tail

    int16_t drag_start_y;
    int scroll_offset_at_drag_start;

    int line_height() const;
    int visible_lines() const;
    int max_scroll_offset() const;

    uint32_t color_for_level(uint8_t level) const;
};