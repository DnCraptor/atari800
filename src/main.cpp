#include <cstdlib>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "graphics.h"

extern "C" {
#include "ps2.h"
}

extern "C" {
#include "libatari800/libatari800.h"
#include "sound.h"
#include "akey.h"
#include "memory.h"
#include "ff.h"
#include "debug.h"
#include "screen.h"
}

#include "psram_spi.h"
#include "nespad.h"
///#include <stdio.h>
extern "C" int	snprintf (char *__restrict, size_t, const char *__restrict, ...) _ATTRIBUTE ((__format__ (__printf__, 3, 4)));

static FATFS fs;
semaphore vga_start_semaphore;
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)
extern "C" UBYTE __aligned(4) __screen[Screen_HEIGHT * Screen_WIDTH];
///uint16_t SCREEN[TEXTMODE_ROWS][TEXTMODE_COLS];

static uint32_t input;

extern "C" {
bool __time_critical_func(handleScancode)(const uint32_t ps2scancode) {
    if (ps2scancode)
        input = ps2scancode;

    return true;
}
}

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    const auto buffer = libatari800_get_screen_ptr(); // TODO: SCREEN;
    graphics_set_buffer(buffer, Screen_WIDTH, Screen_HEIGHT);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(0, 0);
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
 ///   clrScr(1);

    sem_acquire_blocking(&vga_start_semaphore);
    // 60 FPS loop
#define frame_tick (16666)
    uint64_t tick = time_us_64();
#ifdef TFT
    uint64_t last_renderer_tick = tick;
#endif
    uint64_t last_input_tick = tick;
    while (true) {
#ifdef TFT
        if (tick >= last_renderer_tick + frame_tick) {
            refresh_lcd();
            last_renderer_tick = tick;
        }
#endif
        // Every 5th frame
        if (tick >= last_input_tick + frame_tick * 5) {
            nespad_read();
            last_input_tick = tick;
        }
        tick = time_us_64();

        tight_loop_contents();
    }

    __unreachable();
}
/***
extern "C"
int LIBATARI800_Input_Initialise(int *argc, char *argv[])
{
    return TRUE;
}

extern "C"
int PLATFORM_Keyboard(void)
{
    return 0xff;
    // return INPUT_key_code;
}

extern "C"
void LIBATARI800_Mouse(void) {

}

extern "C"
int PLATFORM_PORT(int num)
{
    // if (num == 0)
        // return (_joy[0] | (_joy[1] << 4)) ^ 0xFF;  // sense is inverted
    // if (num == 1)
        // return (_joy[2] | (_joy[3] << 4)) ^ 0xFF;  // sense is inverted
    return 0xff;
}

extern "C"
int PLATFORM_TRIG(int num)
{
    if (num < 0 || num >= 4)
        return 1;
    return 0;
    // return _trig[num] ^ 1;
}

Sound_setup_t Sound_desired = {
    15720,
    1,  // 8 bit
    1,  // 1 channel
    0,
    0
};
***/
#include "f_util.h"
static FATFS fatfs;
static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        printf("Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
    } else {
  ///      SD_CARD_AVAILABLE = true;
    }

  ////  if (SD_CARD_AVAILABLE) {
///        DIR dir;
///        if (f_opendir(&dir, "\\BK") != FR_OK) {
///            f_mkdir("\\BK");
///        } else {
///            f_closedir(&dir);
///        }
    //    insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
    //    insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img"); // TODO: why not attached?
    //    insertdisk(2, 819200, 0, "\\BK\\hdd0.img");
    //    insertdisk(3, 819200, 0, "\\BK\\hdd1.img"); // TODO: why not attached?
///        read_config("\\.conf");
   /// }
}

int main() {
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(378 * KHZ, true);
    stdio_init_all();
    keyboard_init();
    //keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    nespad_read();
    sleep_ms(50);

    init_fs(); // TODO: psram replacement (pagefile)
    init_psram();

    // F12 Boot to USB FIRMWARE UPDATE mode
    if (nespad_state & DPAD_START || input == 0x58) {
        printf("reset_usb_boot");
        reset_usb_boot(0, 0);
    }
    input_template_t input;
    /* force the 400/800 OS to get the Memo Pad */
    char *test_args[] = {
        "-atari",
        NULL,
    };
    printf("libatari800_init");
    libatari800_init(-1, test_args);
    printf("libatari800_clear_input_array");
    libatari800_clear_input_array(&input);

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (int i = 0; i < 6; i++) {
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }
    graphics_init_pal();
    //emulator_state_t state;
    //cpu_state_t *cpu;
    //int frame = 0;
    //char tmp[255];
    //bool blinker = true;
    //gpio_put(PICO_DEFAULT_LED_PIN, blinker);
    while(true) {
    //    libatari800_get_current_state(&state);
    //    cpu = (cpu_state_t *)&state.state[state.tags.cpu];  /* order: A,SR,SP,X,Y */
    //    snprintf(tmp, 255, "frame %d: A=%02x X=%02x Y=%02x SP=%02x SR=%02x\n", frame++, cpu->A, cpu->X, cpu->Y, cpu->P, cpu->S);
    //    printf(tmp);
    //    draw_text(tmp, 0, 0, 15, 0);
        libatari800_next_frame(&input);
        //blinker = ~blinker;
        //gpio_put(PICO_DEFAULT_LED_PIN, blinker);
    }

    __unreachable();
}
