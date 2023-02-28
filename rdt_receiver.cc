/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
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
#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_util.h"
#include <map>

std::map<int, packet> buffered_packets;
int cur_seq_expected = 0;
int tot_to = 0;
/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    cur_seq_expected = 0;
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}


/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    if (!check_packet(pkt)) { // wrong packet, ignore it
        DEBUG("[Receiver]Corrupted packet!\n", 2);
        return ;
    } else {
        if (pkt->data[1] == cur_seq_expected) { // right order
            message *msg = new message;
            msg->size = pkt->data[0];
            msg->data = (char*) malloc (msg->size);
            memcpy(msg->data, pkt->data + HEADER_SIZE, msg->size);
            Receiver_ToUpperLayer(msg);
            inc(cur_seq_expected);
            //cur_seq_expected = (cur_seq_expected + 1) % (MAX_SEQ + 1);
            DEBUG("[LOWER] got seq = %d, tot_to = %d\n", pkt->data[1], ++tot_to);
            DEBUG("[RR]Receiver received seq = %d, send ack to sender\n", pkt->data[1]);
            Receiver_ToLowerLayer(pkt); // ack
            if (msg->data != NULL) free(msg->data);
            if (msg != NULL)free(msg);
        } else {
            // current turn's packet
            if (this_turn(pkt->data[1], cur_seq_expected)) {
                DEBUG("[RR-O]Receiver receive seq = %d, but expect %d, store it and ack\n", pkt->data[1], cur_seq_expected);
                if (buffered_packets.count(pkt->data[1])) {
                    DEBUG("****Fatal: buffer overflow seq=%d\n", pkt->data[1]);
                }
                buffered_packets[pkt->data[1]] = *pkt;
            }
            else {
                DEBUG("[RR-L]Receiver receive seq = %d, but expect %d, and this may be last turn, only send ack back\n",
                      pkt->data[1], cur_seq_expected);
            }
            Receiver_ToLowerLayer(pkt); // send ack this packet
        }
        while (!buffered_packets.empty() && buffered_packets.count(cur_seq_expected)) { // have this
            packet pkt = buffered_packets[cur_seq_expected];
            message *msg = new message;
            msg->size = pkt.data[0];
            msg->data = (char*) malloc (msg->size);
            memcpy(msg->data, pkt.data + HEADER_SIZE, msg->size);
            Receiver_ToUpperLayer(msg);
            DEBUG("[LOWER] got seq = %d, total to = %d\n", cur_seq_expected, ++tot_to);
            DEBUG("[RR-B]Receiver from buffer, get seq=%d\n", cur_seq_expected);
            ASSERT(buffered_packets.erase(cur_seq_expected) > 0);
            inc(cur_seq_expected);
            //Receiver_ToLowerLayer(&pkt); // ack
        }
    }
}
