#include "j1939_transport_protocol.h"

void
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt)
{
    if (dt->seq != tp->next_seq)
        return;

    uint8_t* buf = tp->buf + ((tp->next_seq - 1) * 7);
    uint8_t* data = (uint8_t*)&dt->data0;
    int bytes_to_copy = (tp->bytes_rem < 7) ? tp->bytes_rem : 7;

    for (int i = 0; i < bytes_to_copy; ++i)
    {
        buf[i] = data[i];
        tp->bytes_rem--;
    }

    tp->next_seq++;
}

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt)
{
    (void)tp, (void)dt;

    uint8_t* buf = tp->buf + ((tp->next_seq - 1) * 7);
    uint8_t* data = (uint8_t*)&dt->data0;
    int bytes_to_copy = (tp->bytes_rem < 7) ? tp->bytes_rem : 7;

    int i;
    for (i = 0; i < bytes_to_copy; ++i)
    {
        data[i] = buf[i];
        tp->bytes_rem--;
    }

    // We've no more bytes remaining, fill unused data bytes with 0xFF
    for (; i < 7; ++i)
        data[i] = 0xFF;

    dt->seq = tp->next_seq;
    tp->next_seq++;
}
