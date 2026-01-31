#include <hardware/clocks.h>
#include <ctype.h>
#include "platform.h"
#include "screen.h"
#include "input.h"
#include "akey.h"
#include "util.h"
#include "libatari800/libatari800.h"
#include <pico/time.h>

UBYTE __aligned(4) __screen[Screen_HEIGHT * Screen_WIDTH];
input_template_t input_map;

int stricmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

double PLATFORM_Time(void) {
	return time_us_64() * 1e-6;
}

static void autoframeskip(double curtime, double lasttime)
{
	static int afs_lastframe = 0, afs_discard = 0;
	static double afs_lasttime = 0.0, afs_sleeptime = 0.0;
	double afs_speedpct, afs_sleeppct, afs_ataritime, afs_realtime;

	if (lasttime - curtime > 0)
		afs_sleeptime += lasttime - curtime;
	if (curtime - afs_lasttime > 0.5) {
		afs_ataritime = ((double) (Atari800_nframes - afs_lastframe)) /
						((double) (Atari800_tv_mode == Atari800_TV_PAL ? Atari800_FPS_PAL : Atari800_FPS_NTSC));
		afs_realtime = curtime - afs_lasttime;
		afs_speedpct = 100.0 * afs_ataritime / afs_realtime;
		afs_sleeppct = 100.0 * afs_sleeptime / afs_realtime;

		if (afs_discard < 3 && (afs_realtime > 2.0 * afs_ataritime)) {
			afs_discard++;
		} else {
			afs_discard = 0;
			if (afs_speedpct < 90.0) {
				if (Atari800_refresh_rate < 4)
					Atari800_refresh_rate++;
			} else {
				if (afs_sleeppct > 20.0 && Atari800_refresh_rate > 1)
					Atari800_refresh_rate--;
			}
		}

		afs_sleeptime = 0.0;
		afs_lastframe = Atari800_nframes;
		afs_lasttime = Util_time();
	}
}

/// @brief  TODO: 
/// @param  
void Atari800_Sync(void) {
	static double lasttime = 0;
	double deltatime = 1.0 / ((Atari800_tv_mode == Atari800_TV_PAL) ? Atari800_FPS_PAL : Atari800_FPS_NTSC);
	double curtime;
	//printf("Atari800_Sync");
#ifdef SYNCHRONIZED_SOUND
	deltatime *= Sound_AdjustSpeed();
#endif
#ifdef ALTERNATE_SYNC_WITH_HOST
	if (! UI_is_active)
		deltatime *= Atari800_refresh_rate;
#endif
	lasttime += deltatime;
	curtime = Util_time();
	if (Atari800_auto_frameskip)
		autoframeskip(curtime, lasttime);
	Util_sleep(lasttime - curtime);
	curtime = Util_time();

	printf("frame dt=%.6f sleep=%.6f\n", curtime - lasttime, sleeptime);

	if ((lasttime + deltatime) < curtime)
		lasttime = curtime;
}

void LIBATARI800_Frame(void) {
	switch (INPUT_key_code) {
	case AKEY_COLDSTART:
		Atari800_Coldstart();
		break;
	case AKEY_WARMSTART:
		Atari800_Warmstart();
		break;
	case AKEY_UI:
#ifdef SOUND
		Sound_Pause();
#endif
		UI_Run();
#ifdef SOUND
		Sound_Continue();
#endif
		break;
	default:
		break;
	}

#ifdef PBI_BB
	PBI_BB_Frame(); /* just to make the menu key go up automatically */
#endif
#if defined(PBI_XLD) || defined (VOICEBOX)
	VOTRAXSND_Frame(); /* for the Votrax */
#endif
	Devices_Frame();
	INPUT_Frame();
	GTIA_Frame();
	ANTIC_Frame(TRUE);
	INPUT_DrawMousePointer();
	Screen_DrawAtariSpeed(Util_time());
	Screen_DrawDiskLED();
	Screen_Draw1200LED();
	POKEY_Frame();
#ifdef SOUND
	Sound_Update();
#endif
	Atari800_nframes++;
	Atari800_Sync();
}

int PLATFORM_Configure(char *option, char *parameters)
{
    return LIBATARI800_ReadConfig(option, parameters);
}

void PLATFORM_Sleep(double s) {
    if (s <= 0)
        return;
    uint64_t us = (uint64_t)(s * 1000000.0 + 0.5);
    if (us > 0)
        sleep_us(us);
}

void PLATFORM_ConfigInit(void) {
}

int PLATFORM_Initialise(int *argc, char *argv[]) {
}
