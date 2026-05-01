#include "cbdd.h"
#include "storage.h"

#define CBDD_PERSIST_MAGIC 0x44444243U /* CBDD */
#define CBDD_PERSIST_VERSION 1U
#define CBDD_PERSIST_LBA 48U
#define CBDD_PERSIST_SECTORS 1U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;
    uint32_t reserved;
} cbdd_snapshot_header_t;

typedef struct {
    uint32_t enabled;
    uint32_t backend;
    uint32_t mode;
    uint32_t debug_overlay;
    uint32_t high_contrast;
    uint32_t cursor_visible;
    uint32_t reserved[10];
} cbdd_snapshot_payload_t;

static int g_cbdd_initialized = 0;
static int g_cbdd_enabled = 1;
static cbdd_backend_t g_cbdd_backend = CBDD_BACKEND_VGA_TEXT;
static cbdd_mode_t g_cbdd_mode = CBDD_MODE_TEXT_80X25;
static int g_cbdd_debug_overlay = 0;
static int g_cbdd_high_contrast = 0;
static int g_cbdd_cursor_visible = 1;
static unsigned char g_cbdd_persist[CBDD_PERSIST_SECTORS * STORAGE_SECTOR_SIZE];

static void zero_bytes(unsigned char* dst, size_t count) {
    size_t i;
    for (i = 0U; i < count; ++i) {
        dst[i] = 0U;
    }
}

static uint32_t checksum_bytes(const unsigned char* data, size_t count) {
    size_t i;
    uint32_t sum = 0U;
    for (i = 0U; i < count; ++i) {
        sum = (sum << 5U) - sum + (uint32_t)data[i];
    }
    return sum;
}

static void cbdd_normalize(void) {
    if (g_cbdd_backend != CBDD_BACKEND_VGA_TEXT && g_cbdd_backend != CBDD_BACKEND_FRAMEBUFFER_PREVIEW) {
        g_cbdd_backend = CBDD_BACKEND_VGA_TEXT;
    }
    if (g_cbdd_mode != CBDD_MODE_TEXT_80X25 &&
        g_cbdd_mode != CBDD_MODE_TEXT_80X50_PREVIEW &&
        g_cbdd_mode != CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW) {
        g_cbdd_mode = CBDD_MODE_TEXT_80X25;
    }
    if (g_cbdd_backend == CBDD_BACKEND_VGA_TEXT && g_cbdd_mode == CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW) {
        g_cbdd_mode = CBDD_MODE_TEXT_80X25;
    }
    if (g_cbdd_backend == CBDD_BACKEND_FRAMEBUFFER_PREVIEW && g_cbdd_mode == CBDD_MODE_TEXT_80X50_PREVIEW) {
        g_cbdd_mode = CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW;
    }
}

void cbdd_init(void) {
    g_cbdd_initialized = 1;
    cbdd_reset_safe();
    (void)cbdd_load_persistent();
}

void cbdd_reset_safe(void) {
    g_cbdd_enabled = 1;
    g_cbdd_backend = CBDD_BACKEND_VGA_TEXT;
    g_cbdd_mode = CBDD_MODE_TEXT_80X25;
    g_cbdd_debug_overlay = 0;
    g_cbdd_high_contrast = 0;
    g_cbdd_cursor_visible = 1;
    g_cbdd_initialized = 1;
}

int cbdd_probe(void) {
    g_cbdd_initialized = 1;
    cbdd_normalize();
    return 0;
}

int cbdd_is_enabled(void) {
    return g_cbdd_enabled != 0 ? 1 : 0;
}

void cbdd_set_enabled(int enabled) {
    g_cbdd_enabled = enabled != 0 ? 1 : 0;
}

cbdd_backend_t cbdd_backend(void) {
    return g_cbdd_backend;
}

void cbdd_cycle_backend(void) {
    if (g_cbdd_backend == CBDD_BACKEND_VGA_TEXT) {
        g_cbdd_backend = CBDD_BACKEND_FRAMEBUFFER_PREVIEW;
        g_cbdd_mode = CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW;
    } else {
        g_cbdd_backend = CBDD_BACKEND_VGA_TEXT;
        g_cbdd_mode = CBDD_MODE_TEXT_80X25;
    }
    cbdd_normalize();
}

cbdd_mode_t cbdd_mode(void) {
    return g_cbdd_mode;
}

void cbdd_cycle_mode(void) {
    if (g_cbdd_backend == CBDD_BACKEND_FRAMEBUFFER_PREVIEW) {
        g_cbdd_mode = CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW;
        return;
    }
    if (g_cbdd_mode == CBDD_MODE_TEXT_80X25) {
        g_cbdd_mode = CBDD_MODE_TEXT_80X50_PREVIEW;
    } else {
        g_cbdd_mode = CBDD_MODE_TEXT_80X25;
    }
    cbdd_normalize();
}

void cbdd_toggle_debug_overlay(void) {
    g_cbdd_debug_overlay = g_cbdd_debug_overlay == 0 ? 1 : 0;
}

void cbdd_toggle_high_contrast(void) {
    g_cbdd_high_contrast = g_cbdd_high_contrast == 0 ? 1 : 0;
}

void cbdd_toggle_cursor(void) {
    g_cbdd_cursor_visible = g_cbdd_cursor_visible == 0 ? 1 : 0;
}

const char* cbdd_driver_name(void) {
    return "Cyralith Basic Display Driver";
}

const char* cbdd_backend_name(void) {
    switch (g_cbdd_backend) {
        case CBDD_BACKEND_FRAMEBUFFER_PREVIEW: return "Framebuffer preview";
        case CBDD_BACKEND_VGA_TEXT:
        default: return "VGA text";
    }
}

