#include <furi.h>
#include <gui/gui.h>

#include <stdio.h>

#define TAG "MagicLife"

#define STANDARD_STARTING_LIFE 20
#define COMMANDER_STARTING_LIFE 40
#define COMMANDER_PLAYERS 4
#define LIFE_MIN (-99)
#define LIFE_MAX 999

typedef enum {
    ScreenMenu,
    ScreenStandard,
    ScreenCommander,
} Screen;

typedef enum {
    MenuStandard = 0,
    MenuCommander = 1,
    MenuCount,
} MenuSelection;

typedef enum {
    CommanderSelect,
    CommanderAdjust,
} CommanderMode;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    Screen screen;
    uint8_t menu_selection;
    int32_t life_p1;
    int32_t life_p2;
    int32_t commander_life[COMMANDER_PLAYERS];
    uint8_t commander_cursor;
    CommanderMode commander_mode;
} MagicLifeApp;

static int32_t clamp_life(int32_t life) {
    if(life < LIFE_MIN) return LIFE_MIN;
    if(life > LIFE_MAX) return LIFE_MAX;
    return life;
}

static void magic_life_draw_menu(Canvas* canvas, MagicLifeApp* app) {
    static const char* items[MenuCount] = {"Standard", "Commander"};

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "MTG Life Tracker");
    canvas_draw_line(canvas, 0, 13, 128, 13);

    for(uint8_t i = 0; i < MenuCount; i++) {
        uint8_t row_y = 20 + i * 14;
        if(i == app->menu_selection) {
            canvas_draw_box(canvas, 24, row_y - 1, 80, 12);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_str_aligned(canvas, 64, row_y + 5, AlignCenter, AlignCenter, items[i]);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_draw_str_aligned(canvas, 64, row_y + 5, AlignCenter, AlignCenter, items[i]);
        }
    }

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "Up/Down + OK to start");
}

static void magic_life_draw_standard(Canvas* canvas, MagicLifeApp* app) {
    char buf[16];

    canvas_clear(canvas);

    // Player 1: adjusted with LEFT / RIGHT.
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 9, "P1");
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 22, 15, AlignCenter, AlignCenter, "<");
    canvas_draw_str_aligned(canvas, 106, 15, AlignCenter, AlignCenter, ">");
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(buf, sizeof(buf), "%ld", (long)app->life_p1);
    canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, buf);

    canvas_draw_line(canvas, 0, 29, 128, 29);

    // Player 2: adjusted with UP / DOWN.
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 38, "P2");
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 22, 43, AlignCenter, AlignCenter, "^");
    canvas_draw_str_aligned(canvas, 106, 43, AlignCenter, AlignCenter, "v");
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(buf, sizeof(buf), "%ld", (long)app->life_p2);
    canvas_draw_str_aligned(canvas, 64, 43, AlignCenter, AlignCenter, buf);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, "OK:reset Back:menu");
}

static void magic_life_commander_reset(MagicLifeApp* app) {
    for(uint8_t i = 0; i < COMMANDER_PLAYERS; i++) {
        app->commander_life[i] = COMMANDER_STARTING_LIFE;
    }
    app->commander_cursor = 0;
    app->commander_mode = CommanderSelect;
}

static void magic_life_draw_commander(Canvas* canvas, MagicLifeApp* app) {
    static const char* labels[COMMANDER_PLAYERS] = {"P1", "P2", "P3", "P4"};
    char buf[16];

    canvas_clear(canvas);

    // 2x2 grid dividers (grid occupies y 0..55, footer below).
    canvas_draw_line(canvas, 64, 0, 64, 55);
    canvas_draw_line(canvas, 0, 28, 128, 28);

    for(uint8_t i = 0; i < COMMANDER_PLAYERS; i++) {
        int32_t x0 = (i % 2) * 64;
        int32_t y0 = (i / 2) * 28;
        int32_t cx = x0 + 32;
        bool selected = (i == app->commander_cursor);

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, x0 + 3, y0 + 9, labels[i]);

        canvas_set_font(canvas, FontBigNumbers);
        snprintf(buf, sizeof(buf), "%ld", (long)app->commander_life[i]);
        int32_t baseline = y0 + 25;

        if(selected) {
            canvas_draw_frame(canvas, x0 + 1, y0 + 1, 62, 26);
        }

        if(selected && app->commander_mode == CommanderAdjust) {
            uint16_t w = canvas_string_width(canvas, buf);
            canvas_draw_box(canvas, cx - w / 2 - 2, baseline - 16, w + 4, 17);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_str_aligned(canvas, cx, baseline, AlignCenter, AlignBottom, buf);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_draw_str_aligned(canvas, cx, baseline, AlignCenter, AlignBottom, buf);
        }
    }

    canvas_set_font(canvas, FontSecondary);
    if(app->commander_mode == CommanderAdjust) {
        canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, "Up/Dn:life OK:done");
    } else {
        canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, "Move:arrows OK:edit");
    }
}

static void magic_life_draw_callback(Canvas* canvas, void* context) {
    MagicLifeApp* app = context;

    switch(app->screen) {
    case ScreenStandard:
        magic_life_draw_standard(canvas, app);
        break;
    case ScreenCommander:
        magic_life_draw_commander(canvas, app);
        break;
    case ScreenMenu:
    default:
        magic_life_draw_menu(canvas, app);
        break;
    }
}

