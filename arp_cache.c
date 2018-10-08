#include "sr_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arp_cache.h"

struct arp_cache *arp_cache_list = NULL;

/*
 *----------------------------------------------------------------------
 *
 * arp_cache_add_mapping --
 *
 *	This function adds the given Ethernet address and IP address mapping to the 
 *  currently maintained ARP cache list. 
 *
 * Parameters:
 *		ether[]: The Ethernet address to be mapped with the given IP address
 *      ip: The IP address to be mapped with given Ethernet address
 *	
 * Return: None.
 *
 * Side effects:
 *      Add a node in arp_cache_list. 
 *
 *----------------------------------------------------------------------
 */
void arp_cache_add_mapping(unsigned char ether[], uint32_t ip)
{
	struct arp_cache *temp = NULL;
	printf("Adding mapping.\n");
	temp = (struct arp_cache *) malloc(sizeof(struct arp_cache));
	memcpy(temp->ether_addr, ether, sizeof(unsigned char) * ETHER_ADDR_LEN);
	temp->ip_addr = ip;
	temp->ts = time(NULL);
	temp->next = arp_cache_list;
	arp_cache_list = temp;
}

/*
 *----------------------------------------------------------------------
 *
 * arp_cache_get_ethernet_addr --
 *
 *	This function finds the Ethernet address with a given IP address
 *  and the same time update the time stamp field to the current time. 
 *
 * Parameters:
 *      ip: The provided IP address in order to search for mapping.
 *	
 * Return: The Ethernet address mapped with the provided IP address.
 *
 * Side effects:
 *      The timestamp of this mapping is updated to current time.
 *
 *----------------------------------------------------------------------
 */
unsigned char * arp_cache_get_ethernet_addr(uint32_t ip)
{
	struct arp_cache *temp = arp_cache_list;
	while(temp){
		if(temp->ip_addr == ip){
			// found THE one!
			temp->ts = time(NULL);
			return temp->ether_addr;
		}
		temp = temp->next;
	}
	return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * arp_cache_check_timeout --
 *
 *	This function iterates the whole ARP cache and finds if any mappings are expired. 
 *
 * Parameters:
 *      None.
 *	
 * Return: None
 *
 * Side effects:
 *      If there are any expired mappings in the cache, remove them. 
 *
 *----------------------------------------------------------------------
 */
void arp_cache_check_timeout()
{
	struct arp_cache *temp = arp_cache_list;
	struct arp_cache *prev;

	// this loop is for every occurrence at the head. 
	while(temp != NULL && difftime(time(NULL), temp->ts) > ARP_TIMEOUT_TIME){
		arp_cache_list = temp->next;
		free(temp);
		temp = arp_cache_list;
	}

	// this loop is for every occurrence other than head
	while(temp != NULL){
		while(temp != NULL && difftime(time(NULL), temp->ts) <= ARP_TIMEOUT_TIME){
			prev = temp;
			temp = temp->next;
		}
		if(temp == NULL)
			break;
		prev->next = temp->next;
		free(temp);
		temp = prev->next;
	}
}

void arp_cache_print_list()
{
	struct arp_cache *traverse = arp_cache_list;
	printf("------------------START------------------\n");
	while(traverse != NULL){
		printf("---------------------------------------\n");
		printf("Ethernet Address:\t");
	   	printf("%hhu:%hhu:%hhu:%hhu:%hhu:%hhu\n", 	traverse->ether_addr[0],
					                                    traverse->ether_addr[1],
					                                    traverse->ether_addr[2],
					                                    traverse->ether_addr[3],
					                                    traverse->ether_addr[4],
					                                    traverse->ether_addr[5]);

	   	printf("IP Address:\t");
	   	unsigned char bytes[4];
	   	bytes[0] = traverse->ip_addr & 0xFF;
	    bytes[1] = (traverse->ip_addr >> 8) & 0xFF;
	    bytes[2] = (traverse->ip_addr >> 16) & 0xFF;
	    bytes[3] = (traverse->ip_addr >> 24) & 0xFF;   
	    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);

	    printf("Timestamp:\t");
	    char *t = ctime(&(traverse->ts));
	    printf("%s", t);

		traverse = traverse->next;
		printf("---------------------------------------\n");
	}
	printf("------------------END------------------\n");
}