/**
 * @file state_machine.c
 * @brief Strict state transition matrix and validation implementation.
 */

#include "state_machine.h"
#include "fault_manager.h"

/*
 * Kernel Transition Matrix:
 * Row: Current State
 * Col: Desired State
 */
static const bool s_kernel_transition_matrix[MK_KERNEL_STATE_COUNT][MK_KERNEL_STATE_COUNT] = {
    /* RESET    INIT   READY  RUNNING SLEEPING FAULT SHUTDOWN */
    /* RESET   */ { false, true,  false, false,  false,   false, true     },
    /* INIT    */ { false, false, true,  false,  false,   true,  true     },
    /* READY   */ { false, false, false, true,   false,   true,  true     },
    /* RUNNING */ { false, false, true,  false,  true,    true,  true     },
    /* SLEEPING*/ { false, false, false, true,   false,   true,  true     },
    /* FAULT   */ { false, false, false, false,  false,   false, true     },
    /* SHUTDOWN*/ { false, false, false, false,  false,   false, false    },
};

/*
 * Application Transition Matrix:
 * OFF -> INIT
 * INIT -> READY
 * READY -> ACTIVE
 * ACTIVE -> STOPPING
 * STOPPING -> READY
 * ACTIVE -> ERROR
 * ERROR -> INIT
 */
static const bool s_app_transition_matrix[MK_APP_STATE_COUNT][MK_APP_STATE_COUNT] = {
    /* OFF      INIT   READY  ACTIVE STOPPING ERROR */
    /* OFF     */ { false, true,  false, false,  false,   false },
    /* INIT    */ { false, false, true,  false,  false,   true  },
    /* READY   */ { true,  false, false, true,   false,   false },
    /* ACTIVE  */ { false, false, false, false,  true,    true  },
    /* STOPPING*/ { false, false, true,  false,  false,   true  },
    /* ERROR   */ { false, true,  false, false,  false,   false },
};

bool mk_state_validate_kernel_transition(mk_kernel_state_t from, mk_kernel_state_t to)
{
    if (from >= MK_KERNEL_STATE_COUNT || to >= MK_KERNEL_STATE_COUNT) {
        return false;
    }
    return s_kernel_transition_matrix[from][to];
}

bool mk_state_validate_app_transition(mk_app_state_t from, mk_app_state_t to)
{
    if (from >= MK_APP_STATE_COUNT || to >= MK_APP_STATE_COUNT) {
        return false;
    }
    return s_app_transition_matrix[from][to];
}

mk_status_t mk_kernel_transition(mk_kernel_t *kernel, mk_kernel_state_t new_state)
{
    if (kernel == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (!mk_state_validate_kernel_transition(kernel->state, new_state)) {
        mk_fault_record_full(
            MK_FAULT_STATE_INVALID_TRANSITION,
            MK_FAULT_SRC_STATE,
            MK_FAULT_SEV_CRITICAL,
            ((uint32_t)kernel->state << 16) | (uint32_t)new_state,
            0U
        );
        return MK_STATUS_INVALID_STATE;
    }

    kernel->state = new_state;
    return MK_STATUS_OK;
}

mk_status_t mk_app_transition(mk_kernel_t *kernel, mk_app_id_t app_id, mk_app_state_t new_state)
{
    if (kernel == NULL || app_id >= MK_APP_COUNT) {
        return MK_STATUS_INVALID_ARG;
    }

    mk_app_control_t *app = &kernel->apps[app_id];
    if (!mk_state_validate_app_transition(app->state, new_state)) {
        mk_fault_source_t src = (app_id == MK_APP_BLE_HID) ? MK_FAULT_SRC_BLE :
                                (app_id == MK_APP_WIFI_DIAGNOSTICS) ? MK_FAULT_SRC_WIFI :
                                (app_id == MK_APP_NETWORK_DIAGNOSTICS) ? MK_FAULT_SRC_NETWORK :
                                MK_FAULT_SRC_DISPLAY;
        mk_fault_record_full(
            MK_FAULT_STATE_INVALID_TRANSITION,
            src,
            MK_FAULT_SEV_WARNING,
            ((uint32_t)app->state << 16) | (uint32_t)new_state,
            (uint32_t)app_id
        );
        return MK_STATUS_INVALID_STATE;
    }

    app->state = new_state;
    return MK_STATUS_OK;
}

const char *mk_kernel_state_to_str(mk_kernel_state_t state)
{
    switch (state) {
        case MK_KERNEL_STATE_RESET:    return "RESET";
        case MK_KERNEL_STATE_INIT:     return "INIT";
        case MK_KERNEL_STATE_READY:    return "READY";
        case MK_KERNEL_STATE_RUNNING:  return "RUNNING";
        case MK_KERNEL_STATE_SLEEPING: return "SLEEPING";
        case MK_KERNEL_STATE_FAULT:    return "FAULT";
        case MK_KERNEL_STATE_SHUTDOWN: return "SHUTDOWN";
        default:                       return "UNKNOWN";
    }
}

const char *mk_app_state_to_str(mk_app_state_t state)
{
    switch (state) {
        case MK_APP_STATE_OFF:      return "OFF";
        case MK_APP_STATE_INIT:     return "INIT";
        case MK_APP_STATE_READY:    return "READY";
        case MK_APP_STATE_ACTIVE:   return "ACTIVE";
        case MK_APP_STATE_STOPPING: return "STOPPING";
        case MK_APP_STATE_ERROR:    return "ERROR";
        default:                    return "UNKNOWN";
    }
}
