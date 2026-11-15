#include "Widget.hpp"

Widget::Widget(int16_t x, int16_t y, int16_t w, int16_t h)
    : x(x), y(y), w(w), h(h),
      color_fg(BLACK), color_bg(WHITE), color_border(GRAY),
      visible(true), font_size(16)
{
    label[0] = '\0';
}

bool Widget::render()
{
    if (!visible || !needs_redraw)
        return false;
    draw();
    needs_redraw = false;
    return true;
}

void Widget::set_label(const char *text)
{
    if (text)
    {
        strncpy(label, text, sizeof(label) - 1);
        label[sizeof(label) - 1] = '\0';
    }
    else
    {
        label[0] = '\0';
    }
}

void Widget::show()
{
    visible = true;
    mark_dirty();
    draw();
}

void Widget::hide()
{
    visible = false;
    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
}

bool Widget::contains(int16_t px, int16_t py) const
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

void Widget::draw_background()
{
    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);
}

void Widget::draw_label_centered(const char *text, uint32_t color)
{
    if (!text || text[0] == '\0')
        return;
    int16_t char_w = font_size / 2;
    int16_t text_w = (int16_t)(strlen(text) * char_w);
    int16_t tx = x + (w - text_w) / 2;
    int16_t ty = y + (h - font_size) / 2;
    lcd_show_string(tx, ty, text_w, font_size, font_size,
                    const_cast<char *>(text), color);
}


Button::Button(int16_t x, int16_t y, int16_t w, int16_t h, const char *lbl)
    : Widget(x, y, w, h),
      pressed(false), on_press(nullptr), on_release(nullptr),
      color_pressed(GRAY)
{
    policy = TouchPolicy::TAP_ONLY;
    set_label(lbl);
}

void Button::draw()
{
    if (!visible)
        return;
    lcd_fill(x, y, x + w - 1, y + h - 1, pressed ? color_pressed : color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);
    draw_label_centered(label, color_fg);
}



Label::Label(int16_t x, int16_t y, int16_t w, int16_t h, const char *text)
    : Widget(x, y, w, h), align(1)
{
    policy = TouchPolicy::NONE;
    set_label(text);
    color_border = color_bg;
}

void Label::set_text(const char *text)
{
    set_label(text);
    mark_dirty();
}

void Label::draw()
{
    if (!visible)
        return;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);

    if (label[0] == '\0')
        return;

    int16_t char_w = font_size / 2;
    int16_t text_w = (int16_t)(strlen(label) * char_w);
    int16_t ty = y + (h - font_size) / 2;
    int16_t tx;

    switch (align)
    {
    case 0:
        tx = x + 2;
        break;
    case 2:
        tx = x + w - text_w - 2;
        break;
    default:
        tx = x + (w - text_w) / 2;
        break;
    }

    lcd_show_string(tx, ty, text_w, font_size, font_size, label, color_fg);
}


// --------------------------------------------------------- ToggleButton ----

ToggleButton::ToggleButton(int16_t x, int16_t y, int16_t w, int16_t h,
                           const char *lbl_off, const char *lbl_on)
    : Widget(x, y, w, h),
      on_change(nullptr),
      color_on(GREEN), color_off(GRAY),
      state(false),
      label_on(lbl_on), label_off(lbl_off)
{
    policy = TouchPolicy::TAP_ONLY;
    set_label(lbl_off);
}

void ToggleButton::set_state(bool s)
{
    state = s;
    mark_dirty();
}

void ToggleButton::draw()
{
    if (!visible)
        return;

    uint32_t bg = state ? color_on : color_off;
    const char *lbl = state ? label_on : label_off;

    lcd_fill(x, y, x + w - 1, y + h - 1, bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);
    draw_label_centered(lbl, color_fg);
}


// --------------------------------------------------------------- Slider ----

