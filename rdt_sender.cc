/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <list>
#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_util.h"

int next_frame_to_send, ack_expected, buffered_num;
std::queue<packet> packets;
std::list<Time_pair> logical_clock;
packet buffers[MAX_SEQ + 1];
bool push_to_buffer(packet &packet) {
    packets.push(packet);
    return true;
}

bool pop_from_buffer(packet *pPacket) {
    if (packets.empty()) return false;
    *pPacket = packets.front();
    packets.pop();
    return true;
}


void Wrapped_StartTimer(int seq) {
    if (Sender_isTimerSet())
        logical_clock.push_back(Time_pair(GetSimulationTime(), seq));
    else {
        logical_clock.push_back(Time_pair(GetSimulationTime(), seq));
        Sender_StartTimer(TIMEOUT);
    }
}

void Wrapped_StopTimer(int seq) {
    bool flag = false;
    for (auto it = logical_clock.begin(); it != logical_clock.end(); ++it) {
        if ((*it).seq == seq) {
            if (it == logical_clock.begin()) { // special case, delete the first timer
                flag = true;
            }
            logical_clock.erase(it);
            break;
        }
    }
    if (flag) {
        if (logical_clock.empty()) { // no timer current
            Sender_StopTimer();
            return ;
        }
        double lastTime = (*logical_clock.begin()).create_time + TIMEOUT - GetSimulationTime();
        ASSERT(lastTime > 0);
        Sender_StartTimer(lastTime);
    }
}

// try to send a packet, if buffered_num >= 10, directly return
// this will be called everywhere
void try_sendPacket(bool flag) {
    if (buffered_num >= MAX_SEQ) return; // inque number, waiting for ack ...
    packet pkt;
    if (!pop_from_buffer(&pkt)) return;
    ASSERT(pkt.data[1] <= MAX_SEQ);
    buffers[(int)pkt.data[1]] = pkt;
    buffered_num++;
    Wrapped_StartTimer(pkt.data[1]);
    Sender_ToLowerLayer(&pkt);
    if (!flag)
        DEBUG("Sent a packet, seq = %d\n", pkt.data[1]);
    else
        DEBUG("Resent a packet, seq = %d\n", pkt.data[1]);
}

/* sender initialization, called once at the very beginning */
void Sender_Init() {
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    next_frame_to_send = 0;
    ack_expected = 0;
    buffered_num = 0;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final() {
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg) {
    int header_size = HEADER_SIZE;
    int maxpayload_size = MAX_PAYLOAD;
    int cursor = 0;
    packet pkt;
    while (msg->size - cursor > maxpayload_size) {
        pkt.data[0] = (char)(maxpayload_size & 0XFF);
        pkt.data[1] = (char)(next_frame_to_send & 0XFF);
        inc(next_frame_to_send);
        memcpy(pkt.data + HEADER_SIZE, msg->data + cursor, maxpayload_size);
        build_checksum(&pkt);
        if (!push_to_buffer(pkt)) {
            DEBUG("Fatal: buffer used up, seq = %d\n", next_frame_to_send);
        }
        cursor += maxpayload_size;
        try_sendPacket(false);
    }
    if (msg->size > cursor) {
        pkt.data[0] = (char)((msg->size - cursor) & 0XFF);
        pkt.data[1] = (char)(next_frame_to_send & 0XFF);
        inc(next_frame_to_send);
        memcpy(pkt.data + HEADER_SIZE, msg->data + cursor, msg->size - cursor);
        build_checksum(&pkt);
        if (!push_to_buffer(pkt)) {
            DEBUG("Fatal: buffer used up, seq = %d\n", next_frame_to_send);
        }
        try_sendPacket(false);
    }
    try_sendPacket(false);
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender
   This is always ack in my implementation
   */
void Sender_FromLowerLayer(struct packet *pkt) {
    if (!check_packet(pkt)) {
        DEBUG("Corrupted packet!\n", 1);
        return ;
    }
    if (ack_expected == pkt->data[1]) { // success to receive ack
        Wrapped_StopTimer(ack_expected);
        buffered_num--;
        inc(ack_expected);
        DEBUG("Sender received ack, seq = %d, next expected seq = %d\n", pkt->data[1], ack_expected);
        try_sendPacket(false);
    }
}

/* event handler, called when the timer expires */
/* simply resend all datas in buffer*/
void Sender_Timeout() {
    DEBUG("Timeout, seq = %d\n", ack_expected);
    next_frame_to_send = ack_expected;
    int tmp = buffered_num;
    for (int i = 1; i <= tmp; ++i) {
        try_sendPacket(true);
    }
}
