#include <kern/arp.h>
#include <kern/ethernet.h>
#include <kern/inet.h>
#include <kern/traceopt.h>

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>

#define IP_FMT "%u.%u.%u.%u"
#define IP_ARG(ip) \
    ((uint8_t *)&(ip))[0], \
    ((uint8_t *)&(ip))[1], \
    ((uint8_t *)&(ip))[2], \
    ((uint8_t *)&(ip))[3]

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(mac) \
    (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]

static struct arp_cache_table arp_table[ARP_TABLE_MAX_SIZE];

uint8_t *
get_mac_by_ip(uint32_t ip) 
{
    struct arp_cache_table *entry;
    for (int i = 0; i < ARP_TABLE_MAX_SIZE; i++) 
    {
        entry = &arp_table[i];
        if (entry->source_ip == ip) 
        {
            return entry->source_mac;
        }
    }

    return NULL;
}

void
initialize_arp_table(void)
{
    for (int i = 0; i < ARP_TABLE_MAX_SIZE; i++) {
        arp_table[i].state = FREE_STATE;
        arp_table[i].source_ip = 0;
        memset(arp_table[i].source_mac, 0, 6);
    }
}

int
update_arp_table(struct arp_hdr *arp_header) 
{
    struct arp_cache_table *entry;
    int i;
    for (i = 0; i < ARP_TABLE_MAX_SIZE; i++)
    {
        entry = &arp_table[i];
        if (entry->state == FREE_STATE)
        {
            entry->source_ip = arp_header->source_ip;
            memcpy(entry->source_mac, arp_header->source_mac, 6);
            entry->state = DYNAMIC_STATE;

            return 0;
        }

        if (entry->source_ip == arp_header->source_ip)
        {
            if (entry->state == DYNAMIC_STATE)
            {
                memcpy(entry->source_mac, arp_header->source_mac, 6);
            }

            break;
        }
    }

    if (i == ARP_TABLE_MAX_SIZE) 
    {
        return -1;
    }

    // cprintf("ARP %s: IP=" IP_FMT "  MAC=" MAC_FMT "\n",
    //     entry->state == DYNAMIC_STATE ? "updated" : "added",
    //     IP_ARG(entry->source_ip),
    //     MAC_ARG(entry->source_mac));

    return 0;
}

int
arp_reply(struct arp_hdr *arp_header) 
{
    cprintf("ARP REPLY\n");
    // if (trace_packet_processing) 
    // {
    //     cprintf("Sending ARP reply\n");
    // }

    arp_header->hardware_type = htons(ARP_ETHERNET);
    arp_header->protocol_type = htons(ARP_IPV4);
    arp_header->opcode = htons(ARP_REPLY);
    arp_header->target_ip = arp_header->source_ip;

    memcpy(arp_header->target_mac, arp_header->source_mac, 6);
    memcpy(arp_header->source_mac, (void *) &qemu_mac[0], 6);
    arp_header->source_ip = htonl(MY_IP);
    // arp_header->source_ip = MY_IP;

    struct eth_hdr reply_header;
    memcpy(reply_header.eth_destination_mac, arp_header->target_mac, 6);
    reply_header.eth_type = htons(ETH_TYPE_ARP);
    memcpy(reply_header.eth_destination_mac, get_mac_by_ip(arp_header->target_ip), 6);
cprintf("\n");
cprintf("\n");
cprintf("ARP REPLY PACKET\n");
cprintf("  hw_type   = 0x%04x\n", ntohs(arp_header->hardware_type));
cprintf("  proto     = 0x%04x\n", ntohs(arp_header->protocol_type));
cprintf("  opcode    = %u\n", ntohs(arp_header->opcode));

cprintf("  sender MAC = " MAC_FMT "\n", MAC_ARG(arp_header->source_mac));
cprintf("  sender IP  = " IP_FMT "\n", IP_ARG(arp_header->source_ip));

cprintf("  target MAC = " MAC_FMT "\n", MAC_ARG(arp_header->target_mac));
cprintf("  target IP  = " IP_FMT "\n", IP_ARG(arp_header->target_ip));
cprintf("\n");
cprintf("\n");
    int status = eth_send(&reply_header, arp_header, sizeof(struct arp_hdr));
    if (status < 0) 
    {
        cprintf("Error attempting arp response.");

        return -1;
    }

    return 0;
}

int
arp_resolve(void* data) 
{
    // if (trace_packet_processing) 
    // {
    //     cprintf("Resolving ARP\n");
    // }

    struct arp_hdr *arp_header;
    arp_header = (struct arp_hdr *)data;



    /* ===== ARP DEBUG PRINT ===== */
    cprintf("\n[ARP] hrd=%u proto=0x%04x op=%u\n"
            "      sender " IP_FMT " (" MAC_FMT ")\n"
            "      target " IP_FMT " (" MAC_FMT ")\n",
            ntohs(arp_header->hardware_type),
            ntohs(arp_header->protocol_type),
            ntohs(arp_header->opcode),
            IP_ARG(arp_header->source_ip),
            MAC_ARG(arp_header->source_mac),
            IP_ARG(arp_header->target_ip),
            MAC_ARG(arp_header->target_mac));
    /* ========================== */





    arp_header->hardware_type = ntohs(arp_header->hardware_type);
    arp_header->protocol_type = ntohs(arp_header->protocol_type);
    arp_header->opcode = ntohs(arp_header->opcode);
    arp_header->target_ip = ntohl(arp_header->target_ip);

    if (arp_header->hardware_type != ARP_ETHERNET)
    {
        cprintf("Error! Only ethernet is supporting.");

        // return -E_UNS_ARP_HRDWR_TYPE;
    }

    if (arp_header->protocol_type != ARP_IPV4) 
    {
        cprintf("Error! Only IPv4 is supported.");

        return -E_UNS_ARP_PROTO;
    }

    int status = update_arp_table(arp_header);
    if (status == 0) 
    {
        cprintf("ARP table is filled in");
    }

    if (arp_header->target_ip != MY_IP)
    {
        cprintf("This is not for me!");

        return -1;
    }

    if (arp_header->opcode != ARP_REQUEST) 
    {
        cprintf("Error! Only arp requests are supported");

        return -E_UNS_ARP_OPCODE;
    }

    return arp_reply(arp_header);
}
