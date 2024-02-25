#include <cstdlib>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <pico/bootrom.h>
#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

#include "graphics.h"
#include "psram_spi.h"
#include "nespad.h"

extern "C" {
#include "ps2.h"
#include "libatari800/libatari800.h"
#include "sound.h"
#include "akey.h"
#include "memory.h"
#include "ff.h"
#include "debug.h"
#include "screen.h"
#include "sound.h"
#include "util.h"

#include "platform.h"

/* a runtime switch for the kbd_joy_X_enabled vars is in the UI */
static int kbd_joy_0_enabled = TRUE;	/* enabled by default, doesn't hurt */
static int kbd_joy_1_enabled = FALSE;	/* disabled, would steal normal keys */

int PLATFORM_IsKbdJoystickEnabled(int num) {
	switch (num) {
	case 0:
		return kbd_joy_0_enabled;
	case 1:
		return kbd_joy_1_enabled;
	default:
		return FALSE;
	}
}

/* Each emulated joystick can take its input from host keyboard, an
   LPT joystick, an actual SDL joystick, or a combination thereof. */
#define MAX_JOYSTICKS	4
static struct stick_dev {
	int *kbd;
	int fd_lpt;
///	SDL_Joystick *sdl_joy;
	int nbuttons;
	SDL_INPUT_RealJSConfig_t real_config;
} stick_devs[MAX_JOYSTICKS];
static int grab_mouse = FALSE;
static int swap_joysticks = FALSE;
static int joy_distinct = FALSE;

#define SDLK_KP8 0x48
#define SDLK_KP5 0x4c
#define SDLK_KP4 0x4b
#define SDLK_KP6 0x4d
#define SDLK_RCTRL 0xe01d

#define SDLK_w 0x11
#define SDLK_s 0x1f
#define SDLK_a 0x1e
#define SDLK_d 0x20
#define SDLK_LCTRL 0x1d

/* joystick emulation (via stick_dev.kbd)
   keys are loaded from config file
   Here the defaults if there is no keymap in the config file,
   in the order up, down, left, right, trigger */
static int kbd_stick0[5] = {
	SDLK_KP8, SDLK_KP5, SDLK_KP4, SDLK_KP6, SDLK_RCTRL
};
static int kbd_stick1[5] = {
	SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_LCTRL
};
#define KBD_STICK_0_UP kbd_stick0[0]
#define KBD_STICK_0_DOWN kbd_stick0[1]
#define KBD_STICK_0_LEFT kbd_stick0[2]
#define KBD_STICK_0_RIGHT kbd_stick0[3]
#define KBD_TRIG_0 kbd_stick0[4]
#define KBD_STICK_1_UP kbd_stick1[0]
#define KBD_STICK_1_DOWN kbd_stick1[1]
#define KBD_STICK_1_LEFT kbd_stick1[2]
#define KBD_STICK_1_RIGHT kbd_stick1[3]
#define KBD_TRIG_1 kbd_stick1[4]

static void update_kbd_sticks(void) {
	stick_devs[swap_joysticks].kbd = kbd_joy_0_enabled ? kbd_stick0 : NULL;
	stick_devs[1 - swap_joysticks].kbd = kbd_joy_1_enabled ? kbd_stick1 : NULL;
}

void PLATFORM_ToggleKbdJoystickEnabled(int num) {
	switch (num) {
	case 0:
		kbd_joy_0_enabled = !kbd_joy_0_enabled;
	case 1:
		kbd_joy_1_enabled = !kbd_joy_1_enabled;
	}
	update_kbd_sticks();
}

/*Get pointer to a real joystick configuration*/
SDL_INPUT_RealJSConfig_t* SDL_INPUT_GetRealJSConfig(int joyIndex) {
    return &stick_devs[joyIndex].real_config;
}


void PLATFORM_SetJoystickKey(int joystick, int direction, int value)
{
	if (joystick == 0) {
		switch(direction) {
			case 0: KBD_STICK_0_LEFT = value; break;
			case 1: KBD_STICK_0_UP = value; break;
			case 2: KBD_STICK_0_RIGHT = value; break;
			case 3: KBD_STICK_0_DOWN = value; break;
			case 4: KBD_TRIG_0 = value; break;
		}
	}
	else {
		switch(direction) {
			case 0: KBD_STICK_1_LEFT = value; break;
			case 1: KBD_STICK_1_UP = value; break;
			case 2: KBD_STICK_1_RIGHT = value; break;
			case 3: KBD_STICK_1_DOWN = value; break;
			case 4: KBD_TRIG_1 = value; break;
		}
	}
}

void PLATFORM_GetJoystickKeyName(int joystick, int direction, char *buffer, int bufsize) {
	char *key = "";
	switch(direction) {
		case 0: key = "LEFT"; /// SDL_GetKeyName((SDLKey)(joystick == 0 ? KBD_STICK_0_LEFT : KBD_STICK_1_LEFT));
			break;
		case 1: key = "UP"; /// SDL_GetKeyName((SDLKey)(joystick == 0 ? KBD_STICK_0_UP : KBD_STICK_1_UP));
			break;
		case 2: key = "RIGHT"; ///SDL_GetKeyName((SDLKey)(joystick == 0 ? KBD_STICK_0_RIGHT : KBD_STICK_1_RIGHT));
			break;
		case 3: key = "DOWN"; ///SDL_GetKeyName((SDLKey)(joystick == 0 ? KBD_STICK_0_DOWN : KBD_STICK_1_DOWN));
			break;
		case 4: key = "TRIG";///SDL_GetKeyName((SDLKey)(joystick == 0 ? KBD_TRIG_0 : KBD_TRIG_1));
			break;
	}
	snprintf(buffer, bufsize, "%11s", key);
}

int PLATFORM_GetRawKey(void) {
	while(TRUE) { 
        /// TODO:
        return 0x35;
    }
}

}

