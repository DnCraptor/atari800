#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"
#include "usbhid.h"

extern bool handleScancode(uint32_t ps2scancode);

/*
 * HID usage (keyboard page) -> XT set 1 make code, exactly as the PS/2
 * driver (ps2getcode) delivers it: extended keys come without the E0
 * prefix, a release is the make code | 0x80.
 */
static const uint8_t hid_to_xt[256] = {
    [HID_KEY_A] = 0x1E, [HID_KEY_B] = 0x30, [HID_KEY_C] = 0x2E, [HID_KEY_D] = 0x20,
    [HID_KEY_E] = 0x12, [HID_KEY_F] = 0x21, [HID_KEY_G] = 0x22, [HID_KEY_H] = 0x23,
    [HID_KEY_I] = 0x17, [HID_KEY_J] = 0x24, [HID_KEY_K] = 0x25, [HID_KEY_L] = 0x26,
    [HID_KEY_M] = 0x32, [HID_KEY_N] = 0x31, [HID_KEY_O] = 0x18, [HID_KEY_P] = 0x19,
    [HID_KEY_Q] = 0x10, [HID_KEY_R] = 0x13, [HID_KEY_S] = 0x1F, [HID_KEY_T] = 0x14,
    [HID_KEY_U] = 0x16, [HID_KEY_V] = 0x2F, [HID_KEY_W] = 0x11, [HID_KEY_X] = 0x2D,
    [HID_KEY_Y] = 0x15, [HID_KEY_Z] = 0x2C,
    [HID_KEY_1] = 0x02, [HID_KEY_2] = 0x03, [HID_KEY_3] = 0x04, [HID_KEY_4] = 0x05,
    [HID_KEY_5] = 0x06, [HID_KEY_6] = 0x07, [HID_KEY_7] = 0x08, [HID_KEY_8] = 0x09,
    [HID_KEY_9] = 0x0A, [HID_KEY_0] = 0x0B,
    [HID_KEY_ENTER] = 0x1C, [HID_KEY_ESCAPE] = 0x01, [HID_KEY_BACKSPACE] = 0x0E,
    [HID_KEY_TAB] = 0x0F, [HID_KEY_SPACE] = 0x39, [HID_KEY_MINUS] = 0x0C,
    [HID_KEY_EQUAL] = 0x0D, [HID_KEY_BRACKET_LEFT] = 0x1A, [HID_KEY_BRACKET_RIGHT] = 0x1B,
    [HID_KEY_BACKSLASH] = 0x2B, [HID_KEY_EUROPE_1] = 0x2B, [HID_KEY_SEMICOLON] = 0x27,
    [HID_KEY_APOSTROPHE] = 0x28, [HID_KEY_GRAVE] = 0x29, [HID_KEY_COMMA] = 0x33,
    [HID_KEY_PERIOD] = 0x34, [HID_KEY_SLASH] = 0x35, [HID_KEY_CAPS_LOCK] = 0x3A,
    [HID_KEY_F1] = 0x3B, [HID_KEY_F2] = 0x3C, [HID_KEY_F3] = 0x3D, [HID_KEY_F4] = 0x3E,
    [HID_KEY_F5] = 0x3F, [HID_KEY_F6] = 0x40, [HID_KEY_F7] = 0x41, [HID_KEY_F8] = 0x42,
    [HID_KEY_F9] = 0x43, [HID_KEY_F10] = 0x44, [HID_KEY_F11] = 0x57, [HID_KEY_F12] = 0x58,
    [HID_KEY_SCROLL_LOCK] = 0x46,
    /* extended keys: same codes as the PS/2 path (E0 prefix dropped) */
    [HID_KEY_INSERT] = 0x52, [HID_KEY_HOME] = 0x47, [HID_KEY_PAGE_UP] = 0x49,
    [HID_KEY_DELETE] = 0x53, [HID_KEY_END] = 0x4F, [HID_KEY_PAGE_DOWN] = 0x51,
    [HID_KEY_ARROW_RIGHT] = 0x4D, [HID_KEY_ARROW_LEFT] = 0x4B,
    [HID_KEY_ARROW_DOWN] = 0x50, [HID_KEY_ARROW_UP] = 0x48,
    [HID_KEY_NUM_LOCK] = 0x45, [HID_KEY_KEYPAD_DIVIDE] = 0x35,
    [HID_KEY_KEYPAD_MULTIPLY] = 0x37, [HID_KEY_KEYPAD_SUBTRACT] = 0x4A,
    [HID_KEY_KEYPAD_ADD] = 0x4E, [HID_KEY_KEYPAD_ENTER] = 0x1C,
    [HID_KEY_KEYPAD_1] = 0x4F, [HID_KEY_KEYPAD_2] = 0x50, [HID_KEY_KEYPAD_3] = 0x51,
    [HID_KEY_KEYPAD_4] = 0x4B, [HID_KEY_KEYPAD_5] = 0x4C, [HID_KEY_KEYPAD_6] = 0x4D,
    [HID_KEY_KEYPAD_7] = 0x47, [HID_KEY_KEYPAD_8] = 0x48, [HID_KEY_KEYPAD_9] = 0x49,
    [HID_KEY_KEYPAD_0] = 0x52, [HID_KEY_KEYPAD_DECIMAL] = 0x53,
    [HID_KEY_APPLICATION] = 0x5D,
};

