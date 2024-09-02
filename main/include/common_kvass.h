#pragma once

// MAX count of notifications is 32!
#define NOTIF_HID_CHANGED 0x1
#define NOTIF_KEYB_CHANGED 0x2
#define NOTIF_MOUSE_CHANGED 0x4
#define NOTIF_PROTOCOL_CHANGED 0x8


struct GodParameters
{
    void *gpioParameters;
    void *commsParameters;
    void *interconnectParameters;
};