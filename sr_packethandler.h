#ifndef SR_PACKETHANDLER_H
#define SR_PACKETHANDLER_H


#include <stdio.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_router.h"


void handleArp( struct sr_arphdr *arphdr,
				struct sr_instance* sr,
			    struct sr_ethernet_hdr *ethhdr,
			    uint8_t * packet/* lent */,
        		unsigned int len,
        		char* interface/* lent */);

void handleIp(struct ip* iphdr);




#endif