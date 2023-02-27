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

int cur_seq_expected = 0;
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
        DEBUG("Corrupted packet!\n", 2);
        return ;
    } else {
        if (pkt->data[1] == cur_seq_expected) { // right order
            message *msg = new message;
            msg->size = pkt->data[0];
            msg->data = (char*) malloc (msg->size);
            memcpy(msg->data, pkt->data + HEADER_SIZE, msg->size);
            Receiver_ToUpperLayer(msg);
            //inc(cur_seq_expected);
            cur_seq_expected = (cur_seq_expected + 1) % (MAX_SEQ + 1);
            DEBUG("Receiver received seq = %d, send ack to sender, next expected = %d\n", pkt->data[1], cur_seq_expected);
            Receiver_ToLowerLayer(pkt); // ack
            if (msg->data != NULL) free(msg->data);
            if (msg != NULL)free(msg);
        } else {
            DEBUG("Receiver receive seq = %d, but expect %d\n", pkt->data[1], cur_seq_expected);
        }
    }
}
