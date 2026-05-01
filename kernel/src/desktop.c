#include "desktop.h"
#include "app.h"
#include "cbdd.h"
#include "console.h"
#include "cyralithfs.h"
#include "keyboard.h"
#include "memory.h"
#include "network.h"
#include "process.h"
#include "shell.h"
#include "string.h"
#include "task.h"
#include "timer.h"
#include "x32fs_console.h"
#include "user.h"

#include <stddef.h>
#include <stdint.h>

#define DESKTOP_SIDE_WIDTH 20U
#define DESKTOP_CONTENT_WIDTH 57U
#define DESKTOP_BODY_ROWS 18U
#define DESKTOP_NOTICE_MAX 96U
#define DESKTOP_TEXT_MAX 96U

typedef enum {
    DESKTOP_ACTION_WINDOW = 0,
    DESKTOP_ACTION_COMMAND = 1,
    DESKTOP_ACTION_X32FS = 2,
    DESKTOP_ACTION_EXIT = 3
} desktop_action_t;

typedef struct {
    const char* name;
    const char* title;
    const char* subtitle;
    const char* command;
    desktop_action_t action;
} desktop_app_entry_t;

static int g_desktop_active = 0;
static size_t g_desktop_selected = 0U;
static int g_desktop_window_open = 0;
static int g_desktop_focus_window = 0;
static uint32_t g_desktop_last_second = 0xFFFFFFFFU;
static char g_desktop_notice[DESKTOP_NOTICE_MAX] = "Willkommen auf dem Cyralith Desktop.";

static const desktop_app_entry_t g_desktop_apps[] = {
    {"kernelconfig", "Kernelconfig", "Kernel/menuconfig-artige Optionen", "kernelconfig", DESKTOP_ACTION_COMMAND},
    {"files", "Files", "CyralithFS-Dateibereich", "open files", DESKTOP_ACTION_COMMAND},
    {"monitor", "Monitor", "Status, Speicher und Prozesse", "open monitor", DESKTOP_ACTION_COMMAND},
    {"network", "Network", "Hostname, IP und Treiber", "open network", DESKTOP_ACTION_COMMAND},
    {"lumen", "Lumen", "Texteditor fuer notes.txt", "edit notes.txt", DESKTOP_ACTION_COMMAND},
    {"snake", "Snake", "Klassisches Terminal-Spiel", "snake", DESKTOP_ACTION_COMMAND},
    {"browser", "Browser", "Geplanter Web-Bereich", "", DESKTOP_ACTION_WINDOW},
    {"paint", "Paint", "Geplanter Zeichenbereich", "", DESKTOP_ACTION_WINDOW},
    {"x32fs", "x32 FS Console", "Native x32 Command Line fuer CyralithFS", "x32fs", DESKTOP_ACTION_X32FS},
    {"shell", "Shell", "Desktop verlassen", "", DESKTOP_ACTION_EXIT}
};

static size_t desktop_app_count(void) {
    return sizeof(g_desktop_apps) / sizeof(g_desktop_apps[0]);
}

