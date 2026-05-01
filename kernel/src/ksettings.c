#include "ksettings.h"
#include "storage.h"
#include <stddef.h>

#define KSETTINGS_PERSIST_MAGIC 0x53474b43U /* CKGS */
#define KSETTINGS_PERSIST_VERSION 1U
#define KSETTINGS_PERSIST_LBA 52U
#define KSETTINGS_PERSIST_SECTORS 1U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;
    uint32_t reserved;
} ksettings_snapshot_header_t;

typedef struct {
    uint32_t flags;
    uint32_t ccs_profile;
    uint32_t revision;
    uint32_t reserved[13];
} ksettings_snapshot_payload_t;

static uint32_t g_ksettings_flags = 0U;
static ksettings_ccs_profile_t g_ksettings_ccs_profile = KSETTINGS_CCS_BASELINE;
static uint32_t g_ksettings_revision = 1U;
static unsigned char g_ksettings_persist[KSETTINGS_PERSIST_SECTORS * STORAGE_SECTOR_SIZE];

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

static void normalize_profile(void) {
    if (g_ksettings_ccs_profile != KSETTINGS_CCS_BASELINE &&
        g_ksettings_ccs_profile != KSETTINGS_CCS_CONSISTENT_TARGET &&
        g_ksettings_ccs_profile != KSETTINGS_CCS_NATIVE_TARGET) {
        g_ksettings_ccs_profile = KSETTINGS_CCS_BASELINE;
    }
}

void ksettings_reset_defaults(void) {
    g_ksettings_flags = KSETTINGS_FLAG_SAFE_DEFAULTS |
                        KSETTINGS_FLAG_AUDIT_LOG |
                        KSETTINGS_FLAG_RECOVERY_READY |
                        KSETTINGS_FLAG_PROTECT_SYSTEM_PATHS |
                        KSETTINGS_FLAG_SYSDO_SINGLE_COMMAND |
                        KSETTINGS_FLAG_APP_ISOLATION_PLAN |
                        KSETTINGS_FLAG_MINIMAL_NETWORK |
                        KSETTINGS_FLAG_PRIMARY_PACKAGE_PATH |
                        KSETTINGS_FLAG_FS_PERSISTENCE |
                        KSETTINGS_FLAG_FS_STRICT_PERMS |
                        KSETTINGS_FLAG_FS_X32_CONSOLE;
    g_ksettings_ccs_profile = KSETTINGS_CCS_BASELINE;
    g_ksettings_revision = 1U;
}

void ksettings_init(void) {
    ksettings_reset_defaults();
    (void)ksettings_load_persistent();
}

int ksettings_flag(uint32_t flag) {
    return (g_ksettings_flags & flag) != 0U ? 1 : 0;
}

void ksettings_set_flag(uint32_t flag, int enabled) {
    if (enabled != 0) {
        g_ksettings_flags |= flag;
    } else {
        g_ksettings_flags &= ~flag;
    }
    g_ksettings_revision++;
}

void ksettings_toggle(uint32_t flag) {
    ksettings_set_flag(flag, ksettings_flag(flag) == 0 ? 1 : 0);
}

ksettings_ccs_profile_t ksettings_ccs_profile(void) {
    return g_ksettings_ccs_profile;
}

void ksettings_cycle_ccs_profile(void) {
    if (g_ksettings_ccs_profile == KSETTINGS_CCS_BASELINE) {
        g_ksettings_ccs_profile = KSETTINGS_CCS_CONSISTENT_TARGET;
    } else if (g_ksettings_ccs_profile == KSETTINGS_CCS_CONSISTENT_TARGET) {
        g_ksettings_ccs_profile = KSETTINGS_CCS_NATIVE_TARGET;
    } else {
        g_ksettings_ccs_profile = KSETTINGS_CCS_BASELINE;
    }
    g_ksettings_revision++;
}

const char* ksettings_ccs_profile_name(void) {
    switch (g_ksettings_ccs_profile) {
        case KSETTINGS_CCS_CONSISTENT_TARGET: return "CCS consistent target";
        case KSETTINGS_CCS_NATIVE_TARGET: return "CCS native target";
        case KSETTINGS_CCS_BASELINE:
        default: return "CCS baseline";
    }
}

void ksettings_get_info(ksettings_info_t* out) {
    if (out == (ksettings_info_t*)0) {
        return;
    }
    out->flags = g_ksettings_flags;
    out->ccs_profile = g_ksettings_ccs_profile;
    out->revision = g_ksettings_revision;
}

int ksettings_save_persistent(void) {
    ksettings_snapshot_header_t* header;
    ksettings_snapshot_payload_t* payload;
    size_t offset = sizeof(ksettings_snapshot_header_t);
    size_t payload_size = sizeof(ksettings_snapshot_payload_t);

    storage_init();
    if (storage_available() == 0) {
        return -1;
    }
    if (offset + payload_size > sizeof(g_ksettings_persist)) {
        return -1;
    }

    zero_bytes(g_ksettings_persist, sizeof(g_ksettings_persist));
    header = (ksettings_snapshot_header_t*)g_ksettings_persist;
    payload = (ksettings_snapshot_payload_t*)(g_ksettings_persist + offset);

    header->magic = KSETTINGS_PERSIST_MAGIC;
    header->version = KSETTINGS_PERSIST_VERSION;
    payload->flags = g_ksettings_flags;
    payload->ccs_profile = (uint32_t)g_ksettings_ccs_profile;
    payload->revision = g_ksettings_revision;
    header->checksum = checksum_bytes((const unsigned char*)payload, payload_size);

    return storage_write_sector(KSETTINGS_PERSIST_LBA, g_ksettings_persist);
}

int ksettings_load_persistent(void) {
    ksettings_snapshot_header_t* header = (ksettings_snapshot_header_t*)g_ksettings_persist;
    ksettings_snapshot_payload_t* payload;
    size_t offset = sizeof(ksettings_snapshot_header_t);
    size_t payload_size = sizeof(ksettings_snapshot_payload_t);
    uint32_t checksum;

    storage_init();
    if (storage_available() == 0) {
        return -1;
    }
    if (offset + payload_size > sizeof(g_ksettings_persist)) {
        return -1;
    }
    if (storage_read_sector(KSETTINGS_PERSIST_LBA, g_ksettings_persist) != 0) {
        return -1;
    }

    payload = (ksettings_snapshot_payload_t*)(g_ksettings_persist + offset);
    if (header->magic != KSETTINGS_PERSIST_MAGIC || header->version != KSETTINGS_PERSIST_VERSION) {
        return -1;
    }
    checksum = checksum_bytes((const unsigned char*)payload, payload_size);
    if (checksum != header->checksum) {
        return -1;
    }

    g_ksettings_flags = payload->flags;
    g_ksettings_ccs_profile = (ksettings_ccs_profile_t)payload->ccs_profile;
    g_ksettings_revision = payload->revision;
    normalize_profile();
    return 0;
}
