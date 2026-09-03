/**
 * @file state_machine.h
 * @brief Strict state transition validation for kernel and applications.
 */

#ifndef MK_STATE_MACHINE_H
#define MK_STATE_MACHINE_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate whether a kernel transition from `from` to `to` is legally permitted.
 */
bool mk_state_validate_kernel_transition(mk_kernel_state_t from, mk_kernel_state_t to);

/**
 * @brief Validate whether an application transition from `from` to `to` is legally permitted.
 */
bool mk_state_validate_app_transition(mk_app_state_t from, mk_app_state_t to);

/**
 * @brief Attempt to transition the kernel lifecycle state.
 * Validates transition rules and updates kernel state or records a fault.
 *
 * @param kernel Pointer to kernel control block.
 * @param new_state Target state.
 * @return MK_STATUS_OK on success, MK_STATUS_INVALID_STATE if transition rejected.
 */
mk_status_t mk_kernel_transition(mk_kernel_t *kernel, mk_kernel_state_t new_state);

/**
 * @brief Attempt to transition an application lifecycle state.
 * Kernel owns application transitions. Rejects invalid transitions.
 *
 * @param kernel Pointer to kernel control block.
 * @param app_id Target application ID.
 * @param new_state Target state.
 * @return MK_STATUS_OK on success, MK_STATUS_INVALID_STATE or MK_STATUS_INVALID_ARG on failure.
 */
mk_status_t mk_app_transition(mk_kernel_t *kernel, mk_app_id_t app_id, mk_app_state_t new_state);

/**
 * @brief Human-readable string for kernel state.
 */
const char *mk_kernel_state_to_str(mk_kernel_state_t state);

/**
 * @brief Human-readable string for application state.
 */
const char *mk_app_state_to_str(mk_app_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* MK_STATE_MACHINE_H */
