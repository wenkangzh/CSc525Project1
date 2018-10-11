/**********************************************************************
 * file:  sr_router.c 
 * date:  Mon Feb 18 12:50:42 PST 2002  
 * Contact: casado@stanford.edu 
 *
 * Description:
 * 
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing. 11
 * 90904102
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_packethandler.h"
#include "arp_cache.h"

/*--------------------------------------------------------------------- 
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 * 
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr) 
{
    /* REQUIRES */
    assert(sr);


    /* Add initialization code here! */

 /* -- sr_init -- */
    
    


    for(int i=0;i<10;i++){
        ipbuffer[i] = (struct ipunit*) malloc(sizeof(struct ipunit));

        ipbuffer[i]-> counter =0;
        ipbuffer[i]->sender_ip =0;
        for(int j=0;j<300;j++){
            ipbuffer[i]-> packet[j] =NULL;
            ipbuffer[i]->packetLen[j] =0;   
        }
    }    

}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr, 
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */){
    /* REQUIRES */
    assert(sr);
    assert(packet);
    assert(interface);

    printf("*** -> Received packet of length %d \n",len);

    struct sr_ethernet_hdr *ethhdr = (struct sr_ethernet_hdr *) packet;
    uint16_t ether_type = ethhdr -> ether_type;

    // printf("The ether_type is  %x \n",ntohs(ether_type));
    printf("Interface: %s\n", interface);
    // Arp packet
    if(ntohs(ether_type) == ETHERTYPE_ARP){
        struct sr_arphdr *arphdr = (struct sr_arphdr*) (packet+sizeof(struct sr_ethernet_hdr));

        printf(" ARP packet Received \n");
        handleArp(arphdr,sr,ethhdr,packet,len,interface);
    }
    // ip packet
    else if (ntohs(ether_type) == ETHERTYPE_IP) {
        struct ip* iphdr = (struct ip*) (packet+sizeof(struct sr_ethernet_hdr));

        printf("Received an IP packet \n");
        handleIp(iphdr,sr,ethhdr,packet,len,interface);
    }
    else // unrecongized.
    {
        printf("I can not recongize this packet \n");
    }
}


/* end sr_ForwardPacket */


/*--------------------------------------------------------------------- 
 * Method:
 *
 *---------------------------------------------------------------------*/
