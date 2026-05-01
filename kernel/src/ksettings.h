#ifndef KSETTINGS_H
#define KSETTINGS_H

#include <stdint.h>

#define KSETTINGS_FLAG_SAFE_DEFAULTS        0x00000001U
#define KSETTINGS_FLAG_AUDIT_LOG            0x00000002U
#define KSETTINGS_FLAG_RECOVERY_READY       0x00000004U
#define KSETTINGS_FLAG_UPDATE_GUARD         0x00000008U
#define KSETTINGS_FLAG_PROTECT_SYSTEM_PATHS 0x00000010U
#define KSETTINGS_FLAG_SYSDO_SINGLE_COMMAND  0x00000020U
#define KSETTINGS_FLAG_APP_ISOLATION_PLAN   0x00000100U
#define KSETTINGS_FLAG_MINIMAL_NETWORK      0x00000200U
#define KSETTINGS_FLAG_SIGNED_SOURCES_PLAN  0x00000400U
#define KSETTINGS_FLAG_ROLLBACK_PLAN        0x00000800U
#define KSETTINGS_FLAG_PRIMARY_PACKAGE_PATH 0x00001000U
#define KSETTINGS_FLAG_FS_PERSISTENCE       0x00002000U
#define KSETTINGS_FLAG_FS_AUTOSAVE          0x00004000U
#define KSETTINGS_FLAG_FS_STRICT_PERMS      0x00008000U
#define KSETTINGS_FLAG_FS_X32_CONSOLE       0x00010000U
#define KSETTINGS_FLAG_FS_RECOVERY_SCAN     0x00020000U

typedef enum {
    KSETTINGS_CCS_BASELINE = 0,
    KSETTINGS_CCS_CONSISTENT_TARGET = 1,
    KSETTINGS_CCS_NATIVE_TARGET = 2
} ksettings_ccs_profile_t;

typedef struct {
    uint32_t flags;
    ksettings_ccs_profile_t ccs_profile;
    uint32_t revision;
} ksettings_info_t;

void ksettings_init(void);
void ksettings_reset_defaults(void);

int ksettings_flag(uint32_t flag);
void ksettings_set_flag(uint32_t flag, int enabled);
void ksettings_toggle(uint32_t flag);

ksettings_ccs_profile_t ksettings_ccs_profile(void);
void ksettings_cycle_ccs_profile(void);
const char* ksettings_ccs_profile_name(void);

void ksettings_get_info(ksettings_info_t* out);
int ksettings_save_persistent(void);
int ksettings_load_persistent(void);

#endif
