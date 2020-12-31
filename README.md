# reDump

Welcome to **reDump** 
This is a small executable written for the [reMarkable 2](https://remarkable.com/). This executable can be used to dump the framebuffer and parse it using scripts like [reStream](https://github.com/rien/reStream) or [rmView](https://github.com/bordaigorl/rmview). In comparison to  system solutions like `dd` with around (2-3 fps) this exectuable can dump with around (12-15 fps). Both numbers include the compression time with `lz4` which is not performed by reDump.

# Usage with reStream

Clone the [feature branche](https://github.com/rien/reStream/tree/feature/support-reMarkable-2) of the [reStream](https://github.com/rien/reStream) project and this repository.
```bash
git clone git@github.com:rien/reStream.git
git clone git@github.com:Foxei/reDump.git
cd reStream
checkout feature/support-reMarkable-2
```

Go throug the usual setup process of the [reStream](https://github.com/rien/reStream) project.

Copy the precompile `reDump` executbale on you reMarkable and make it executable.
```bash
cd reDump
cd bin
scp ./reDump.remarkable.shared root@10.11.99.1:/home/root/reDump
ssh root@10.11.99.1 'chmod +x /home/root/reDump'
```

Open the `reStream.sh` from the [reStream](https://github.com/rien/reStream) project and replace the following lines:

```bash
skip_bytes="$((0x$skip_bytes_hex + 8))"
echo "framebuffer is at 0x$skip_bytes_hex"

# carve the framebuffer out of the process memory
page_size=4096
window_start_blocks="$((skip_bytes / page_size))"
window_offset="$((skip_bytes % page_size))"
window_length_blocks="$((window_bytes / page_size + 1))"

# Using dd with bs=1 is too slow, so we first carve out the pages our desired
# bytes are located in, and then we trim the resulting data with what we need.
head_fb0="dd if=/proc/$pid/mem bs=$page_size skip=$window_start_blocks count=$window_length_blocks 2>/dev/null | tail -c+$window_offset | head -c $window_bytes"
```
By the patch to use `reDump`:
```bash
# Save framebuffer location
framebuffer_addresse="$((0x$skip_bytes_hex))"
echo "framebuffer is at 0x$framebuffer_addresse"

# carve the framebuffer out of the process memory
window_offset=8

# Use the faster reDump insted of dd, head and tail
head_fb0="/home/root/reDump -p $pid -b $framebuffer_addresse -s $window_offset -c $window_bytes"
```

**Optional** Fix rotation.

Add a `transpose=2` to the video filters.

```bash
video_filters="$video_filters,transpose=2"
```

Done! Now you can use reStream!

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

