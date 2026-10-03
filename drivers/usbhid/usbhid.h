#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* USB HID keyboard (TinyUSB host). Key events are translated to the same
   XT scancodes the PS/2 driver produces and passed to handleScancode(). */
void usbhid_init(void);
void usbhid_task(void);

#ifdef __cplusplus
}
#endif