Slider::Slider(int16_t x, int16_t y, int16_t w, int16_t h,
               int32_t min_v, int32_t max_v, int32_t init)
    : Widget(x, y, w, h),
      on_change(nullptr),
      color_track(GRAY), color_fill(BLUE), color_thumb(WHITE),
      value(init), min_val(min_v), max_val(max_v)
{
    policy = TouchPolicy::CAPTURE; // follows the finger live

    if (value < min_val)
        value = min_val;
    if (value > max_val)
        value = max_val;
}

int32_t Slider::x_to_value(int16_t px) const
{
    int16_t track_x0 = x + THUMB_R;
    int16_t track_w = w - 2 * THUMB_R;
    if (track_w <= 0)
        return min_val;

    int32_t clamped = px - track_x0;
    if (clamped < 0)
        clamped = 0;
    if (clamped > track_w)
        clamped = track_w;

    return min_val + (int32_t)clamped * (max_val - min_val) / track_w;
}

int16_t Slider::value_to_x() const
{
    int16_t track_x0 = x + THUMB_R;
    int16_t track_w = w - 2 * THUMB_R;
    if (max_val == min_val)
        return track_x0;

    return (int16_t)(track_x0 +
                     (int32_t)(value - min_val) * track_w / (max_val - min_val));
}

void Slider::set_value(int32_t v)
{
    if (v < min_val)
        v = min_val;
    if (v > max_val)
        v = max_val;
    value = v;
    mark_dirty();
}

void Slider::draw()
{
    if (!visible)
        return;

    int16_t cy = y + h / 2;
    int16_t track_y = cy - 2;
    int16_t tx = value_to_x();
    int16_t tx0 = x + THUMB_R;
    int16_t tx1 = x + w - THUMB_R;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);

    if (tx > tx0)
        lcd_fill(tx0, track_y, tx, track_y + 3, color_fill);

    if (tx < tx1)
        lcd_fill(tx + 1, track_y, tx1, track_y + 3, color_track);

    lcd_fill_circle(tx, cy, THUMB_R, color_thumb);
    lcd_draw_circle(tx, cy, THUMB_R, color_border);
}


// ----------------------------------------------------------- ProgressBar --

ProgressBar::ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h)
    : Widget(x, y, w, h),
      color_fill(GREEN), color_track(GRAY),
      value(0)
{
    policy = TouchPolicy::NONE;
}

void ProgressBar::set_value(uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    value = percent;
    mark_dirty();
}

void ProgressBar::draw()
{
    if (!visible)
        return;

    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);

    int16_t inner_w = w - 2;
    int16_t fill_w = (int16_t)((int32_t)value * inner_w / 100);

    if (fill_w > 0)
        lcd_fill(x + 1, y + 1, x + fill_w, y + h - 2, color_fill);

    if (fill_w < inner_w)
        lcd_fill(x + 1 + fill_w, y + 1, x + w - 2, y + h - 2, color_track);
}


// -------------------------------------------------------------- Checkbox --

Checkbox::Checkbox(int16_t x, int16_t y, int16_t box_sz, const char *lbl)
    : Widget(x, y,
             box_sz + 4 + (int16_t)(strlen(lbl) * 8),
             box_sz),
      on_change(nullptr),
      color_check(BLACK),
      checked(false),
      box_size(box_sz)
{
    policy = TouchPolicy::TAP_ONLY;
    set_label(lbl);
}

void Checkbox::set_checked(bool c)
{
    checked = c;
    mark_dirty();
}

void Checkbox::draw()
{
    if (!visible)
        return;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + box_size - 1, y + box_size - 1, color_border);

    if (checked)
    {
        int16_t mid_x = x + box_size / 3;
        int16_t mid_y = y + box_size * 3 / 4;
        int16_t left_x = x + 3;
        int16_t left_y = y + box_size / 2;
        int16_t top_x = x + box_size - 3;
        int16_t top_y = y + 3;

        lcd_draw_line(left_x, left_y, mid_x, mid_y, color_check);
        lcd_draw_line(mid_x, mid_y, top_x, top_y, color_check);
    }

    if (label[0] != '\0')
    {
        int16_t lx = x + box_size + 4;
        int16_t ly = y + (box_size - font_size) / 2;
        int16_t lw = (int16_t)(strlen(label) * (font_size / 2));
        lcd_show_string(lx, ly, lw, font_size, font_size, label, color_fg);
    }
}



