#include "rdt_util.h"
#include "rdt_struct.h"
#include <iostream>
#include <cstring>

unsigned short calc_checksum(packet *packet) {
    int res = 0;
    int size = packet->data[0] + HEADER_SIZE;
    for (int i = 0; i < size; ++i) {
        res = res * BASE_NUMBER + packet->data[i] + BIOS_NUMBER;
    }
    return res & 0XFFFF;
}

bool check_packet(packet *packet) {
    ASSERT(packet);
    unsigned short actual_checksum = calc_checksum(packet);
    unsigned short origin_checksum = (((unsigned char)packet->data[RDT_PKTSIZE - TAIL_SIZE]) << 8)
            + (unsigned char) packet->data[RDT_PKTSIZE - TAIL_SIZE + 1];
    //printf("origin: %hd, actual:%hd\n", origin_checksum, actual_checksum);
    return (origin_checksum == actual_checksum);
}

// a <= b < c
static bool between(int a, int b, int c) {
    if ((a <= b && b < c) || (c < a && a <= b) || ((b < c) && (c < a)))
        return true;
    return false;
}

void build_checksum(packet *packet) {
    unsigned short checksum = calc_checksum(packet);
    packet->data[RDT_PKTSIZE - TAIL_SIZE] = (checksum >> 8) & 0XFF;
    packet->data[RDT_PKTSIZE - TAIL_SIZE + 1] = checksum & 0XFF;
}

