#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include "aether_protocol.h" // Your shared header

// Function to set up and create the virtual device
struct libevdev_uinput* setup_uinput_device() {
    struct libevdev *dev = libevdev_new();
    libevdev_set_name(dev, "Aether Portal Device");

    // Enable event types
    libevdev_enable_event_type(dev, EV_REL);
    libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
    libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);
    libevdev_enable_event_code(dev, EV_REL, REL_WHEEL, NULL);

    libevdev_enable_event_type(dev, EV_KEY);
    // Enable all possible keys
    for (int i = 0; i < KEY_MAX; ++i) {
        libevdev_enable_event_code(dev, EV_KEY, i, NULL);
    }

    struct libevdev_uinput *uidev;
    int err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
    if (err != 0) {
        // error handling
        return NULL;
    }
    libevdev_free(dev);
    return uidev;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    struct libevdev_uinput *uidev = setup_uinput_device();
    if (!uidev) {
        fprintf(stderr, "Failed to create uinput device. Are you root?\n");
        return 1;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("Aether Portal client listening on port %d\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd < 0) {
        perror("accept");
        return 1;
    }

    printf("Server connected!\n");

    // --- Main Event Loop ---
    struct aether_event net_event;
    while (read(conn_fd, &net_event, sizeof(net_event)) > 0) {
        switch(net_event.type) {
            case AETHER_EVENT_MOUSE_MOTION_REL:
                libevdev_uinput_write_event(uidev, EV_REL, REL_X, net_event.value1);
                libevdev_uinput_write_event(uidev, EV_REL, REL_Y, net_event.value2);
                break;
            case AETHER_EVENT_KEY:
                libevdev_uinput_write_event(uidev, EV_KEY, net_event.value1, net_event.value2); // Add 8 for libinput->evdev offset
                break;
            case AETHER_EVENT_MOUSE_BUTTON:
                // value1 = button code (e.g., BTN_LEFT), value2 = 1 (press) or 0 (release)
                libevdev_uinput_write_event(uidev, EV_KEY, net_event.value1, net_event.value2);
                break;

            case AETHER_EVENT_MOUSE_SCROLL:
                // value1 = direction (0 = vertical, 1 = horizontal), value2 = amount
                if (net_event.value1 == 0)
                {
                    libevdev_uinput_write_event(uidev, EV_REL, REL_WHEEL, (float)net_event.value2);
                }
                else if (net_event.value1 == 1)
                {
                    libevdev_uinput_write_event(uidev, EV_REL, REL_HWHEEL, (float)net_event.value2);
                }
                break;
        }
        // Tell the kernel we've finished this group of events
        libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
    }

    printf("Server disconnected. Exiting.\n");
    close(conn_fd);
    libevdev_uinput_destroy(uidev);
    return 0;
}
