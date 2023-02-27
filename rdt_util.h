//
// Created by chasingdreams on 2/27/23.
//

#ifndef RDT_RDT_UTIL_H
#define RDT_RDT_UTIL_H

#include "rdt_struct.h"

#define MAX_SEQ 10
#define SEND_BUFFER 128
#define HEADER_SIZE 2
#define TAIL_SIZE 2
#define MAX_PAYLOAD (RDT_PKTSIZE - HEADER_SIZE - TAIL_SIZE)
#define BASE_NUMBER 73
#define BIOS_NUMBER 27
#define TIMEOUT 0.3
#define DEBUG(format, ...) do { \
    if (true)                   \
        fprintf(stdout, "%f %s %s(Line %d):", GetSimulationTime(), __FILE__, __FUNCTION__, __LINE__);\
        fprintf(stdout, format, __VA_ARGS__);\
    } \
    while(0)
#define inc(seq) do { \
    if (seq < MAX_SEQ) \
        seq = seq + 1;\
        else seq = 0;\
} while(0)

struct Time_pair {
    double create_time;
    int seq;

    Time_pair() {}

    Time_pair(double x1, int seq_) {
        create_time = x1;
        seq = seq_;
    }
};

void build_checksum(packet *packet);
unsigned short calc_checksum(packet *packet);
bool check_packet(packet *packet);
static bool between(int a, int b, int c);

#endif //RDT_RDT_UTIL_H
