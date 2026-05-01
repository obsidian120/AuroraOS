#include "x32fs_console.h"
#include "console.h"
#include "cyralithfs.h"
#include "keyboard.h"
#include "string.h"
#include "user.h"

#include <stddef.h>

#define X32FS_INPUT_MAX 128
#define X32FS_ARG_MAX 64

static int g_x32fs_active = 0;
static char g_x32fs_input[X32FS_INPUT_MAX];
static size_t g_x32fs_input_len = 0U;

static void x32fs_prompt(void) {
    char pwd[80];
    afs_pwd(pwd, sizeof(pwd));
    console_write("x32fs:");
    console_write(pwd);
    console_write("> ");
}

static const char* x32fs_skip_spaces(const char* s) {
    while (*s == ' ') { s++; }
    return s;
}

static int x32fs_split_name_text(const char* input, char* name, size_t name_max, const char** text) {
    size_t i = 0U;
    input = x32fs_skip_spaces(input);
    if (*input == '\0') { return -1; }
    while (input[i] != '\0' && input[i] != ' ' && i + 1U < name_max) {
        name[i] = input[i];
        i++;
    }
    name[i] = '\0';
    input += i;
    input = x32fs_skip_spaces(input);
    if (name[0] == '\0' || *input == '\0') { return -1; }
    *text = input;
    return 0;
}

static void x32fs_result(int rc, const char* ok, const char* fail) {
    if (rc == AFS_OK) {
        console_writeln(ok);
        return;
    }
    console_write(fail);
    console_write(" rc=");
    console_write_dec((unsigned int)rc);
    console_putc('\n');
}

static void x32fs_help(void) {
    console_writeln("Native x32 Command Line fuer CyralithFS");
    console_writeln("  help                 - Hilfe anzeigen");
    console_writeln("  pwd                  - aktuellen FS-Pfad anzeigen");
    console_writeln("  ls [pfad]            - Nodes auflisten");
    console_writeln("  cd <pfad>            - Ordner wechseln");
    console_writeln("  stat <pfad>          - Metadaten anzeigen");
    console_writeln("  cat <datei>          - Datei roh lesen");
    console_writeln("  mkdir/touch <pfad>   - Ordner oder Datei erstellen");
    console_writeln("  write <datei> <text> - Datei ueberschreiben");
    console_writeln("  append <datei> <txt> - Text anhaengen");
    console_writeln("  rm <pfad>            - Datei entfernen");
    console_writeln("  rmdir <pfad>         - leeren Ordner entfernen");
    console_writeln("  save/load            - nur CyralithFS persistieren/laden");
    console_writeln("  nodes                - Node-Zaehler");
    console_writeln("  clear                - Anzeige leeren");
    console_writeln("  exit                 - x32FS Console verlassen");
}

static void x32fs_cat(const char* path) {
    char data[1024];
    int rc = afs_read_file(path, data, sizeof(data));
    if (rc == AFS_OK) {
        console_writeln(data);
        return;
    }
    x32fs_result(rc, "OK", "cat fehlgeschlagen");
}

static void x32fs_stat(const char* path) {
    char info[256];
    int rc = afs_stat(path, info, sizeof(info));
    if (rc == AFS_OK) {
        console_writeln(info);
        return;
    }
    x32fs_result(rc, "OK", "stat fehlgeschlagen");
}

