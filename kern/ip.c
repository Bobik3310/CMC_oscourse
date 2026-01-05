#include <kern/ip.h>

#include <inc/string.h>
#include <inc/error.h>
#include <inc/stdio.h>

#include <kern/inet.h>
#include <kern/ethernet.h>
// #include <kern/icmp.h>
// #include <kern/udp.h>

uint32_t
ip2num(uint8_t ip[4]) {
    return
        ((uint32_t) ip[0] << 24) |
        ((uint32_t) ip[1] << 16) |
        ((uint32_t) ip[2] << 8 ) |
        ((uint32_t) ip[3] << 0 );
}

static uint16_t packet_id = 0;

static uint16_t
ip_checksum(void *vdata, size_t length) {
    char *data = vdata;
    uint32_t acc = 0xffff;
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }

    // Handle any partial block at the end of the data
    if (length & 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }

    // Return the checksum in network byte order
    return htons(~acc);
}


int
ip_send(struct ip_pkt *pkt, uint16_t length) {
    uint16_t id = ++packet_id;

    struct ip_hdr *hdr = &pkt->hdr;
    hdr->ip_verlen          = IP_VER_LEN;
    hdr->ip_tos             = 0;
    hdr->ip_total_length    = htons(length + IP_HEADER_LEN);
    hdr->ip_id              = htons(id);
    hdr->ip_flags_offset    = 0;
    hdr->ip_ttl             = IP_TTL;
    hdr->ip_header_checksum = ip_checksum((void *) pkt, IP_HEADER_LEN);

    struct eth_hdr e_hdr;
    e_hdr.eth_type = htons(ETH_TYPE_IP);

    // length - data length
    return eth_send(&e_hdr, (void *) pkt, sizeof(*hdr) + length);
}

int
ip_recv(void *buf, size_t buflen)
{
    int res = eth_recv(buf, buflen);
    if (res < 0) {
        return res;
    }
    if ((size_t)res < sizeof(struct ip_hdr)) {
        return -E_INV_IP_LEN;   // too small to even hold base header // MYTODO: Add special error for it
    }

    struct ip_hdr *hdr = (struct ip_hdr *)buf;

    // ip_verlen: high 4 bits = version, low 4 bits = header length in 32-bit words
    uint8_t ver = hdr->ip_verlen >> 4;
    uint8_t ihl_words = hdr->ip_verlen & 0x0F;
    size_t ihl_bytes = (size_t) ihl_words * 4;

    if (ver != 4) {
        return -E_UNS_IP_VER;
    }
    if (ihl_bytes < sizeof(struct ip_hdr)) {
        return -E_INV_IP_HLEN;  // invalid header length // MYTODO: Add special error for it
    }
    if (ihl_bytes > (size_t)res) {
        return -E_INV_IP_HLEN;  // header says longer than received bytes // MYTODO: Add special error for it
    }

    // Total length is in network byte order
    uint16_t total_len = ntohs(hdr->ip_total_length);

    if (total_len < ihl_bytes) {
        return -E_INV_IP_LEN; // MYTODO: Add special error for it
    }
    if (total_len > (uint16_t)res) {
        return -E_INV_IP_LEN;   // packet not fully received / truncated // MYTODO: Add special error for it
    }

    // Checksum is over the IP header only (ihl_bytes)
    uint16_t checksum = hdr->ip_header_checksum;
    hdr->ip_header_checksum = 0;
    if (checksum != ip_checksum((void *)hdr, ihl_bytes)) {
        return -E_INV_IP_CHECKSUM;
    }
    hdr->ip_header_checksum = checksum; // optional: restore

    // Dispatch to higher level protocols
    const enum IPProto current_protocol = hdr->ip_protocol;
    switch (current_protocol) {
        // case IP_PROTO_ICMP: {
        // }
        // case IP_PROTO_TCP: {
        // }
        // case IP_PROTO_UDP: {
        // }
        default: {
            return -E_BAD_IP_PROTO;
        }
    }

    return (int)total_len;
}

