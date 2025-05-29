#include "j1939.h"

#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s == -1) {
        // error
        perror(NULL);
    }

    struct sockaddr_can addr;
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
        perror(NULL);
    }

    struct J1939 j1939;
    struct J1939Msg msg;

    struct can_frame frame;
    ssize_t nbytes = 0;
    while (1) {
        nbytes = read(s, &frame, sizeof(frame));

        if (nbytes > 0) {
            printf("received ID: %X\n", frame.can_id);
            j1939_rx(&j1939, &msg, frame.can_id, frame.data, frame.len);
        }
    }

    return 0;
}
