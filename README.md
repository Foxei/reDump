# reDump

Welcome to **reDump** this is a small executable written for the [reMarkable 2](https://remarkable.com/). This executable can be used to dump the framebuffer and parse it using script like [reStream](https://github.com/rien/reStream) or [rmView](https://github.com/bordaigorl/rmview). In comparrison to the system solutions like `dd` with around (2-3 fps) this exectuable can dump with around (12-15 fps). Both numbers including the compression time with `lz4` that is not performed by reDump.

# Usage with reStream

Clone the [feature branche](https://github.com/rien/reStream/tree/feature/support-reMarkable-2) of the [reStream](https://github.com/rien/reStream) project and this repository.
```bash
git clone git@github.com:rien/reStream.git
git clone git@github.com:Foxei/reDump.git
cd reStream
checkout feature/support-reMarkable-2
```

Go throug the usuall setup process of the [reStream](https://github.com/rien/reStream) project.

Copy the precompile `reDump` executbale on you reMarkable and make it executable.
```bash
cd reDump
cd bin
scp ./reDump.remarkable.shared root@10.11.99.1:/home/root/reDump
ssh root@10.11.99.1 'chmod +x /home/root/reDump'
```

Open the `reStream.sh` from the [reStream](https://github.com/rien/reStream) prject and replace the folling lines:

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

Done use reStream as normal.
