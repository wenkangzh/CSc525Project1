
#include <stdio.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_router.h"
#include <string.h>

void handleIp(struct ip* iphdr){
}




void handleArp( struct sr_arphdr *arphdr,
                struct sr_instance* sr,
                struct sr_ethernet_hdr *ethhdr,
                uint8_t * packet/* lent */,
                unsigned int len,
                char* interface/* lent */){

	    // if arp request, we simply open it, change it and send back.
           
        
        // handleArp(arphdr,sr, ethhdr,packet,len,interface);
        printf(" ARP packet Received \n");
        printf("%d\n",arphdr->ar_op);

            // if arp request, we simply open it, change it and send back.
    if(arphdr->ar_op==ARP_REQUEST){

        printf(" ARP is request \n");

        arphdr->ar_op = ARP_REPLY; // change to arp reply

        // change sender/target info.  IP and MAC
        uint32_t tmp = arphdr->ar_tip;
        arphdr->ar_tip = arphdr->ar_sip;
        arphdr->ar_sip = tmp;
        
        memcpy(arphdr->ar_tha,arphdr->ar_sha,6);
        memcpy(arphdr->ar_sha,sr->if_list->addr,6);

        //modify ethernet header!
        memcpy(ethhdr->ether_dhost,ethhdr->ether_shost,6);
        memcpy(ethhdr->ether_shost,sr->if_list->addr,6);



    // printf("-------------- Ethernet-dest-address --------------\n");
    // printf("%hhu:%hhu:%hhu:%hhu:%hhu:%hhu\n", ethernet_p->ether_dhost[0],
    //                                         ethernet_p->ether_dhost[1],
    //                                         ethernet_p->ether_dhost[2],
    //                                         ethernet_p->ether_dhost[3],
    //                                         ethernet_p->ether_dhost[4],
    //                                         ethernet_p->ether_dhost[5]);
    // printf("-------------- Ethernet-dest-address --------------\n");

    // printf("-------------- Ethernet-source-address --------------\n");
    // printf("%hhu:%hhu:%hhu:%hhu:%hhu:%hhu\n", ethernet_p->ether_shost[0],
    //                                         ethernet_p->ether_shost[1],
    //                                         ethernet_p->ether_shost[2],
    //                                         ethernet_p->ether_shost[3],
    //                                         ethernet_p->ether_shost[4],
    //                                         ethernet_p->ether_shost[5]);
    // printf("-------------- Ethernet-source-address --------------\n");

    //  struct sr_arphdr *p = (struct sr_arphdr *) (sizeof(struct sr_ethernet_hdr) + packet);
    // printf("-------------- ARP-OP --------------\n");
    // printf("%hu\n", ntohs(p->ar_op));
    // printf("-------------- ARP-OP --------------\n");

    // printf("-------------- Sender-MAC --------------\n");
    // int i;
    // for (i=0; i < ETHER_ADDR_LEN; ++i)
    // {
    //     printf("%x", (p->ar_sha)[i]);
    // }
    // printf("\n");
    // printf("-------------- Sender-MAC --------------\n");

    // printf("-------------- Sender-IP --------------\n");

    // unsigned char bytes[4];
    // bytes[0] = p->ar_sip & 0xFF;
    // bytes[1] = (p->ar_sip >> 8) & 0xFF;
    // bytes[2] = (p->ar_sip >> 16) & 0xFF;
    // bytes[3] = (p->ar_sip >> 24) & 0xFF;   
    // printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    // printf("-------------- Sender-IP --------------\n");

    // printf("-------------- Target-MAC --------------\n");
    // for (i=0; i < ETHER_ADDR_LEN; ++i)
    // {
    //     printf("%x", (p->ar_tha)[i]);
    // }
    // printf("\n");
    // printf("-------------- Target-MAC --------------\n");

    // printf("-------------- Target-IP --------------\n");

    // bytes[0] = p->ar_tip & 0xFF;
    // bytes[1] = (p->ar_tip >> 8) & 0xFF;
    // bytes[2] = (p->ar_tip >> 16) & 0xFF;
    // bytes[3] = (p->ar_tip >> 24) & 0xFF;   
    // printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    // printf("-------------- Target-IP --------------\n");

     sr_send_packet(sr,packet,len,interface);

}


}