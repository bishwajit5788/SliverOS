/**
 * @file ui_runtime.h
 * @brief Master graphical runtime and developer diagnostics screen coordinator.
 */

#ifndef MK_UI_RUNTIME_H
#define MK_UI_RUNTIME_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_MODE_LAUNCHER = 0,
    UI_MODE_APP_ACTIVE,
    UI_MODE_DEV_SCREEN
} mk_ui_mode_t;

mk_status_t ui_runtime_init(void);
void ui_runtime_task(void *context);
void ui_runtime_set_mode(mk_ui_mode_t mode);
mk_ui_mode_t ui_runtime_get_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_UI_RUNTIME_H */
