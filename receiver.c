#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

int main(void)
{
    const char* device = "vcan0";

    int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0)
        perror("socket()");

    // For non-blocking reads
    fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK));

    struct sockaddr_can addr;
    struct ifreq ifr;
    strcpy(ifr.ifr_name, device);
    ioctl(sockfd, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_can)))
        perror("bind()");

    struct can_frame frame;
    ssize_t nbytes = 0;

    while (true)
    {
        nbytes = read(sockfd, &frame, sizeof(struct can_frame));

        if (nbytes < 0)
        {
            if (errno != EWOULDBLOCK)
                perror("read()");
            else
                continue;
        }
        else
        {
            printf("Received: 0x%06X [%d] ", frame.can_id, frame.len);
            for (int i = 0; i < frame.len; ++i) {
                printf("%02X ", frame.data[i]);
            }
            printf("\n");
        }
    }

    close(sockfd);
    return 0;
}