Graph::Graph(int16_t x, int16_t y, int16_t w, int16_t h)
    : Widget(x, y, w, h),
      title(nullptr), label_y(nullptr),
      color_line(RED), color_grid(0x444444), color_axis(WHITE),
      grid_divisions_x(4), grid_divisions_y(4),
      count(0), head(0),
      y_min(0), y_max(100),
      auto_scale(true),
      px(0), py(0), pw(0), ph(0),
      dirty_full(true)
{
    policy = TouchPolicy::NONE;
    memset(samples, 0, sizeof(samples));
}

void Graph::push(int32_t value)
{
    samples[head] = value;
    head = (head + 1) % MAX_POINTS;
    if (count < MAX_POINTS)
        count++;
    if (auto_scale)
        compute_y_range();
    mark_dirty();
}

void Graph::set_data(const int32_t *data, int n)
{
    count = 0;
    head = 0;
    if (n > MAX_POINTS)
        n = MAX_POINTS;
    memcpy(samples, data, n * sizeof(int32_t));
    count = n;
    head = n % MAX_POINTS;
    if (auto_scale)
        compute_y_range();
    dirty_full = true;
    mark_dirty();
}

void Graph::clear()
{
    count = 0;
    head = 0;
    dirty_full = true;
    mark_dirty();
}

void Graph::set_y_range(int32_t min_y, int32_t max_y)
{
    auto_scale = (min_y == max_y);
    if (!auto_scale)
    {
        y_min = min_y;
        y_max = max_y;
    }
    dirty_full = true;
    mark_dirty();
}

void Graph::compute_plot_area()
{
    int16_t margin_l = 36;
    int16_t margin_r = 6;
    int16_t margin_t = font_size + 4;
    int16_t margin_b = font_size + 4;

    px = x + margin_l;
    py = y + margin_t;
    pw = w - margin_l - margin_r;
    ph = h - margin_t - margin_b;
}

void Graph::compute_y_range()
{
    if (count == 0)
        return;
    int32_t lo = samples[0], hi = samples[0];
    for (int i = 1; i < count; ++i)
    {
        if (samples[i] < lo)
            lo = samples[i];
        if (samples[i] > hi)
            hi = samples[i];
    }
    if (lo == hi)
    {
        lo -= 1;
        hi += 1;
    }
    int32_t pad = (hi - lo) / 10;
    y_min = lo - pad;
    y_max = hi + pad;
}

int16_t Graph::map_y(int32_t v) const
{
    if (y_max == y_min)
        return py + ph / 2;
    int32_t clamped = v < y_min ? y_min : v > y_max ? y_max : v;
    return (int16_t)(py + ph - 1 - (int32_t)(clamped - y_min) * (ph - 1) / (y_max - y_min));
}