static FATFS fs;
semaphore vga_start_semaphore;
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)
extern "C" UBYTE __aligned(4) __screen[Screen_HEIGHT * Screen_WIDTH];
///uint16_t SCREEN[TEXTMODE_ROWS][TEXTMODE_COLS];

pwm_config config = pwm_get_default_config();
void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}

void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

static input_template_t input_map;
static unsigned int sp = 0;
extern "C" {
bool __time_critical_func(handleScancode)(const uint32_t ps2scancode) {
    if (!ps2scancode) {
        input_map.keychar = 0;
        input_map.select = 0;
        input_map.option = 0;
        input_map.start = 0;
    } else {
        if (ps2scancode == 0xE053) { input_map.keychar = 127; return true; } // Del
        if (ps2scancode == 0xE01D) { input_map.control |= 2; return true; } // rCnt
        if (ps2scancode == 0xE09D) { input_map.control &= ~2; return true; } // rCnt
        if (ps2scancode == 0xE038) { input_map.alt |= 2; return true; } // rAlt
        if (ps2scancode == 0xE0B8) { input_map.alt &= ~2; return true; } // rAlt
        //if (ps2scancode == 0xE05B) { ; return true; } // lWin
        //if (ps2scancode == 0xE05C) { ; return true; } // rWin
        //if (ps2scancode == 0xE05D) { ; return true; } // Menu
        if ((ps2scancode & 0xFF) > 0x80) {
            switch(ps2scancode & 0xFF) {
                case 0xb6: sp &= ~1; input_map.shift = sp; break; // rshift
                case 0xaa: sp &= ~2; input_map.shift = sp; break; // lshift // CapsLock TODO: reverse shift
                case 0x9d: input_map.control = 0; break; // lctrl
                case 0xb8: input_map.alt &= ~1; break; // lalt
                case 0xbc: input_map.option = 0; break; // F2 Option
                case 0xbd: input_map.select = 0; break; // F3 Select
                case 0xbe: input_map.start = 0; break; // F4 Start
                default: input_map.keychar = 0; break;
            }
            return true;
        }
        switch(ps2scancode & 0xFF) {
            case 0x1c: input_map.keychar = '\n'; break;
            case 0x39: input_map.keychar = ' '; break;
            case 0x0b: input_map.keychar = sp ? ')' : '0'; break;
            case 0x02: input_map.keychar = sp ? '!' : '1'; break;
            case 0x03: input_map.keychar = sp ? '@' : '2'; break;
            case 0x04: input_map.keychar = sp ? '#' : '3'; break;
            case 0x05: input_map.keychar = sp ? '$' : '4'; break;
            case 0x06: input_map.keychar = sp ? '%' : '5'; break;
            case 0x07: input_map.keychar = sp ? '^' : '6'; break;
            case 0x08: input_map.keychar = sp ? '&' : '7'; break;
            case 0x09: input_map.keychar = sp ? '*' : '8'; break;
            case 0x0a: input_map.keychar = sp ? '(' : '9'; break;
            case 0x34: input_map.keychar = sp ? '>' : '.'; break;
            case 0x33: input_map.keychar = sp ? '<' : ','; break;
            case 0x27: input_map.keychar = sp ? ':' : ';'; break;
            case 0x35: input_map.keychar = sp ? '?' : '/'; break;
            case 0x2b: input_map.keychar = sp ? '|' : '\\'; break;
            case 0x29: input_map.keychar = sp ? '`' : '~'; break;
            case 0x0c: input_map.keychar = sp ? '_' : '-'; break;
            case 0x0d: input_map.keychar = sp ? '+' : '='; break;
            case 0x1a: input_map.keychar = sp ? '{' : '['; break;
            case 0x1b: input_map.keychar = sp ? '}' : ']'; break;
            case 0x28: input_map.keychar = sp ? '\'' : '"'; break;
            case 0x0f: input_map.keychar = '\t'; break;
            case 0x3a: if(sp & 4) sp &= ~4; else sp |= 4; input_map.shift = sp; break; // CapsLock
            case 0x1d: input_map.control = 1; break; // lctl
            case 0x38: input_map.alt |= 1; break; // lalt
            case 0x1e: input_map.keychar = sp ? 'A' : 'a'; break;
            case 0x30: input_map.keychar = sp ? 'B' : 'b'; break;
            case 0x2e: input_map.keychar = sp ? 'C' : 'c'; break;
            case 0x20: input_map.keychar = sp ? 'D' : 'd'; break;
            case 0x12: input_map.keychar = sp ? 'E' : 'e'; break;
            case 0x21: input_map.keychar = sp ? 'F' : 'f'; break;
            case 0x22: input_map.keychar = sp ? 'G' : 'g'; break;
            case 0x23: input_map.keychar = sp ? 'H' : 'h'; break;
            case 0x17: input_map.keychar = sp ? 'I' : 'i'; break;
            case 0x24: input_map.keychar = sp ? 'J' : 'j'; break;
            case 0x25: input_map.keychar = sp ? 'K' : 'k'; break;
            case 0x26: input_map.keychar = sp ? 'L' : 'l'; break;
            case 0x32: input_map.keychar = sp ? 'M' : 'm'; break;
            case 0x31: input_map.keychar = sp ? 'N' : 'n'; break;
            case 0x18: input_map.keychar = sp ? 'O' : 'o'; break;
            case 0x19: input_map.keychar = sp ? 'P' : 'p'; break;
            case 0x10: input_map.keychar = sp ? 'Q' : 'q'; break;
            case 0x13: input_map.keychar = sp ? 'R' : 'r'; break;
            case 0x1f: input_map.keychar = sp ? 'S' : 's'; break;
            case 0x14: input_map.keychar = sp ? 'T' : 't'; break;
            case 0x16: input_map.keychar = sp ? 'U' : 'u'; break;
            case 0x2f: input_map.keychar = sp ? 'V' : 'v'; break;
            case 0x11: input_map.keychar = sp ? 'W' : 'w'; break;
            case 0x2d: input_map.keychar = sp ? 'X' : 'x'; break;
            case 0x15: input_map.keychar = sp ? 'Y' : 'y'; break;
            case 0x2c: input_map.keychar = sp ? 'Z' : 'z'; break;
            case 0x36: sp |= 1; input_map.shift = sp; break; // rshift
            case 0x2a: sp |= 2; input_map.shift = sp; break; // lshift
            case 0x01: input_map.keychar = 27; break; // Esc
            case 0x0e: input_map.keychar = '\b'; break; // Backspace
            case 0x3b: input_map.keychar = 255; break; // F1 UI
            case 0x3c: input_map.option = 1; break; // F2 Option
            case 0x3d: input_map.select = 1; break; // F3 Select
            case 0x3e: input_map.start = 1; break; // F4 Start
            case 0x3f: input_map.keychar = 250; break; // F5 Help
            case 0x48: input_map.keychar = 254; break; // Up
            case 0x4b: input_map.keychar = 253; break; // Left
            case 0x50: input_map.keychar = 252; break; // Down
            case 0x4d: input_map.keychar = 251; break; // Right
            default:
                return true;
        }
        if(input_map.alt) {
            if(input_map.keychar >= 'a' && input_map.keychar <= 'z')
                input_map.keychar -= 'a' - 1;
            else if(input_map.keychar >= 'A' && input_map.keychar <= 'Z')
                input_map.keychar -= 'A' - 1;
        }
    }
    return true;
}
}

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    const auto buffer = libatari800_get_screen_ptr();
    graphics_set_buffer(buffer, Screen_WIDTH, Screen_HEIGHT);
    graphics_set_textbuffer(buffer);
    graphics_set_bgcolor(0x000000);
    graphics_set_offset(0, 0);
    graphics_set_mode(GRAPHICSMODE_DEFAULT);
    graphics_set_flashmode(false, false);
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

