# devices
A device is a special type of file that acts as a portal between User Space and the Kernel's Device Drivers. The udev system enables userspace programs to automatically configure and use new devices

Device files have a prefix b,c,s,p denoting block, character, socket and pipe devices

**Block device** - Like disks, which can store fixed data in blocks. Easy to index and lseek()
**Character Device** - Work with data streams, e.g input devices. Data cannot be reexamined after it has passed
**Pipe devices** -  Just like character devices, but instead of involving kernel drivers, they have a process to receive the input on the other end

** Not all devices have device files

## The sysfs device path
- when you examine /dev across reboots, the same device might have a different name, as they are assigned the names in order of discovery. Linux provides a uniform view of devices based on their hardware attributes thru the sysfs interface.
- The base path for devices is /sys/devices
- the /dev path is enables userspace processes to use the device, but the /sys/devices path is used to manage the device, and view more info about it.
Use the command `udevadm info --query=all --name=/dev/sda` to find the /sys path and other attributes

Example in C: Reading from a keyboard
```c
{% include-markdown "./read_keyboard.c" %}
```