static void copy_limited(char* dst, const char* src, size_t max) {
    size_t i = 0U;
    if (max == 0U) {
        return;
    }
    if (src == (const char*)0) {
        dst[0] = '\0';
        return;
    }
    while (src[i] != '\0' && i + 1U < max) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void append_limited(char* dst, size_t max, const char* src) {
    size_t pos;
    size_t i = 0U;
    if (dst == (char*)0 || max == 0U || src == (const char*)0) {
        return;
    }
    pos = kstrlen(dst);
    if (pos >= max) {
        dst[max - 1U] = '\0';
        return;
    }
    while (src[i] != '\0' && pos + 1U < max) {
        dst[pos++] = src[i++];
    }
    dst[pos] = '\0';
}

static void append_char_limited(char* dst, size_t max, char ch) {
    size_t pos;
    if (dst == (char*)0 || max == 0U) {
        return;
    }
    pos = kstrlen(dst);
    if (pos + 1U >= max) {
        return;
    }
    dst[pos++] = ch;
    dst[pos] = '\0';
}

static void append_uint_limited(char* dst, size_t max, uint32_t value) {
    char tmp[11];
    size_t len = 0U;
    if (value == 0U) {
        append_char_limited(dst, max, '0');
        return;
    }
    while (value > 0U && len < sizeof(tmp)) {
        tmp[len++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (len > 0U) {
        append_char_limited(dst, max, tmp[--len]);
    }
}

static void append_two_digits(char* dst, size_t max, uint32_t value) {
    value %= 100U;
    append_char_limited(dst, max, (char)('0' + (value / 10U)));
    append_char_limited(dst, max, (char)('0' + (value % 10U)));
}

static void write_repeat(char ch, size_t count) {
    size_t i;
    for (i = 0U; i < count; ++i) {
        console_putc(ch);
    }
}

static void write_fixed(const char* text, size_t width) {
    size_t i = 0U;
    if (text != (const char*)0) {
        while (text[i] != '\0' && i < width) {
            console_putc(text[i]);
            i++;
        }
    }
    while (i < width) {
        console_putc(' ');
        i++;
    }
}

static void write_full_border(int newline) {
    console_putc('+');
    write_repeat('-', 78U);
    console_putc('+');
    if (newline != 0) {
        console_putc('\n');
    }
}

static void write_split_border(void) {
    console_putc('+');
    write_repeat('-', DESKTOP_SIDE_WIDTH);
    console_putc('+');
    write_repeat('-', DESKTOP_CONTENT_WIDTH);
    console_putc('+');
    console_putc('\n');
}

static void write_split_row(const char* side, const char* content) {
    console_putc('|');
    write_fixed(side, DESKTOP_SIDE_WIDTH);
    console_putc('|');
    write_fixed(content, DESKTOP_CONTENT_WIDTH);
    console_putc('|');
    console_putc('\n');
}

static uint32_t desktop_uptime_seconds(void) {
    uint32_t frequency = timer_frequency();
    if (frequency == 0U) {
        frequency = 100U;
    }
    return timer_ticks() / frequency;
}

static void format_uptime(char* out, size_t max) {
    uint32_t seconds = desktop_uptime_seconds();
    uint32_t minutes = (seconds / 60U) % 100U;
    uint32_t secs = seconds % 60U;
    copy_limited(out, "Uptime ", max);
    append_two_digits(out, max, minutes);
    append_char_limited(out, max, ':');
    append_two_digits(out, max, secs);
}

static size_t desktop_installed_app_count(void) {
    size_t i;
    size_t installed = 0U;
    for (i = 0U; i < app_count(); ++i) {
        const app_t* app = app_get(i);
        if (app != (const app_t*)0 && app->installed != 0U) {
            installed++;
        }
    }
    return installed;
}

static int desktop_selected_installed(void) {
    const desktop_app_entry_t* entry = &g_desktop_apps[g_desktop_selected];
    const app_t* app = app_find(entry->name);
    if (entry->action == DESKTOP_ACTION_EXIT || entry->action == DESKTOP_ACTION_X32FS) {
        return 1;
    }
    if (app == (const app_t*)0) {
        return 1;
    }
    return app->installed != 0U ? 1 : 0;
}

static void set_notice(const char* text) {
    copy_limited(g_desktop_notice, text, sizeof(g_desktop_notice));
}

static void build_topbar(char* out, size_t max) {
    char uptime[24];
    size_t len;
    copy_limited(out, " Cyralith Desktop Combi-Pack", max);
    len = kstrlen(out);
    while (len < 54U && len + 1U < max) {
        append_char_limited(out, max, ' ');
        len++;
    }
    format_uptime(uptime, sizeof(uptime));
    append_limited(out, max, uptime);
    append_limited(out, max, "  ");
    append_limited(out, max, cbdd_backend_name());
}

static void side_text(size_t row, char* out, size_t max) {
    if (row == 0U) {
        if (g_desktop_focus_window != 0 && g_desktop_window_open != 0) {
            copy_limited(out, " Apps", max);
        } else {
            copy_limited(out, ">Apps", max);
        }
        return;
    }
    if (row == 1U) {
        copy_limited(out, "", max);
        return;
    }
    if (row >= 2U && row < 2U + desktop_app_count()) {
        size_t index = row - 2U;
        const desktop_app_entry_t* entry = &g_desktop_apps[index];
        const app_t* app = app_find(entry->name);
        out[0] = '\0';
        append_char_limited(out, max, index == g_desktop_selected ? '>' : ' ');
        append_char_limited(out, max, ' ');
        append_limited(out, max, entry->title);
        if (app != (const app_t*)0 && app->installed == 0U) {
            append_limited(out, max, " *");
        }
        return;
    }
    if (row == 13U) {
        copy_limited(out, "", max);
        return;
    }
    if (row == 14U) {
        copy_limited(out, "* nicht install.", max);
        return;
    }
    if (row == 15U) {
        copy_limited(out, "", max);
        return;
    }
    if (row == 16U) {
        copy_limited(out, "Enter Fenster", max);
        return;
    }
    if (row == 17U) {
        copy_limited(out, "O Direktstart", max);
        return;
    }
    copy_limited(out, "", max);
}

static void dashboard_text(size_t row, char* out, size_t max) {
    char tmp[32];
    const user_t* user = user_current();

    switch (row) {
        case 0U:
            copy_limited(out, "Willkommen, ", max);
            append_limited(out, max, user != (const user_t*)0 ? user->display_name : "User");
            append_limited(out, max, ".");
            break;
        case 1U:
            copy_limited(out, "Combi-Pack: Fenster + App-Framework + Taskbar.", max);
            break;
        case 2U:
            copy_limited(out, "", max);
            break;
        case 3U:
            copy_limited(out, "Systemkarten", max);
            break;
        case 4U:
            copy_limited(out, " Kernel: Cyralith | Desktop: Textmode WM", max);
            break;
        case 5U:
            copy_limited(out, " User: ", max);
            append_limited(out, max, user != (const user_t*)0 ? user->username : "unknown");
            append_limited(out, max, " | Rolle: ");
            append_limited(out, max, user != (const user_t*)0 ? user_role_name(user->role) : "-");
            break;
        case 6U:
            copy_limited(out, " Speicher frei: ", max);
            append_uint_limited(out, max, (uint32_t)(kmem_free_bytes() / 1024U));
            append_limited(out, max, " KiB");
            break;
        case 7U:
            copy_limited(out, " Prozesse: ", max);
            append_uint_limited(out, max, (uint32_t)process_count());
            append_limited(out, max, " | Tasks: ");
            append_uint_limited(out, max, (uint32_t)task_count());
            break;
        case 8U:
            copy_limited(out, " FS: ", max);
            append_limited(out, max, afs_name());
            append_limited(out, max, " | Nodes: ");
            append_uint_limited(out, max, (uint32_t)afs_node_count());
            break;
        case 9U:
            copy_limited(out, " Net: ", max);
            append_limited(out, max, network_hostname());
            append_limited(out, max, " / ");
            append_limited(out, max, network_ip());
            break;
        case 10U:
            copy_limited(out, " Apps installiert: ", max);
            append_uint_limited(out, max, (uint32_t)desktop_installed_app_count());
            append_limited(out, max, "/");
            append_uint_limited(out, max, (uint32_t)app_count());
            break;
        case 11U:
            copy_limited(out, " Display: ", max);
            append_limited(out, max, cbdd_mode_name());
            break;
        case 12U:
            format_uptime(tmp, sizeof(tmp));
            copy_limited(out, "Mini-Taskbar aktiv: ", max);
            append_limited(out, max, tmp);
            break;
        case 13U:
            copy_limited(out, "Enter oeffnet ein Desktop-Fenster.", max);
            break;
        case 14U:
            copy_limited(out, "O startet die passende Shell-App direkt.", max);
            break;
        case 15U:
            copy_limited(out, "Q/Ctrl+Q verlaesst den Desktop.", max);
            break;
        case 16U:
            copy_limited(out, "Naechster Schritt: Maus + Framebuffer.", max);
            break;
        default:
            copy_limited(out, "", max);
            break;
    }
}

static void app_info_line(const desktop_app_entry_t* entry, size_t inner_row, char* out, size_t max) {
    const app_t* app = app_find(entry->name);
    char pwd[64];

    switch (inner_row) {
        case 0U:
            copy_limited(out, entry->subtitle, max);
            break;
        case 1U:
            copy_limited(out, "", max);
            break;
        case 2U:
            copy_limited(out, "Status: ", max);
            if (entry->action == DESKTOP_ACTION_EXIT) {
                append_limited(out, max, "Desktop-Systemeintrag");
            } else if (app == (const app_t*)0) {
                append_limited(out, max, "Desktop-intern");
            } else {
                append_limited(out, max, app->installed != 0U ? "installiert" : "nicht installiert");
                append_limited(out, max, app->builtin != 0U ? " | builtin" : " | optional");
            }
            break;
        case 3U:
            copy_limited(out, "Befehl: ", max);
            append_limited(out, max, entry->command[0] != '\0' ? entry->command : "kein Direktbefehl");
            break;
        case 4U:
            copy_limited(out, "", max);
            break;
        case 5U:
            if (kstrcmp(entry->name, "kernelconfig") == 0) {
                copy_limited(out, "Kernelconfig: Sprache, Netzwerk, Security, CBDD.", max);
            } else if (kstrcmp(entry->name, "files") == 0) {
                afs_pwd(pwd, sizeof(pwd));
                copy_limited(out, "Aktueller Pfad: ", max);
                append_limited(out, max, pwd);
            } else if (kstrcmp(entry->name, "monitor") == 0) {
                copy_limited(out, "Freier Speicher: ", max);
                append_uint_limited(out, max, (uint32_t)(kmem_free_bytes() / 1024U));
                append_limited(out, max, " KiB");
            } else if (kstrcmp(entry->name, "network") == 0) {
                copy_limited(out, "Hostname: ", max);
                append_limited(out, max, network_hostname());
            } else if (kstrcmp(entry->name, "lumen") == 0) {
                copy_limited(out, "Lumen startet mit notes.txt als schneller Notiz.", max);
            } else if (kstrcmp(entry->name, "snake") == 0) {
                copy_limited(out, "Snake uebernimmt kurz den Bildschirm.", max);
            } else if (kstrcmp(entry->name, "browser") == 0) {
                copy_limited(out, "Browser ist geplant; Netzstack ist die Grundlage.", max);
            } else if (kstrcmp(entry->name, "paint") == 0) {
                copy_limited(out, "Paint wartet auf Maus/Framebuffer-Unterstuetzung.", max);
            } else if (kstrcmp(entry->name, "x32fs") == 0) {
                copy_limited(out, "Native FS-Konsole: direkt an CyralithFS angebunden.", max);
            } else {
                copy_limited(out, "Shell bringt dich zurueck zur Kommandozeile.", max);
            }
            break;
        case 6U:
            if (kstrcmp(entry->name, "network") == 0) {
                copy_limited(out, "IP: ", max);
                append_limited(out, max, network_ip());
                append_limited(out, max, " | Treiber: ");
                append_limited(out, max, network_driver_name());
            } else if (kstrcmp(entry->name, "files") == 0) {
                copy_limited(out, "Nodes im CyralithFS: ", max);
                append_uint_limited(out, max, (uint32_t)afs_node_count());
            } else if (kstrcmp(entry->name, "monitor") == 0) {
                copy_limited(out, "Prozesse: ", max);
                append_uint_limited(out, max, (uint32_t)process_count());
                append_limited(out, max, " | Scheduler: ");
                append_limited(out, max, task_scheduler_name());
            } else {
                copy_limited(out, "", max);
            }
            break;
        case 7U:
            copy_limited(out, "", max);
            break;
        case 8U:
            copy_limited(out, "Tasten im Fenster:", max);
            break;
        case 9U:
            copy_limited(out, "  O / Enter  Direkt starten", max);
            break;
        case 10U:
            copy_limited(out, "  Q / Esc    Fenster schliessen", max);
            break;
        case 11U:
            copy_limited(out, "  Tab        Fokus wechseln", max);
            break;
        default:
            copy_limited(out, "", max);
            break;
    }
}

static void window_text(size_t row, char* out, size_t max) {
    const desktop_app_entry_t* entry = &g_desktop_apps[g_desktop_selected];
    const size_t inner_width = DESKTOP_CONTENT_WIDTH - 4U;

    if (row == 0U) {
        out[0] = '\0';
        append_limited(out, max, "+");
        append_limited(out, max, g_desktop_focus_window != 0 ? "[" : " ");
        append_limited(out, max, entry->title);
        append_limited(out, max, g_desktop_focus_window != 0 ? "]" : " ");
        while (kstrlen(out) < DESKTOP_CONTENT_WIDTH - 1U) {
            append_char_limited(out, max, '-');
        }
        append_limited(out, max, "+");
        return;
    }
    if (row == DESKTOP_BODY_ROWS - 1U) {
        copy_limited(out, "+", max);
        while (kstrlen(out) < DESKTOP_CONTENT_WIDTH - 1U) {
            append_char_limited(out, max, '-');
        }
        append_limited(out, max, "+");
        return;
    }

    {
        char inner[DESKTOP_TEXT_MAX];
        app_info_line(entry, row - 1U, inner, sizeof(inner));
        copy_limited(out, "| ", max);
        append_limited(out, max, inner);
        while (kstrlen(out) < inner_width + 2U) {
            append_char_limited(out, max, ' ');
        }
        append_limited(out, max, " |");
    }
}

static void content_text(size_t row, char* out, size_t max) {
    if (g_desktop_window_open != 0) {
        window_text(row, out, max);
        return;
    }
    dashboard_text(row, out, max);
}

static void draw_desktop(void) {
    size_t row;
    char topbar[DESKTOP_TEXT_MAX];
    char side[DESKTOP_TEXT_MAX];
    char content[DESKTOP_TEXT_MAX];
    char help[DESKTOP_TEXT_MAX];

    console_set_color(15U, 0U);
    console_clear();

    write_full_border(1);
    build_topbar(topbar, sizeof(topbar));
    console_putc('|');
    write_fixed(topbar, 78U);
    console_putc('|');
    console_putc('\n');
    write_split_border();

    for (row = 0U; row < DESKTOP_BODY_ROWS; ++row) {
        side_text(row, side, sizeof(side));
        content_text(row, content, sizeof(content));
        write_split_row(side, content);
    }

    write_split_border();
    copy_limited(help, " Pfeile waehlen | Enter Fenster/Start | O direkt | Tab Fokus | Q Shell", sizeof(help));
    console_putc('|');
    write_fixed(help, 78U);
    console_putc('|');
    console_putc('\n');
    console_putc('|');
    write_fixed(g_desktop_notice, 78U);
    console_putc('|');
    console_putc('\n');
    write_full_border(0);
}

static void desktop_open_window(void) {
    const desktop_app_entry_t* entry = &g_desktop_apps[g_desktop_selected];
    if (entry->action == DESKTOP_ACTION_EXIT) {
        g_desktop_active = 0;
        console_clear();
        console_writeln("Desktop beendet.");
        return;
    }
    g_desktop_window_open = 1;
    g_desktop_focus_window = 1;
    set_notice("Fenster geoeffnet. O startet die App, Q schliesst nur das Fenster.");
    draw_desktop();
}

static void desktop_direct_launch(void) {
    const desktop_app_entry_t* entry = &g_desktop_apps[g_desktop_selected];

    if (entry->action == DESKTOP_ACTION_EXIT) {
        g_desktop_active = 0;
        console_clear();
        console_writeln("Desktop beendet.");
        return;
    }

    if (entry->action == DESKTOP_ACTION_X32FS) {
        g_desktop_active = 0;
        g_desktop_window_open = 0;
        g_desktop_focus_window = 0;
        x32fs_console_open();
        return;
    }

    if (entry->command[0] == '\0') {
        set_notice("Diese App ist ein Platzhalter und hat noch keinen Direktstart.");
        g_desktop_window_open = 1;
        g_desktop_focus_window = 1;
        draw_desktop();
        return;
    }

    if (desktop_selected_installed() == 0) {
        set_notice("App ist nicht installiert. Nutze die App-Verwaltung oder installiere sie spaeter.");
        g_desktop_window_open = 1;
        g_desktop_focus_window = 1;
        draw_desktop();
        return;
    }

    g_desktop_active = 0;
    g_desktop_window_open = 0;
    g_desktop_focus_window = 0;
    console_clear();
    console_write("Desktop startet: ");
    console_writeln(entry->title);
    shell_run_command(entry->command);
}

int desktop_is_active(void) {
    return g_desktop_active;
}

void desktop_open(void) {
    g_desktop_active = 1;
    g_desktop_selected = 0U;
    g_desktop_window_open = 0;
    g_desktop_focus_window = 0;
    g_desktop_last_second = 0xFFFFFFFFU;
    set_notice("Combi-Pack bereit: Fenster, App-Framework und Uhr laufen.");
    draw_desktop();
}

void desktop_handle_key(int key) {
    if (g_desktop_active == 0) {
        return;
    }

    if (key == KEY_CTRL_Q || key == KEY_CTRL_C || key == 'q' || key == 'Q') {
        if (g_desktop_window_open != 0 && g_desktop_focus_window != 0) {
            g_desktop_window_open = 0;
            g_desktop_focus_window = 0;
            set_notice("Fenster geschlossen. Desktop laeuft weiter.");
            draw_desktop();
            return;
        }
        g_desktop_active = 0;
        console_clear();
        console_writeln("Desktop beendet.");
        return;
    }

    if (key == 27) {
        if (g_desktop_window_open != 0) {
            g_desktop_window_open = 0;
            g_desktop_focus_window = 0;
            set_notice("Fenster geschlossen.");
            draw_desktop();
            return;
        }
        g_desktop_active = 0;
        console_clear();
        console_writeln("Desktop beendet.");
        return;
    }

    if (key == '\t') {
        if (g_desktop_window_open != 0) {
            g_desktop_focus_window = g_desktop_focus_window == 0 ? 1 : 0;
            set_notice(g_desktop_focus_window != 0 ? "Fokus: Fenster." : "Fokus: App-Liste.");
            draw_desktop();
        }
        return;
    }

    if (key == 'o' || key == 'O') {
        desktop_direct_launch();
        return;
    }

    if (key == KEY_ENTER) {
        if (g_desktop_window_open != 0 && g_desktop_focus_window != 0) {
            desktop_direct_launch();
            return;
        }
        desktop_open_window();
        return;
    }

    if (key == KEY_UP) {
        if (g_desktop_focus_window == 0) {
            if (g_desktop_selected == 0U) {
                g_desktop_selected = desktop_app_count() - 1U;
            } else {
                g_desktop_selected--;
            }
            if (g_desktop_window_open != 0) {
                set_notice("Fensterinhalt folgt der ausgewaehlten App.");
            }
            draw_desktop();
        }
        return;
    }

    if (key == KEY_DOWN) {
        if (g_desktop_focus_window == 0) {
            g_desktop_selected = (g_desktop_selected + 1U) % desktop_app_count();
            if (g_desktop_window_open != 0) {
                set_notice("Fensterinhalt folgt der ausgewaehlten App.");
            }
            draw_desktop();
        }
        return;
    }

    if (key == KEY_LEFT || key == KEY_RIGHT) {
        if (g_desktop_window_open != 0) {
            g_desktop_focus_window = g_desktop_focus_window == 0 ? 1 : 0;
            set_notice(g_desktop_focus_window != 0 ? "Fokus: Fenster." : "Fokus: App-Liste.");
            draw_desktop();
        }
        return;
    }
}

void desktop_poll(void) {
    uint32_t seconds;
    if (g_desktop_active == 0) {
        return;
    }
    seconds = desktop_uptime_seconds();
    if (seconds != g_desktop_last_second) {
        g_desktop_last_second = seconds;
        draw_desktop();
    }
}