static UINT sound_array_idx = 0;
static UINT sound_array_fill = 0;
extern "C" UBYTE *LIBATARI800_Sound_array = 0;
extern "C" void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size)
{
    if (LIBATARI800_Sound_array) free(LIBATARI800_Sound_array);
    LIBATARI800_Sound_array = (UBYTE *) Util_malloc(size, "PLATFORM_SoundWrite");
	memcpy(LIBATARI800_Sound_array, buffer, size);
	sound_array_idx = 0;
	sound_array_fill = size;
}

#ifdef SOUND
static repeating_timer_t timer;
static int snd_channels = 2;
static int snd_bits = 16;
static bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt) {
    static uint16_t outL = 0;  
    static uint16_t outR = 0;
    register uint16_t idx = sound_array_idx;
    if (idx >= sound_array_fill) {
        return true;
    }
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;
    if (!Sound_enabled || paused) {
        return true;
    }
    register UBYTE* uba = LIBATARI800_Sound_array;
    if (snd_channels == 2) {
     //   if (snd_bits == 8) {
            outL = ((uint16_t)uba[idx]) << (11 - 8);
            outR = ((uint16_t)uba[idx + 1]) << (11 - 8);
            sound_array_idx += 2;
     //   } else {
     //       outL = ((uint16_t*)uba)[idx << 1] >> (16 - 11);
     //       outR = ((uint16_t*)uba)[(idx << 1) + 2] >> (16 - 11);
     //       sound_array_idx += 4;
     //   }
    } else {
     //   if (snd_bits == 8) {
            outL = outR = ((uint16_t)uba[idx]) << (11 - 8);
            sound_array_idx++;
      //  } else {
      //      outL = outR = ((uint16_t*)uba)[idx << 1] >> (16 - 11);
      //      sound_array_idx += 2;
      //  }
    }
  //  if (outR || outL) {
     ///   register uint8_t mult = 0; ///g_conf.snd_volume;
     ///   if (mult >= 0) {
     ///       if (mult > 5) mult = 5;
     //       outL <<= mult;
     ///       outR <<= mult;
     ///   } else {
     ///       register int8_t div = -mult;
     ///       if (div > 16) div = 16;
     ///       outL >>= div;
     ///       outR >>= div;
     ///   }
 //       pwm_set_gpio_level(BEEPER_PIN, 0);
 //   }
    return true;
}
#endif

