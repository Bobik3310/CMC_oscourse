// ITASK: Your code here

#include <kern/udp.h>
#include <kern/inet.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <kern/traceopt.h>

static const uint16_t SOURCE_PORT = 8081;
static const uint16_t DESTINATION_PORT = 1234;

int
udp_send(void *data, int length) {
    // if (trace_packet_processing) {
    //     cprintf("Sending UDP packet\n");
    // }

    if (length < 0 || length > (int) UDP_DATA_LENGTH) {
        cprintf("Check length in udp_send\n");
        return -1;
    }

    struct udp_pkt pkt;

    struct udp_hdr *hdr = &pkt.hdr;
    hdr->source_port = htons(SOURCE_PORT);
    hdr->destination_port = htons(DESTINATION_PORT);
    hdr->length = htons(length + sizeof(*hdr));
    hdr->checksum = 0; // means "not used"
    // MYTODO: Sanitize length
    memcpy((void *) pkt.data, data, length);

    struct ip_pkt result = {0};
    result.hdr.ip_protocol = IP_PROTO_UDP;
    result.hdr.ip_source_address = htonl(MY_IP);
    result.hdr.ip_destination_address = htonl(HOST_IP);
    memcpy((void *) result.data, (void *) &pkt, length + sizeof(*hdr));

    // Dispatch to the lower level protocol
    return ip_send(&result, length + sizeof(*hdr));
}

int
udp_recv(struct ip_pkt *pkt) {
    // if (trace_packet_processing) {
    //     cprintf("Processing UDP packet\n");
    // }

    struct udp_pkt upkt;
    int size = ntohs(pkt->hdr.ip_total_length) - IP_HEADER_LEN;
    // MYTODO: Sanitize length
    memcpy((void *) &upkt, (void *) pkt->data, size);

    struct udp_hdr *hdr = &upkt.hdr;
    cprintf("port: %d\n", ntohs(hdr->destination_port));
    for (size_t i = 0; i < ntohs(hdr->length) - UDP_HEADER_LEN; ++i) {
        cprintf("%c", upkt.data[i]);
    }
    cprintf("\n");

    // udp_send(upkt.data, ntohs(hdr->length) - UDP_HEADER_LEN); // MYTODO: WHAT?!

    return 0;
}

