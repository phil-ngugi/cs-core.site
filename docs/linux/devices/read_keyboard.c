// get keyboard devices
#include <libudev.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct udev *udev = udev_new();

    struct udev_enumerate *e = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(e, "input");
    udev_enumerate_scan_devices(e);

    struct udev_list_entry *devices =
        udev_enumerate_get_list_entry(e);

    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);

        struct udev_device *dev =
            udev_device_new_from_syspath(udev, path);

        const char *is_kbd =
            udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");

        if (is_kbd && strcmp(is_kbd, "1") == 0) {
            const char *device = udev_device_get_devnode(dev);

            if (device)
                // 
                int fd = open(device, O_RDONLY);
                    if (fd < 0) {
                        perror("open");
                        return 1;
                    }

                    struct input_event ev;

                    while (1) {
                        ssize_t n = read(fd, &ev, sizeof(ev));

                        if (n == sizeof(ev)) {

                            // Only key events
                            if (ev.type == EV_KEY) {

                                // ev.value:
                                // 1 = key press
                                // 0 = key release
                                // 2 = hold (auto-repeat)

                                if (ev.value == 1) {
                                    printf("Key code: %d pressed\n", ev.code);
                                }
                            }
                        }
                    }
                // 
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(e);
    udev_unref(udev);
}