#include "f_util.h"
static FATFS fatfs;
bool SD_CARD_AVAILABLE = false;
static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        printf("Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
    } else {
        SD_CARD_AVAILABLE = true;
    }
}

int main() {
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(10);
    set_sys_clock_khz(378 * KHZ, true);
    stdio_init_all();
    keyboard_init();
    keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    nespad_read();
    sleep_ms(50);

    // F12 Boot to USB FIRMWARE UPDATE mode
    if (nespad_state & DPAD_START || input_map.keycode == 0x58) { // F12
        printf("reset_usb_boot");
        reset_usb_boot(0, 0);
    }
    /* force the 400/800 OS to get the Memo Pad */
    char *test_args[] = {
        "-atari",
       // "-xl",
       // "-atari_files",
       // "\\atari800",
       // "-basic",
        NULL,
    };

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

    init_fs(); // TODO: psram replacement (pagefile)
    init_psram();

    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
#ifdef SOUND
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
#endif
#if LOAD_WAV_PIO
    //пин ввода звука
    inInit(LOAD_WAV_PIO);
#endif
    printf("libatari800_init");
    libatari800_init(-1, test_args);
    printf("libatari800_clear_input_array");
    libatari800_clear_input_array(&input_map);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (int i = 0; i < 6; i++) {
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(33);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }

#ifdef SOUND
	int hz = libatari800_get_sound_frequency(); ///44100;	//44000 //44100 //96000 //22050
    snd_channels = libatari800_get_num_sound_channels();
    snd_bits = libatari800_get_sound_sample_size();
	// negative timeout means exact delay (rather than delay between callbacks)
	if (!add_repeating_timer_us(-1000000 / hz, AY_timer_callback, NULL, &timer)) {
		printf("Failed to add timer");
	}
#endif

    while(true) {
        libatari800_next_frame(&input_map);
        sleep_us(5);
    }

    __unreachable();
}
