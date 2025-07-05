## sdat2img written in C

This program is based on [sdat2img](https://github.com/xpirt/sdat2img) , which was written in Python and was translated into C with the assistance of AI and is provided AS IS and without warranty.  All testing has been done using Clang 14.0.6 on Debian 12 "Bookworm".

## **Usage**

```
sdat2img <transfer_list> <system_new_file> [system_img]
```

*   `<transfer_list>` = input, system.transfer.list from rom zip
*   `<system_new_file>` = input, system.new.dat from rom zip
*   `[system_img]` = output ext4 raw image file (optional)

## **Example**

This is a simple example on a Linux system:

```bash
~$ ./sdat2img system.transfer.list system.new.dat system.img
```
