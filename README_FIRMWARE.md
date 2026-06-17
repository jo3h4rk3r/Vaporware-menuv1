# Firmware Backups

Original firmware dumps from devices — keep these safe before flashing anything new.

| File | Device | Dumped | Notes |
|------|--------|--------|-------|
| *(gv2024_1018v8.bin — LOST)* | GV2024 1018V8 | 2026-04-15 | Dumped but only saved to /tmp, lost when session ended |

## How to dump before flashing

```bat
wsl openocd -f /mnt/c/Users/cooli/Claude_Vapes/Vaporware/examples/slots/n32g031.openocd.cfg ^
  -c "tcl_port disabled; telnet_port disabled; gdb_port disabled" ^
  -c "init" -c "halt" ^
  -c "dump_image /mnt/c/Users/cooli/Claude_Vapes/Vaporware/firmware/DEVICENAME_backup.bin 0x08000000 65536" ^
  -c "exit"
```

Always dump to `/mnt/c/...` (the Windows filesystem), never `/tmp`.