const char* cbdd_mode_name(void) {
    switch (g_cbdd_mode) {
        case CBDD_MODE_TEXT_80X50_PREVIEW: return "80x50 text preview";
        case CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW: return "safe framebuffer preview";
        case CBDD_MODE_TEXT_80X25:
        default: return "80x25 text";
    }
}

uint32_t cbdd_width(void) {
    switch (g_cbdd_mode) {
        case CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW: return 640U;
        case CBDD_MODE_TEXT_80X50_PREVIEW: return 80U;
        case CBDD_MODE_TEXT_80X25:
        default: return 80U;
    }
}

uint32_t cbdd_height(void) {
    switch (g_cbdd_mode) {
        case CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW: return 480U;
        case CBDD_MODE_TEXT_80X50_PREVIEW: return 50U;
        case CBDD_MODE_TEXT_80X25:
        default: return 25U;
    }
}

uint32_t cbdd_bpp(void) {
    switch (g_cbdd_mode) {
        case CBDD_MODE_FRAMEBUFFER_SAFE_PREVIEW: return 32U;
        case CBDD_MODE_TEXT_80X50_PREVIEW:
        case CBDD_MODE_TEXT_80X25:
        default: return 16U;
    }
}

uint32_t cbdd_flags(void) {
    uint32_t flags = CBDD_FLAG_SAFE_MODE;
    if (g_cbdd_backend == CBDD_BACKEND_VGA_TEXT) {
        flags |= CBDD_FLAG_TEXT_MODE | CBDD_FLAG_VGA_COMPAT;
    } else {
        flags |= CBDD_FLAG_FRAMEBUFFER;
    }
    if (g_cbdd_debug_overlay != 0) {
        flags |= CBDD_FLAG_DEBUG_OVERLAY;
    }
    if (g_cbdd_high_contrast != 0) {
        flags |= CBDD_FLAG_HIGH_CONTRAST;
    }
    if (g_cbdd_cursor_visible != 0) {
        flags |= CBDD_FLAG_CURSOR_VISIBLE;
    }
    return flags;
}

void cbdd_get_info(cbdd_info_t* out) {
    if (out == (cbdd_info_t*)0) {
        return;
    }
    out->driver_name = cbdd_driver_name();
    out->backend_name = cbdd_backend_name();
    out->mode_name = cbdd_mode_name();
    out->width = cbdd_width();
    out->height = cbdd_height();
    out->bpp = cbdd_bpp();
    out->flags = cbdd_flags();
    out->initialized = g_cbdd_initialized;
    out->enabled = cbdd_is_enabled();
    out->debug_overlay = g_cbdd_debug_overlay;
    out->high_contrast = g_cbdd_high_contrast;
    out->cursor_visible = g_cbdd_cursor_visible;
}

int cbdd_save_persistent(void) {
    cbdd_snapshot_header_t* header;
    cbdd_snapshot_payload_t* payload;
    size_t offset = sizeof(cbdd_snapshot_header_t);
    size_t payload_size = sizeof(cbdd_snapshot_payload_t);

    storage_init();
    if (storage_available() == 0) {
        return -1;
    }
    if (offset + payload_size > sizeof(g_cbdd_persist)) {
        return -1;
    }

    zero_bytes(g_cbdd_persist, sizeof(g_cbdd_persist));
    header = (cbdd_snapshot_header_t*)g_cbdd_persist;
    payload = (cbdd_snapshot_payload_t*)(g_cbdd_persist + offset);

    header->magic = CBDD_PERSIST_MAGIC;
    header->version = CBDD_PERSIST_VERSION;
    payload->enabled = (uint32_t)g_cbdd_enabled;
    payload->backend = (uint32_t)g_cbdd_backend;
    payload->mode = (uint32_t)g_cbdd_mode;
    payload->debug_overlay = (uint32_t)g_cbdd_debug_overlay;
    payload->high_contrast = (uint32_t)g_cbdd_high_contrast;
    payload->cursor_visible = (uint32_t)g_cbdd_cursor_visible;
    header->checksum = checksum_bytes((const unsigned char*)payload, payload_size);

    return storage_write_sector(CBDD_PERSIST_LBA, g_cbdd_persist);
}

int cbdd_load_persistent(void) {
    cbdd_snapshot_header_t* header = (cbdd_snapshot_header_t*)g_cbdd_persist;
    cbdd_snapshot_payload_t* payload;
    size_t offset = sizeof(cbdd_snapshot_header_t);
    size_t payload_size = sizeof(cbdd_snapshot_payload_t);
    uint32_t checksum;

    storage_init();
    if (storage_available() == 0) {
        return -1;
    }
    if (offset + payload_size > sizeof(g_cbdd_persist)) {
        return -1;
    }
    if (storage_read_sector(CBDD_PERSIST_LBA, g_cbdd_persist) != 0) {
        return -1;
    }

    payload = (cbdd_snapshot_payload_t*)(g_cbdd_persist + offset);
    if (header->magic != CBDD_PERSIST_MAGIC || header->version != CBDD_PERSIST_VERSION) {
        return -1;
    }
    checksum = checksum_bytes((const unsigned char*)payload, payload_size);
    if (checksum != header->checksum) {
        return -1;
    }

    g_cbdd_enabled = payload->enabled != 0U ? 1 : 0;
    g_cbdd_backend = (cbdd_backend_t)payload->backend;
    g_cbdd_mode = (cbdd_mode_t)payload->mode;
    g_cbdd_debug_overlay = payload->debug_overlay != 0U ? 1 : 0;
    g_cbdd_high_contrast = payload->high_contrast != 0U ? 1 : 0;
    g_cbdd_cursor_visible = payload->cursor_visible != 0U ? 1 : 0;
    g_cbdd_initialized = 1;
    cbdd_normalize();
    return 0;
}
