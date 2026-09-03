#!/usr/bin/env python3
"""Verify that SliverOS fixed kernel memory symbols are linked into internal DRAM."""
from pathlib import Path
import re
import sys

MAP = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/microkernel-esp32.map")
text = MAP.read_text(errors="replace")

symbols = ("s_kernel", "s_arena_buffer", "s_pool_buf_0", "s_pool_buf_1", "s_pool_buf_2", "s_pool_buf_3", "s_pool_buf_4")
lines = text.splitlines()

for symbol in symbols:
    hits = [i for i, line in enumerate(lines) if re.search(rf"\b{re.escape(symbol)}\b", line)]
    if not hits:
        raise SystemExit(f"ERROR: linker map does not contain required symbol {symbol}")

    ok = False
    for idx in hits:
        window = "\n".join(lines[max(0, idx - 8): idx + 1]).lower()
        # GNU ld map files list the output section immediately before its symbols.
        if ".dram0" in window or ".dram1" in window:
            ok = True
            break
    if not ok:
        raise SystemExit(f"ERROR: {symbol} was not proven to be in an internal DRAM output section")
    print(f"PASS: {symbol} linked in internal DRAM section")

print("Internal kernel/arena/pool linker placement checks passed.")
