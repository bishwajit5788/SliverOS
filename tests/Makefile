CC ?= clang
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -DMK_HOST_TEST=1 \
         -I../os/kernel \
         -I../os/hal \
         -I../os/vfs \
         -I../os/ui \
         -I../os/apps/ble_hid \
         -I../os/apps/wifi_diagnostics \
         -I../os/apps/network_diagnostics \
         -I../os/apps/retro_games

SRCS = test_runner.c \
       test_memory_manager.c \
       test_memory_pool.c \
       test_event_bus.c \
       test_scheduler.c \
       test_state_machine.c \
       test_macro_parser.c \
       test_vfs_block.c \
       test_vfs_recovery.c \
       test_wifi_classification.c \
       test_net_state_machine.c \
       test_game_physics.c \
       ../os/kernel/kernel.c \
       ../os/kernel/scheduler.c \
       ../os/kernel/state_machine.c \
       ../os/kernel/memory_manager.c \
       ../os/kernel/memory_pool.c \
       ../os/kernel/fault_manager.c \
       ../os/kernel/event_bus.c \
       ../os/hal/hal_timer.c \
       ../os/hal/hal_gpio.c \
       ../os/hal/hal_spi.c \
       ../os/hal/hal_wifi.c \
       ../os/hal/hal_ble.c \
       ../os/vfs/vfs.c \
       ../os/vfs/vfs_block.c \
       ../os/vfs/vfs_log.c \
       ../os/apps/ble_hid/macro_parser.c \
       ../os/apps/wifi_diagnostics/wifi_diagnostics.c \
       ../os/apps/network_diagnostics/network_diagnostics.c \
       ../os/apps/retro_games/retro_games.c \
       ../os/apps/retro_games/input.c \
       ../os/apps/retro_games/renderer.c

TARGET = test_runner

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
