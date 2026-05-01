#include "ccs_settings.h"
#include "ksettings.h"

void ccs_settings_init(void) {
    /* CCS settings currently live in the kernel settings snapshot.
       This separate module keeps the CCS UI/policy boundary clean for later. */
}

ccs_settings_profile_t ccs_settings_profile(void) {
    return (ccs_settings_profile_t)ksettings_ccs_profile();
}

void ccs_settings_cycle_profile(void) {
    ksettings_cycle_ccs_profile();
}

const char* ccs_settings_profile_name(void) {
    return ksettings_ccs_profile_name();
}

int ccs_settings_flag(uint32_t flag) {
    return ksettings_flag(flag);
}

void ccs_settings_toggle(uint32_t flag) {
    ksettings_toggle(flag);
}

void ccs_settings_set_flag(uint32_t flag, int enabled) {
    ksettings_set_flag(flag, enabled);
}