static void magic_life_input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void magic_life_handle_menu_input(MagicLifeApp* app, InputEvent* event, bool* running) {
    if(event->type != InputTypeShort) return;

    switch(event->key) {
    case InputKeyBack:
        *running = false;
        break;
    case InputKeyUp:
        app->menu_selection = (app->menu_selection + MenuCount - 1) % MenuCount;
        break;
    case InputKeyDown:
        app->menu_selection = (app->menu_selection + 1) % MenuCount;
        break;
    case InputKeyOk:
        if(app->menu_selection == MenuStandard) {
            app->life_p1 = STANDARD_STARTING_LIFE;
            app->life_p2 = STANDARD_STARTING_LIFE;
            app->screen = ScreenStandard;
        } else {
            magic_life_commander_reset(app);
            app->screen = ScreenCommander;
        }
        break;
    default:
        break;
    }
}

static void magic_life_handle_standard_input(MagicLifeApp* app, InputEvent* event) {
    // Press + Repeat so holding a direction keeps adjusting life.
    if(event->type == InputTypePress || event->type == InputTypeRepeat) {
        switch(event->key) {
        case InputKeyLeft:
            app->life_p1 = clamp_life(app->life_p1 - 1);
            break;
        case InputKeyRight:
            app->life_p1 = clamp_life(app->life_p1 + 1);
            break;
        case InputKeyUp:
            app->life_p2 = clamp_life(app->life_p2 + 1);
            break;
        case InputKeyDown:
            app->life_p2 = clamp_life(app->life_p2 - 1);
            break;
        default:
            break;
        }
        return;
    }

    if(event->type != InputTypeShort) return;

    switch(event->key) {
    case InputKeyBack:
        app->screen = ScreenMenu;
        break;
    case InputKeyOk:
        app->life_p1 = STANDARD_STARTING_LIFE;
        app->life_p2 = STANDARD_STARTING_LIFE;
        break;
    default:
        break;
    }
}

static void magic_life_handle_commander_input(MagicLifeApp* app, InputEvent* event) {
    if(app->commander_mode == CommanderAdjust) {
        // Press + Repeat so holding up/down keeps adjusting life.
        if(event->type == InputTypePress || event->type == InputTypeRepeat) {
            if(event->key == InputKeyUp) {
                int32_t* life = &app->commander_life[app->commander_cursor];
                *life = clamp_life(*life + 1);
            } else if(event->key == InputKeyDown) {
                int32_t* life = &app->commander_life[app->commander_cursor];
                *life = clamp_life(*life - 1);
            }
            return;
        }

        if(event->type != InputTypeShort) return;

        if(event->key == InputKeyOk || event->key == InputKeyBack) {
            app->commander_mode = CommanderSelect;
        }
        return;
    }

    // CommanderSelect: arrows move the cursor (act on press so holding
    // a direction doesn't skip around the grid).
    if(event->type == InputTypePress) {
        switch(event->key) {
        case InputKeyLeft:
            app->commander_cursor &= (uint8_t)~1u;
            break;
        case InputKeyRight:
            app->commander_cursor |= 1u;
            break;
        case InputKeyUp:
            app->commander_cursor &= (uint8_t)~2u;
            break;
        case InputKeyDown:
            app->commander_cursor |= 2u;
            break;
        default:
            break;
        }
        return;
    }

    if(event->type != InputTypeShort) return;

    switch(event->key) {
    case InputKeyBack:
        app->screen = ScreenMenu;
        break;
    case InputKeyOk:
        app->commander_mode = CommanderAdjust;
        break;
    default:
        break;
    }
}

int32_t magic_life_app(void* p) {
    UNUSED(p);

    MagicLifeApp app = {
        .screen = ScreenMenu,
        .menu_selection = MenuStandard,
        .life_p1 = STANDARD_STARTING_LIFE,
        .life_p2 = STANDARD_STARTING_LIFE,
        .commander_life =
            {COMMANDER_STARTING_LIFE,
             COMMANDER_STARTING_LIFE,
             COMMANDER_STARTING_LIFE,
             COMMANDER_STARTING_LIFE},
        .commander_cursor = 0,
        .commander_mode = CommanderSelect,
    };

    app.gui = furi_record_open(RECORD_GUI);
    app.view_port = view_port_alloc();
    app.event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    view_port_draw_callback_set(app.view_port, magic_life_draw_callback, &app);
    view_port_input_callback_set(app.view_port, magic_life_input_callback, app.event_queue);
    gui_add_view_port(app.gui, app.view_port, GuiLayerFullscreen);

    FURI_LOG_I(TAG, "MTG Life Tracker started");

    bool running = true;
    while(running) {
        InputEvent event;
        furi_check(furi_message_queue_get(app.event_queue, &event, FuriWaitForever) == FuriStatusOk);

        switch(app.screen) {
        case ScreenStandard:
            magic_life_handle_standard_input(&app, &event);
            break;
        case ScreenCommander:
            magic_life_handle_commander_input(&app, &event);
            break;
        case ScreenMenu:
        default:
            magic_life_handle_menu_input(&app, &event, &running);
            break;
        }

        view_port_update(app.view_port);
    }

    gui_remove_view_port(app.gui, app.view_port);
    view_port_free(app.view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(app.event_queue);

    return 0;
}
