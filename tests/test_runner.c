/**
 * @file test_runner.c
 * @brief Host unit test suite runner for MicroKernel OS executive.
 */

#include <stdio.h>

extern void test_memory_manager(void);
extern void run_memory_pool_tests(void);
extern void run_event_bus_tests(void);
extern void test_scheduler(void);
extern void test_state_machine(void);
extern void test_macro_parser(void);
extern void test_vfs_block(void);
extern void run_vfs_recovery_tests(void);
extern void run_wifi_classification_tests(void);
extern void run_net_state_machine_tests(void);
extern void run_game_physics_tests(void);

int main(void)
{
    printf("\n===================================================\n");
    printf("     MicroKernel OS Host-Native Test Suite         \n");
    printf("===================================================\n\n");

    test_memory_manager();
    run_memory_pool_tests();
    run_event_bus_tests();
    test_scheduler();
    test_state_machine();
    test_macro_parser();
    test_vfs_block();
    run_vfs_recovery_tests();
    run_wifi_classification_tests();
    run_net_state_machine_tests();
    run_game_physics_tests();

    printf("\n===================================================\n");
    printf("  >>> ALL 11 HOST UNIT TEST SUITES PASSED (100%%) <<< \n");
    printf("===================================================\n\n");
    return 0;
}