static void x32fs_execute(const char* raw) {
    const char* args;
    char name[X32FS_ARG_MAX];
    const char* text;

    raw = x32fs_skip_spaces(raw);
    if (*raw == '\0') { return; }

    if (kstrcmp(raw, "help") == 0 || kstrcmp(raw, "?") == 0) { x32fs_help(); return; }
    if (kstrcmp(raw, "exit") == 0 || kstrcmp(raw, "quit") == 0 || kstrcmp(raw, "q") == 0) {
        g_x32fs_active = 0;
        console_clear();
        console_writeln("x32FS Console beendet.");
        return;
    }
    if (kstrcmp(raw, "clear") == 0) {
        console_clear();
        console_writeln("Cyralith x32FS Console");
        return;
    }
    if (kstrcmp(raw, "pwd") == 0) {
        char pwd[80];
        afs_pwd(pwd, sizeof(pwd));
        console_writeln(pwd);
        return;
    }
    if (kstrcmp(raw, "nodes") == 0) {
        console_write("CyralithFS Nodes: ");
        console_write_dec((unsigned int)afs_node_count());
        console_putc('\n');
        return;
    }
    if (kstrcmp(raw, "ls") == 0) { afs_ls(""); return; }
    if (kstarts_with(raw, "ls ")) { afs_ls(x32fs_skip_spaces(raw + 3)); return; }
    if (kstarts_with(raw, "cd ")) { x32fs_result(afs_cd(x32fs_skip_spaces(raw + 3)), "Pfad gewechselt.", "cd fehlgeschlagen"); return; }
    if (kstarts_with(raw, "mkdir ")) { x32fs_result(afs_mkdir(x32fs_skip_spaces(raw + 6)), "Ordner erstellt.", "mkdir fehlgeschlagen"); return; }
    if (kstarts_with(raw, "touch ")) { x32fs_result(afs_touch(x32fs_skip_spaces(raw + 6)), "Datei erstellt.", "touch fehlgeschlagen"); return; }
    if (kstarts_with(raw, "cat ")) { x32fs_cat(x32fs_skip_spaces(raw + 4)); return; }
    if (kstarts_with(raw, "stat ")) { x32fs_stat(x32fs_skip_spaces(raw + 5)); return; }
    if (kstarts_with(raw, "rm ")) { x32fs_result(afs_rm(x32fs_skip_spaces(raw + 3)), "Datei entfernt.", "rm fehlgeschlagen"); return; }
    if (kstarts_with(raw, "rmdir ")) { x32fs_result(afs_rmdir(x32fs_skip_spaces(raw + 6), 0), "Ordner entfernt.", "rmdir fehlgeschlagen"); return; }
    if (kstarts_with(raw, "write ")) {
        if (x32fs_split_name_text(raw + 6, name, sizeof(name), &text) != 0) { console_writeln("Nutze: write <datei> <text>"); return; }
        x32fs_result(afs_write_file(name, text), "Datei geschrieben.", "write fehlgeschlagen");
        return;
    }
    if (kstarts_with(raw, "append ")) {
        if (x32fs_split_name_text(raw + 7, name, sizeof(name), &text) != 0) { console_writeln("Nutze: append <datei> <text>"); return; }
        x32fs_result(afs_append_file(name, text), "Text angehaengt.", "append fehlgeschlagen");
        return;
    }
    if (kstarts_with(raw, "find ")) { afs_find("", x32fs_skip_spaces(raw + 5)); return; }
    if (kstrcmp(raw, "save") == 0 || kstrcmp(raw, "savefs") == 0) { x32fs_result(afs_save_persistent(), "CyralithFS gespeichert.", "save fehlgeschlagen"); return; }
    if (kstrcmp(raw, "load") == 0 || kstrcmp(raw, "loadfs") == 0) { x32fs_result(afs_load_persistent(), "CyralithFS geladen.", "load fehlgeschlagen"); return; }
    if (kstarts_with(raw, "protect ")) {
        args = x32fs_skip_spaces(raw + 8);
        if (x32fs_split_name_text(args, name, sizeof(name), &text) != 0) { console_writeln("Nutze: protect <pfad> <private|team|public|shared>"); return; }
        x32fs_result(afs_protect(name, text), "Rechte gesetzt.", "protect fehlgeschlagen");
        return;
    }

    console_writeln("Unbekannter x32FS-Befehl. Nutze 'help'.");
}

void x32fs_console_open(void) {
    g_x32fs_active = 1;
    g_x32fs_input_len = 0U;
    g_x32fs_input[0] = '\0';
    console_clear();
    console_writeln("Cyralith x32FS Console");
    console_writeln("Native x32 Command Line fuer CyralithFS. 'help' zeigt Befehle.");
    console_writeln("Diese Konsole nutzt das FS direkt und ist nicht die normale Shell.");
    console_writeln("");
    x32fs_prompt();
}

int x32fs_console_is_active(void) {
    return g_x32fs_active;
}

void x32fs_console_handle_key(int key) {
    if (g_x32fs_active == 0) { return; }
    if (key == KEY_CTRL_C || key == KEY_CTRL_Q) {
        g_x32fs_active = 0;
        console_clear();
        console_writeln("x32FS Console beendet.");
        return;
    }
    if (key == KEY_BACKSPACE) {
        if (g_x32fs_input_len > 0U) {
            g_x32fs_input_len--;
            g_x32fs_input[g_x32fs_input_len] = '\0';
            console_backspace();
        }
        return;
    }
    if (key == KEY_ENTER) {
        g_x32fs_input[g_x32fs_input_len] = '\0';
        console_putc('\n');
        x32fs_execute(g_x32fs_input);
        g_x32fs_input_len = 0U;
        g_x32fs_input[0] = '\0';
        if (g_x32fs_active != 0) { x32fs_prompt(); }
        return;
    }
    if (key > 0 && key < 256 && g_x32fs_input_len + 1U < X32FS_INPUT_MAX) {
        g_x32fs_input[g_x32fs_input_len++] = (char)key;
        g_x32fs_input[g_x32fs_input_len] = '\0';
        console_putc((char)key);
    }
}

void x32fs_console_poll(void) {
    (void)g_x32fs_active;
}
