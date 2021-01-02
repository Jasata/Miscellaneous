/*
 * TEMPer.c - Simple TEMPer USB temperature meter reader
 * Adapted from https://gist.github.com/artms/5356eafcd1244c6fabc0f735e5de7096
 * Credit: Arturas Moskvinas
 *
 * Adapted to C by Jani Tammi <jasata@utu.fi>, 2020-10-11
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


// TEMPer command
// echo -e '\x00\x01\x80\x33\x01\x00\x00\x00\x00\c' >&5
static const char READCMD[] = {
    0x00, 0x01, 0x80, 0x33,
    0x01, 0x00, 0x00, 0x00,
    0x00
};
uint8_t result[8];


int main(int argc, char **argv)
{
    ssize_t nbytes;
    uint16_t temp;

    if (argc != 2)
    {
        printf("Usage: temper </dev/hidraw[n]>\n");
        return -1;
    }

    //  | O_NONBLOCK  - read cannot be non-blocking
    int fd = open(argv[1], O_RDWR);
    nbytes = write(fd, READCMD, sizeof(READCMD));
    if (nbytes == 9)
    {
        nbytes = read(fd, &result, sizeof(result));
        if (nbytes == sizeof(result))
        {
            /*
             * Bytes #2 and #3 (16-bit) value returns the temperature
             * in Celcius x100
             */
            printf(
                "%3.2f\n",
                 (float)((uint16_t)result[2] << 8 | result[3]) / 100
            );
            return 0;
        }
        else
        {
            perror("Faulty read from device!");
            return -1;
        }

    }
    else
    {
        perror("Short write to device! (root privileges required)");
        printf(
            "ERROR: Short write to device '%s' (%d bytes)\n",
            argv[1],
            nbytes
        );
        return -1;
    }
}

// EOF
