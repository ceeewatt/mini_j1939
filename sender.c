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
    frame.can_id = 0x18FE3455;
    frame.len = 8;
    uint64_t data = 0x1122334455667788;
    memcpy(frame.data, (uint8_t*)&data, 8);

    while (true)
    {
        printf("Press enter to send a CAN frame\n");
        getchar();

        if (write(sockfd, &frame, sizeof(struct can_frame)))
            perror("write()");
        else
            printf("sent CAN frame\n");
    }

    close(sockfd);
    return 0;
}
