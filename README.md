# reDump

Welcome to **reDump** 
This is a small executable written for the [reMarkable 2](https://remarkable.com/). This executable can be used to dump the framebuffer and parse it using scripts like [reStream](https://github.com/rien/reStream) or [rmView](https://github.com/bordaigorl/rmview). In comparison to  system solutions like `dd` with around (2-3 fps) this exectuable can dump with around (12-15 fps). Both numbers include the compression time with `lz4` which is not performed by reDump.

# Usage with reStream

See my fork of the [reStream](https://github.com/Foxei/reStream) project.
```bash
git clone git@github.com:Foxei/reStream.git
```

# What do I know about the rM2 framebuffer

Most of the information I will present here is a collection of information by different users and projects. Namingly the [rmView](https://github.com/bordaigorl/rmview) and [reStream](https://github.com/rien/reStream) project.
The goal to a successfull stream is locating the framebuffer (the image that is displayed on the reMarkables screen) within the memory of the `xochitl` (main reMarkable software) application.
In the following I explain how reDump accomplishs that. 

## How is the framebuffer located inside the memory?

The frame buffer is located in its own memory region and is stored consecutively.
The memory regions allocated by a program are mapped inside the `/proc/pid/maps` file.
Some smart person found out that the exact address can be extracted with the following shell command.
Note: `pid` stands for the process id of `xochitl`.

```bash
# it is actually the map allocated _after_ the fb0 mmap
read_address="grep -C1 '/dev/fb0' /proc/$pid/maps | tail -n1 | sed 's/-.*$//'"
framebuffer_addresse_hex="$(ssh_cmd "$read_address")"
# Save framebuffer location
framebuffer_addresse="$((0x$framebuffer_addresse_hex))"
```

## How to access the memory?

With the address known it is now the task to locate the the memory region inside the `/proc/pid/mem` file that matches this address. There for just iterate over all starting addresses of regions until we find it. The problem is that it is not aligned with the default memory page size in the rM2. The default page size is 4096 but the buffer is shifted by 8 bytes (I have not understood why this is. If you know that let me know as well. :wink: ). To compensate that just shift the start address by 8 bytes before dumping. 

You may know that accessing the `/proc/pid` directory is not generally allowed by the Linux kernel. Obviously… To get access to the directory we have to trail the process (As I understand we hook into the process as a subprocess). Do not forget to detach from the process afterwards and close all open file stream. 

## How much to dump?

Now we need know exactly the number of bytes that should be dumped. This number is calculated from the known image format and the pixel resolution. The rM1 uses 1408x1872 pixel with Grayscale 16 and the rM2 uses 1404x1872 pixel with Grayscale 8. Note: The rM2 stores the image upside down if compared to the rM2.

Now we know how much pixels to dump and where the frame buffer is located. The rest is straight C with a little touch of C++. Done! 

