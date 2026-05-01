#ifndef X32FS_CONSOLE_H
#define X32FS_CONSOLE_H

void x32fs_console_open(void);
int x32fs_console_is_active(void);
void x32fs_console_handle_key(int key);
void x32fs_console_poll(void);

#endif