/* modifier bit (KEYBOARD_MODIFIER_*) -> XT make code */
static const uint8_t mod_to_xt[8] = {
    0x1D, /* left ctrl   */
    0x2A, /* left shift  */
    0x38, /* left alt    */
    0x5B, /* left gui    */
    0x1D, /* right ctrl  */
    0x36, /* right shift */
    0x38, /* right alt   */
    0x5C, /* right gui   */
};

static hid_keyboard_report_t prev_report;

static bool report_has_key(hid_keyboard_report_t const *r, uint8_t key) {
    for (int i = 0; i < 6; i++)
        if (r->keycode[i] == key)
            return true;
    return false;
}

static void send_xt(uint8_t code, bool release) {
    if (code)
        handleScancode(release ? (uint32_t)(code | 0x80) : (uint32_t)code);
}

/* Pause/Break: dedicated codes understood by handleScancode() (Atari BREAK) */
static void send_pause(bool release) {
    handleScancode(release ? 0xE19D : 0xE11D);
}

static void process_kbd_report(hid_keyboard_report_t const *r) {
    uint8_t changed = r->modifier ^ prev_report.modifier;
    for (int b = 0; b < 8; b++) {
        if (changed & (1u << b))
            send_xt(mod_to_xt[b], !(r->modifier & (1u << b)));
    }
    /* releases first, then presses */
    for (int i = 0; i < 6; i++) {
        uint8_t k = prev_report.keycode[i];
        if (k && !report_has_key(r, k)) {
            if (k == HID_KEY_PAUSE) send_pause(true);
            else send_xt(hid_to_xt[k], true);
        }
    }
    for (int i = 0; i < 6; i++) {
        uint8_t k = r->keycode[i];
        if (k > 3 /* 1..3: rollover/POST/undefined errors */ && !report_has_key(&prev_report, k)) {
            if (k == HID_KEY_PAUSE) send_pause(false);
            else send_xt(hid_to_xt[k], false);
        }
    }
    prev_report = *r;
}

#define MAX_REPORT 4
static struct {
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    if (instance < CFG_TUH_HID &&
        tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_NONE) {
        hid_info[instance].report_count =
            tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr; (void)instance;
    /* release everything that was held on the unplugged keyboard */
    hid_keyboard_report_t empty;
    memset(&empty, 0, sizeof(empty));
    process_kbd_report(&empty);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    uint8_t const protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        if (len >= sizeof(hid_keyboard_report_t))
            process_kbd_report((hid_keyboard_report_t const *)report);
    } else if (protocol == HID_ITF_PROTOCOL_NONE && instance < CFG_TUH_HID) {
        /* report-protocol keyboard: find the keyboard report by usage */
        uint8_t const rpt_count = hid_info[instance].report_count;
        tuh_hid_report_info_t *rpt_info_arr = hid_info[instance].report_info;
        tuh_hid_report_info_t *rpt_info = NULL;
        if (rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
            rpt_info = &rpt_info_arr[0];
        } else if (len > 0) {
            uint8_t const rpt_id = report[0];
            for (uint8_t i = 0; i < rpt_count; i++) {
                if (rpt_id == rpt_info_arr[i].report_id) {
                    rpt_info = &rpt_info_arr[i];
                    break;
                }
            }
            report++;
            len--;
        }
        if (rpt_info && rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP &&
            rpt_info->usage == HID_USAGE_DESKTOP_KEYBOARD &&
            len >= sizeof(hid_keyboard_report_t)) {
            process_kbd_report((hid_keyboard_report_t const *)report);
        }
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void usbhid_init(void) {
    memset(&prev_report, 0, sizeof(prev_report));
    tuh_init(BOARD_TUH_RHPORT);
}

void usbhid_task(void) {
    tuh_task();
}
