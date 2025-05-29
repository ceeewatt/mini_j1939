#include "j1939_app.h"

#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

static bool physical_rx(struct J1939CanFrame* frame);
static bool physical_tx(struct J1939Msg* msg);
static void app_rx(struct J1939Msg* msg);
static void print_j1939_msg(struct J1939Msg* msg);
static void startup_delay(void* param);

int sockfd;

void
j1939_app_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    const char* device_name,
    int tick_rate_ms)
{
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0)
        perror("socket()");

    // For non-blocking reads
    fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK));

    struct sockaddr_can addr;
    struct ifreq ifr;
    strcpy(ifr.ifr_name, device_name);
    ioctl(sockfd, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)))
        perror("bind()");

    j1939_init(
        node,
        name,
        physical_rx,
        physical_tx,
        app_rx,
        startup_delay,
        NULL,
        preferred_address,
        tick_rate_ms);
}

static bool
physical_rx(
    struct J1939CanFrame* jcanframe)
{
    struct can_frame frame;
    ssize_t nbytes = 0;
    nbytes = read(sockfd, &frame, sizeof(struct can_frame));

    if (nbytes < 0)
    {
        if (errno != EWOULDBLOCK) 
            perror("read()");

        return false;
    }

    if (nbytes < sizeof(struct can_frame))
        return false;

    jcanframe->can_id = frame.can_id;
    memcpy(jcanframe->data, frame.data, frame.len);
    jcanframe->len = frame.len;

    return true;
}

static bool
physical_tx(
    struct J1939Msg* msg)
{
    struct can_frame frame;

    uint32_t can_id = 0;
    uint8_t dp = (msg->pgn >> 16) & 0x01;
    uint8_t pf = (msg->pgn >> 8) & 0xFF;

    uint8_t ps;
    if (msg->dst != J1939_ADDR_GLOBAL)
    {
        ps = msg->dst;
    }
    else
    {
        if (pf < 240)
            ps = J1939_ADDR_GLOBAL;
        else
            ps = msg->pgn & 0xFF;
    }

    can_id |=
        (1 << 31) |
        (msg->pri << 26) |
        (dp << 24) |
        (pf << 16) |
        (ps << 8) |
        msg->src;

    frame.can_id = can_id;
    frame.len = msg->len;
    memcpy(frame.data, msg->data, frame.len);

    ssize_t nbytes = write(sockfd, &frame, sizeof(struct can_frame));

    if (nbytes < sizeof(struct can_frame))
    {
        perror("write()");
        return false;
    }
    else
    {
        return true;
    }
}

static void
app_rx(
    struct J1939Msg* msg)
{
    print_j1939_msg(msg);
}

static void
print_j1939_msg(
    struct J1939Msg* msg)
{
    printf("0x%06X  %d  0x%02X  0x%02X  [%d] ",
        msg->pgn, msg->pri, msg->src, msg->dst, msg->len);

    for (int i = 0; i < msg->len; i++)
    {
        printf("%02X ", msg->data[i]);
    }
    printf("\n");
}

static
void startup_delay(
    void* param)
{
    const int delay_ms = 250;
    usleep(250 * 1000);
}
