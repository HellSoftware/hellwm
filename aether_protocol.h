// Define this in a new shared header file, e.g., "aether_protocol.h"
#include <stdint.h>
#include <linux/input-event-codes.h>

enum aether_event_type {
    AETHER_EVENT_KEY,
    AETHER_EVENT_MOUSE_MOTION_REL,
    AETHER_EVENT_MOUSE_BUTTON,
    AETHER_EVENT_MOUSE_SCROLL,
};

struct aether_event {
    uint8_t type; // From aether_event_type enum
    uint8_t padding[3];
    int32_t value1;
    int32_t value2;
};

// Usage examples:
// Key press: { .type = AETHER_EVENT_KEY, .value1 = BTN_A, .value2 = 1 (pressed) }
// Mouse move: { .type = AETHER_EVENT_MOUSE_MOTION_REL, .value1 = dx, .value2 = dy }
// Scroll: { .type = AETHER_EVENT_MOUSE_SCROLL, .value1 = axis, .value2 = value }
