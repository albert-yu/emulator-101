#ifndef EMU8080_H
#define EMU8080_H

typedef enum emu_mode_t {
    RUN_MODE,
    STEP_MODE,
    DISASM_MODE 
} EmuMode;

int emu_start(char *folder, EmuMode mode);

#endif // EMU8080_H
