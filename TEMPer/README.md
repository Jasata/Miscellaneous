# TEMPer USB temperature sensor (on Linux)

![TEMPer USB Adapter](https://github.com/Jasata/Miscellaneous/blob/main/TEMPer/TEMPer.jpg?raw=true)

These are Chinese-made items that can be bought from eBay, DX.com, or AliExpress for $5 ... $10 / ea.

## Source in this directory

The C source in this directory assumes that the TEMPer is recognized by your Linux system and that a device file for it is available (`/dev/hidraw[n]`). This solution avoids installing HIDAPI libraries by sending a binary command (found from Maikel Van Leeuwen's blog, see links below) and reading the bytes that encode the temperature.

## Usage in Bash scripts

### Step 1: Look up the device USB ID

It appears that multiple different hardwares are sold simply as "TEMPer" devices. These can be differentiated by the different USB ID’s. It also appears that USB devices with different USB ID's do not necessarily work with the software found on the Internet.

    $ lsusb
    Bus 001 Device 004: ID 413d:2107
    ...

You should verity this with `dmesg`:

$ dmesg

    [2284450.179125] usb 1-1.4: new full-speed USB device number 4 using dwc_otg
    [2284450.313102] usb 1-1.4: New USB device found, idVendor=413d, idProduct=2107, bcdDevice= 0.00
    [2284450.313130] usb 1-1.4: New USB device strings: Mfr=0, Product=0, SerialNumber=0
    [2284450.329975] input: HID 413d:2107 as /devices/platform/soc/3f980000.usb/usb1/1-1/1-1.4/1-1.4:1.0/0003:413D:2107.0001/input/input0
    [2284450.400894] hid-generic 0003:413D:2107.0001: input,hidraw0: USB HID v1.11 Keyboard [HID 413d:2107] on usb-3f980000.usb-1.4/input0
    [2284450.406706] hid-generic 0003:413D:2107.0002: hiddev96,hidraw1: USB HID v1.10 Device [HID 413d:2107] on usb-3f980000.usb-1.4/input1

In this case, we can be sure that this Linux recognizes thec connected TEMPer device.

### Step 2: Access without HIDAPI

```bash
#!/bin/bash
exec 5<> /dev/hidraw1
echo -e '\x00\x01\x80\x33\x01\x00\x00\x00\x00\c' >&5
OUT=$(dd count=1 bs=8 <&5 2>/dev/null | xxd -p)
HEX4=${OUT:4:4}
DVAL=$((16#$HEX4))
CTEMP=$(bc <<< "scale=2; $DVAL/100")
echo $CTEMP
```

**Line 2**: _Assumption_ is that `exec` creates bidirectional stream 5.  
**Line 3**: Option '-e' enables `echo` to interpret backslash escapes. '\c' keeps the cursor on the same line after the end of the echo (requires '-e' option).  
**Line 4**: `dd` reads 8 bytes from steam 5. `xxd` converts binary to string representation, '-p' option removes address and ASCII character dump.  

    $ echo -e '\x00\x02\c' | xxd -p
    0002

**Line 5**: Bytes 2 and 3 are extracted from OUT string (16 character-string, 8 bytes as string). `8899AABBCCDDEEFF` -> `AABB `  
**Line 6**: Hex string is converted into decima string.  
**Line 7**: Utility `bc` (Basic Calculator) is used to divide the value by 100.  

## Using HIDAPI

### Step 1: Get the developer Distro packages needed to compile the TEMPer software

    $ sudo apt install hidapi-devel bc

### Step 2: Clone the needed software

    $ sudo -i
    $ cd /opt/
    $ git clone https://github.com/edorfaus/TEMPered
    $ cd TEMPered
    $ cmake .
    $ cd /opt/TEMPered/utils
    $ make
    $ cp hid-query /usr/local/bin

### Find out which hidraw device is our TEMpered device by trying to get metrics out of it

    $ ./hid-query /dev/hidraw1 0x01 0x80 0x33 0x01 0x00 0x00 0x00 0x00
    No data was read from the device (timeout).
    
    $ ./hid-query /dev/hidraw2 0x01 0x80 0x33 0x01 0x00 0x00 0x00 0x00
    Device /dev/hidraw2 : 413d:2107 interface 1 : (null) (null)
    
    Writing data (9 bytes):
    
    00 01 80 33   01 00 00 00 00
    
    Response from device (8 bytes):
    
    80 80 0a 21   4e 20 00 00

In this example, TEMPered device is located at `/dev/hidraw2`

### Create TEMPered script

```bash
#!/bin/bash
OUTLINE=”/usr/local/bin/hid-query /dev/hidraw2 0x01 0x80 0x33 0x01 0x00 0x00 0x00 0x00|grep -A1 ^Response|tail -1″
OUTNUM=”echo $OUTLINE|sed -e ‘s/^[^0-9a-f]*[0-9a-f][0-9a-f] [0-9a-f][0-9a-f] \([0-9a-f][0-9a-f]\) \([0-9a-f][0-9a-f]\) .*$/0x\1\2/'”
HEX4=${OUTNUM:2:4}
DVAL=$(( 16#$HEX4 ))
CTEMP=$(bc <<< “scale=2; $DVAL/100”)
echo $CTEMP
```

    $ chmod +x TEMPered

Test the script:

    $ ./TEMPered
    
    25.93

Even with HIDAPI, this is a lot of work - not worth installing the HIDAPI. Equally unfortunate is the fact that the script needs to be executed as a root. This could be avoided with an udev rule:

    SUBSYSTEMS==”usb”, ACTION==”add”, ATTRS{idVendor}==”413d”, ATTRS{idProduct}==”2107″, MODE=”666″

The above should work for Debian 9 systems. (Write it into `/etc/udev/rules.d/99-tempsensor.rules`).

## Links

  * [TEMPer (Temperature sensor 413d:2107) on Linux tutorial](https://maikel.tiny-host.nl/it-related-articles-english-dutch/temper-temperature-sensor-413d2107-on-linux/) by Maikel Van Leeuwen, September 2019
