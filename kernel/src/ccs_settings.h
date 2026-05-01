#ifndef CCS_SETTINGS_H
#define CCS_SETTINGS_H

#include <stdint.h>

#define CCS_SETTINGS_FLAG_APP_ISOLATION_PLAN   0x00000100U
#define CCS_SETTINGS_FLAG_MINIMAL_NETWORK      0x00000200U
#define CCS_SETTINGS_FLAG_SIGNED_SOURCES_PLAN  0x00000400U
#define CCS_SETTINGS_FLAG_ROLLBACK_PLAN        0x00000800U
#define CCS_SETTINGS_FLAG_PRIMARY_PACKAGE_PATH 0x00001000U

typedef enum {
    CCS_SETTINGS_BASELINE = 0,
    CCS_SETTINGS_CONSISTENT_TARGET = 1,
    CCS_SETTINGS_NATIVE_TARGET = 2
} ccs_settings_profile_t;

void ccs_settings_init(void);
ccs_settings_profile_t ccs_settings_profile(void);
void ccs_settings_cycle_profile(void);
const char* ccs_settings_profile_name(void);
int ccs_settings_flag(uint32_t flag);
void ccs_settings_toggle(uint32_t flag);
void ccs_settings_set_flag(uint32_t flag, int enabled);

#endif