void Graph::draw()
{
    if (!visible)
        return;
    compute_plot_area();

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(px, py, px + pw - 1, py + ph - 1, color_axis);

    if (title)
    {
        int16_t tw = (int16_t)(strlen(title) * font_size / 2);
        int16_t tx = x + (w - tw) / 2;
        lcd_show_string(tx, y + 2, tw, font_size, font_size,
                        const_cast<char *>(title), color_fg);
    }

    char buf[12];
    for (int i = 0; i <= grid_divisions_y; ++i)
    {
        int16_t gy = (int16_t)(py + i * (ph - 1) / grid_divisions_y);
        if (i > 0 && i < grid_divisions_y)
            lcd_draw_hline(px + 1, gy, pw - 2, color_grid);

        int32_t val = y_max - (int32_t)i * (y_max - y_min) / grid_divisions_y;
        snprintf(buf, sizeof(buf), "%ld", (long)val);
        int16_t lw = (int16_t)(strlen(buf) * font_size / 2);
        lcd_show_string(px - lw - 2, gy - font_size / 2,
                        lw, font_size, font_size, buf, color_fg);
    }

    for (int i = 0; i <= grid_divisions_x; ++i)
    {
        int16_t gx = (int16_t)(px + i * (pw - 1) / grid_divisions_x);
        if (i > 0 && i < grid_divisions_x)
            lcd_draw_vline(gx, py + 1, ph - 2, color_grid);
    }

    int n = (count < pw) ? count : pw;
    if (n >= 2)
    {
        for (int i = 1; i < n; ++i)
        {
            int age0 = n - 1 - i;
            int age1 = n - i;
            int si0 = ((head - 1 - age0) % MAX_POINTS + MAX_POINTS) % MAX_POINTS;
            int si1 = ((head - 1 - age1) % MAX_POINTS + MAX_POINTS) % MAX_POINTS;

            int16_t x0 = (int16_t)(px + pw - 1 - age0 * (pw - 1) / (n - 1));
            int16_t x1 = (int16_t)(px + pw - 1 - age1 * (pw - 1) / (n - 1));
            int16_t y0 = map_y(samples[si0]);
            int16_t y1 = map_y(samples[si1]);

            lcd_draw_line(x0, y0, x1, y1, color_line);
        }
    }

    dirty_full = false;
}


// ------------------------------------------------------------ FpsCounter --

FpsCounter::FpsCounter(int16_t x, int16_t y)
    : Widget(x, y, 72, 20),
      last_cycle(0), count(0), fps(0)
{
    policy = TouchPolicy::NONE;
    buf[0] = '\0';
    color_bg = BLACK;
    color_fg = GREEN;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void FpsCounter::draw()
{
    if (!visible)
        return;
    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    draw_label_centered(buf, color_fg);
}


// -------------------------------------------------------------- InfoTile --

InfoTile::InfoTile(int16_t x, int16_t y, int16_t w, int16_t h,
                   const char *label_text, const char *value_text)
    : Widget(x, y, w, h), on_tap(nullptr)
{
    policy = TouchPolicy::TAP_ONLY;
    set_label(label_text);

    value_buf[0] = '\0';
    if (value_text)
    {
        strncpy(value_buf, value_text, sizeof(value_buf) - 1);
        value_buf[sizeof(value_buf) - 1] = '\0';
    }
    font_size = 24;
}

void InfoTile::set_value(const char *v)
{
    if (v)
    {
        strncpy(value_buf, v, sizeof(value_buf) - 1);
        value_buf[sizeof(value_buf) - 1] = '\0';
    }
    else
    {
        value_buf[0] = '\0';
    }
    mark_dirty();
}

void InfoTile::draw()
{
    if (!visible)
        return;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);

    // Status accent, derived from whatever set_value() last wrote. This
    // relies on Display always passing exactly "OK"/"OFFLINE" (it does,
    // in on_telemetry()) — if you want this driven by an explicit bool
    // instead of string-matching, say so and I'll add a proper
    // set_status(bool ok, const char *text) to Widget.hpp.
    bool is_offline = (strcmp(value_buf, "OFFLINE") == 0);
    uint32_t accent = is_offline ? RED : GREEN;

    const int16_t ACCENT_W = 5;
    lcd_fill(x + 1, y + 1, x + ACCENT_W, y + h - 2, accent);

    int16_t text_x0 = x + ACCENT_W + 8;
    int16_t text_area_w = w - (text_x0 - x) - 6;

    if (label[0] != '\0')
        lcd_show_string(text_x0, y + 4, text_area_w, font_size, font_size, label, GRAY);

    if (value_buf[0] != '\0')
    {
        int16_t value_font = font_size + 8; // noticeably bigger than the label
        int16_t vy = y + font_size + 10;

        // Fake bold: draw twice, 1px apart, to thicken the strokes.
        lcd_show_string(text_x0 + 1, vy, text_area_w, value_font, value_font, value_buf, accent);
        lcd_show_string(text_x0,     vy, text_area_w, value_font, value_font, value_buf, accent);
    }
}

// ------------------------------------------------------------ StatusChip --

