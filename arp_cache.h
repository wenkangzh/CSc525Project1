#ifndef ARP_CACHE_H
#define ARP_CACHE_H

#include "sr_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// time out 20 seconds
#define ARP_TIMEOUT_TIME 20.0

typedef struct arp_cache
{
	unsigned char ether_addr[ETHER_ADDR_LEN];
	uint32_t ip_addr;
	time_t ts;
	struct arp_cache *next;
}arp_cache;

extern struct arp_cache *arp_cache_list;

//get MAC address
unsigned char * arp_cache_get_ethernet_addr(uint32_t ip);

void arp_cache_add_mapping(unsigned char *ether, uint32_t ip);
void arp_cache_check_timeout();
void arp_cache_print_list();

#endif