#ifndef CBDD_H
#define CBDD_H

#include <stdint.h>
#include <stddef.h>

#define CBDD_FLAG_TEXT_MODE      0x00000001U
#define CBDD_FLAG_VGA_COMPAT     0x00000002U
#define CBDD_FLAG_FRAMEBUFFER    0x00000004U
#define CBDD_FLAG_SAFE_MODE      0x00000008U
#define CBDD_FLAG_DEBUG_OVERLAY  0x00000010U
#define CBDD_FLAG_HIGH_CONTRAST  0x00000020U
#define CBDD_FLAG_CURSOR_VISIBLE 0x00000040U

typedef enum {
    CBDD_BACKEND_VGA_TEXT = 0,
    CBDD_BACKEND_FRAMEBUFFER_PREVIEW = 1
} cbdd_backend_t;

typedef enum {
    CBDD_MODE_TEXT_80X25 = 0,
    CBDD_MODE_TEXT_80X50_PREVIEW = 1,
    CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW = 2
} cbdd_mode_t;

typedef struct {
    const char* driver_name;
    const char* backend_name;
    const char* mode_name;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t flags;
    int initialized;
    int enabled;
    int debug_overlay;
    int high_contrast;
    int cursor_visible;
} cbdd_info_t;

void cbdd_init(void);
void cbdd_reset_safe(void);
int cbdd_probe(void);

int cbdd_is_enabled(void);
void cbdd_set_enabled(int enabled);

cbdd_backend_t cbdd_backend(void);
void cbdd_cycle_backend(void);

cbdd_mode_t cbdd_mode(void);
void cbdd_cycle_mode(void);

void cbdd_toggle_debug_overlay(void);
void cbdd_toggle_high_contrast(void);
void cbdd_toggle_cursor(void);

const char* cbdd_driver_name(void);
const char* cbdd_backend_name(void);
const char* cbdd_mode_name(void);
uint32_t cbdd_width(void);
uint32_t cbdd_height(void);
uint32_t cbdd_bpp(void);
uint32_t cbdd_flags(void);

void cbdd_get_info(cbdd_info_t* out);
int cbdd_save_persistent(void);
int cbdd_load_persistent(void);

#endif