StatusChip::StatusChip(int16_t x, int16_t y, int16_t w, int16_t h, const char *name)
    : Widget(x, y, w, h),
      healthy(false)
{
    policy = TouchPolicy::NONE;
    set_label(name);
    color_bg = RGB565(0x22, 0x22, 0x22);
    color_fg = WHITE;
}

void StatusChip::set_healthy(bool ok)
{
    if (ok != healthy)
    {
        healthy = ok;
        mark_dirty();
    }
}

void StatusChip::draw()
{
    if (!visible)
        return;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);

    int16_t dot_r = h / 4;
    int16_t cx = x + dot_r + 6;
    int16_t cy = y + h / 2;
    uint32_t dot_color = healthy ? GREEN : RED;
    lcd_fill_circle(cx, cy, dot_r, dot_color);

    if (label[0] != '\0')
    {
        int16_t lx = cx + dot_r + 8;
        int16_t ly = y + (h - font_size) / 2;
        int16_t lw = (int16_t)(strlen(label) * (font_size / 2));
        lcd_show_string(lx, ly, lw, font_size, font_size, label, color_fg);
    }
}


// ---------------------------------------------------------------- LogView --

LogView::LogView(int16_t x, int16_t y, int16_t w, int16_t h)
    : Widget(x, y, w, h), head(0), count(0),
      scroll_offset(0),
      drag_start_y(0), scroll_offset_at_drag_start(0)
{
    policy = TouchPolicy::CAPTURE; // scrolls live; page never sees it as a swipe
    memset(lines, 0, sizeof(lines));
    memset(levels, 0, sizeof(levels));
    color_bg = BLACK;
    color_fg = WHITE;
}

void LogView::push_line(const char *text, uint8_t level)
{
    strncpy(lines[head], text, sizeof(lines[head]) - 1);
    lines[head][sizeof(lines[head]) - 1] = '\0';
    levels[head] = level;

    head = (head + 1) % MAX_LINES;
    if (count < MAX_LINES)
        count++;

    // Deliberately NOT resetting scroll_offset here. If the user has
    // scrolled back into history, snapping them back to the live tail
    // every time a new line arrives would make scrollback unusable on a
    // busy log. Leaving it means the visible window drifts one line
    // closer to live per new message, same as most terminal/log viewers.
    mark_dirty();
}

void LogView::clear()
{
    count = 0;
    head = 0;
    scroll_offset = 0;
    mark_dirty();
}

int LogView::line_height() const
{
    return font_size + 4;
}

int LogView::visible_lines() const
{
    int lh = line_height();
    return (lh > 0) ? (h - 4) / lh : 0;
}

int LogView::max_scroll_offset() const
{
    int vis = visible_lines();
    return (count > vis) ? (count - vis) : 0;
}

void LogView::set_scroll_offset(int lines_back)
{
    int max_off = max_scroll_offset();
    if (lines_back < 0)
        lines_back = 0;
    if (lines_back > max_off)
        lines_back = max_off;

    if (lines_back != scroll_offset)
    {
        scroll_offset = lines_back;
        mark_dirty();
    }
}

uint32_t LogView::color_for_level(uint8_t level) const
{
    switch (level)
    {
        case 1:  return 0xBC21; // WARN — RGB565(184,134,11)
        case 2:  return 0xC145; // ERR  — RGB565(198,40,40)
        default: return 0x2104; // INFO — RGB565(32,32,32)
    }
}

void LogView::draw()
{
    if (!visible)
        return;

    lcd_fill(x, y, x + w - 1, y + h - 1, color_bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, color_border);

    int16_t line_h = (int16_t)line_height();
    int max_rows = visible_lines();
    int n = (count < max_rows) ? count : max_rows;

    static const char *level_tag[3] = { "[INFO]", "[WARN]", "[ERR] " };

    // i = 0 is the BOTTOM row (most recent line in the current scroll
    // window); i increases going upward toward older lines. This is the
    // opposite of the old top-down layout, so the log reads like a
    // terminal/chat feed with the newest line anchored to the bottom edge.
    for (int i = 0; i < n; ++i)
    {
        int age = scroll_offset + i; // 0 = true latest line, regardless of window
        if (age >= count)
            continue;

        int idx = ((head - 1 - age) % MAX_LINES + MAX_LINES) % MAX_LINES;

        // Row i from the bottom sits (n-1-i) slots down from the top of
        // the drawable area — same row spacing as before, just filled
        // bottom-up instead of top-down.
        int16_t ly = y + 2 + (n - 1 - i) * line_h;
        uint32_t col = color_for_level(levels[idx]);

        char buf[80];
        snprintf(buf, sizeof(buf), "%s %s", level_tag[levels[idx] > 2 ? 0 : levels[idx]], lines[idx]);

        int16_t lw = (int16_t)(strlen(buf) * (font_size / 2));
        lcd_show_string(x + 4, ly, lw, font_size, font_size, buf, col);
    }

    // Small on-screen cue that you're not looking at the live tail. Easy
    // to remove if you'd rather keep the widget clean.
    if (scroll_offset > 0)
    {
        char tag[16];
        snprintf(tag, sizeof(tag), "-%d", scroll_offset);
        int16_t tw = (int16_t)(strlen(tag) * (font_size / 2));
        lcd_show_string(x + w - tw - 6, y + 2, tw, font_size, font_size, tag, color_fg);
    }
}



//Touch hooks for widgets. These are called by the Page when a touch event occurs, and
// the widget can choose to handle it or ignore it. If the widget handles it, it should
// set the appropriate state and call mark_dirty() to request a redraw. If the widget
// wants to notify the page of an event (like a button press), it can call the callback function if it is set.
// The Page will then call the widget's draw() method to update

// ---- Button: pressed look on touch, callback on a completed tap -------------
void Button::touch_down(Point)
{
    pressed = true;
    mark_dirty();
    if (on_press)
        on_press(this);
}

void Button::touch_tap()
{
    pressed = false;
    mark_dirty();
    if (on_release)
        on_release(this);
}

void Button::touch_cancel()
{
    pressed = false;
    mark_dirty();
}

// ---- ToggleButton / Checkbox / InfoTile: act on a completed tap -------------
void ToggleButton::touch_tap()
{
    state = !state;
    mark_dirty();
    if (on_change)
        on_change(this, state);
}

void Checkbox::touch_tap()
{
    checked = !checked;
    mark_dirty();
    if (on_change)
        on_change(this, checked);
}

void InfoTile::touch_tap()
{
    if (on_tap)
        on_tap(this);
}

// ---- Slider: follows the finger on EVERY move ------------------------------
void Slider::follow_finger(int16_t px)
{
    int32_t v = x_to_value(px);
    if (v == value)
        return;
    value = v;
    mark_dirty();
    if (on_change)
        on_change(this, value);
}

void Slider::touch_down(Point p) { follow_finger(p.x); }       // jump to touch
void Slider::touch_move(Point p, Point) { follow_finger(p.x); } // live drag
void Slider::touch_up(Point p) { follow_finger(p.x); }

// ---- LogView: finger down = look at older lines ----------------------------
void LogView::touch_down(Point p)
{
    drag_start_y = p.y;
    scroll_offset_at_drag_start = scroll_offset;
}

void LogView::touch_move(Point p, Point)
{
    // finger DOWN -> older lines (offset grows); finger UP -> back to live tail
    int lh = line_height();
    int delta_lines = (lh > 0) ? ((p.y - drag_start_y) / lh) : 0;
    set_scroll_offset(scroll_offset_at_drag_start + delta_lines);
}

// ---- FpsCounter: counts frames (moved here from the old update()) ----------
void FpsCounter::tick()
{
    uint32_t now = DWT->CYCCNT;
    uint32_t delta = now - last_cycle;
    last_cycle = now;

    if (delta != 0)
    {
        fps = 480000000 / delta;
        snprintf(buf, sizeof(buf), "FPS:%lu", fps);
        mark_dirty();
    }
}