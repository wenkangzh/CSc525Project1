
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "sr_router.h"
#include "sr_packethandler.h"
#include "sr_if.h"
#include "sr_rt.h"
#include "sr_protocol.h"
#include "arp_cache.h"

int handleIp(  struct ip* iphdr,
                struct sr_instance* sr,
                struct sr_ethernet_hdr *ethhdr,
                uint8_t * packet/* lent */,
                unsigned int len,
                char* interface){


    testip(iphdr);
    //verify version = 4.
    if(iphdr->ip_v != 4)
        //drop;
        return -1;

    //check if the destination is router itself. and TCP/UDP, drop.
    uint32_t ipdst = (iphdr->ip_dst).s_addr;
    // tcp = 6 udp =15
    if((ipdst==sr->if_list->ip)&&(iphdr->ip_p==6||iphdr->ip_p==15)){
        //drop packet
        return -1;
    }


    //decrement then check TTL
    iphdr->ip_ttl -= 1;
    if(iphdr->ip_ttl==0)
        //drop packet
        return -1;
    else{
        updatechecksum();
    }


    //look up routing table/ find ip of next hop

    if(existin_routingtable){
        sr_send_packet(sr,packet,len,nexthopinterface);
    }

    else 
    {
        sendArpRequest(sr,iphdr->ip_dst->s_addr);

        //save to ip buffer

        for(int i=0;i<10;i++){
            //find ipbuffer unit for this ip address
            if(ipbuffer[i]->sender_ip==0){
                ipbuffer[i]->sender_ip==iphdr->ip_dst->s_addr;
                counter++;
                ipbuffer[i]->packet[counter] = packet;
                ipbuffer[i]->packetLen[counter] = packetLen;
                break;
            }

        }


    }

}


// --------------------------------------------------------------------------------

void handleArp( struct sr_arphdr *arphdr,
                struct sr_instance* sr,
                struct sr_ethernet_hdr *ethhdr,
                uint8_t * packet/* lent */,
                unsigned int len,
                char* interface/* lent */){


                testarp();

	    // if arp request, we simply open it, change it and send back.
           
        
        // handleArp(arphdr,sr, ethhdr,packet,len,interface);

    // if arp request, we simply open it, change it and send back.
    if(ntohs(arphdr->ar_op)==ARP_REQUEST && arphdr->ar_tip==sr->if_list->ip){

        printf(" ARP is request \n");

        arphdr->ar_op = htons(ARP_REPLY); // change to arp reply
        // change sender/target info.  IP and MAC
        uint32_t tmp = arphdr->ar_tip;
        arphdr->ar_tip = arphdr->ar_sip;
        arphdr->ar_sip = tmp;
        
        //modify the arpMAC
        memcpy(arphdr->ar_tha,arphdr->ar_sha,6);
        memcpy(arphdr->ar_sha,sr->if_list->addr,6);


        //modify ethernet header!
        memcpy(ethhdr->ether_dhost,ethhdr->ether_shost,6);
        memcpy(ethhdr->ether_shost,sr->if_list->addr,6);

 
        sr_send_packet(sr,packet,len,interface);
        printf(" ARP reply packet sent out \n");
    }

    else if(ntohs(arphdr->ar_op)==ARP_REPLY){
        printf("ARP is reply \n");

        //if not in the cache:
        unsigned char *MAC = arp_cache_get_ethernet_addr(arphdr->ar_sip);

        if(MAC ==NULL)
            arp_cache_add_mapping(arphdr->ar_sha,arphdr->ar_sip);


        // search ip buffer for arphdr->ar_sip
        for(int i=0;i<10;i++){
            //find ipbuffer unit for this ip address
            if(ipbuffer[i]->sender_ip==arphdr->ar_sip){

                for(int j=0;j<ipbuffer[i]->counter;j++){

                    int packetLen = ipbuffer[i]->packetLen[j];
                    sr_send_packet(sr,ipbuffer[i]->packet[j],packetLen,ipdestinferface);

                    ipbuffer[i]->packetLen[j] =0;
                    ipbuffer[i]->packet[j] =NULL;
                }
                ipbuffer[i]->counter =0;
                ipbuffer[i]->sender_ip=0;
            }
        }


    }

 
}
// --------------------------------------------------------------------------------

void sendArpRequest(
                    struct sr_instance* sr,
                    uint32_t ip //pass in final ip in ip header 
                        ){

     // set len/ malloc packet             
    unsigned int len = sizeof(sr_ethernet_hdr) + sizeof(sr_arphdr);
    uint8_t * packet = (uint8_t*) malloc (len);   
    // locate ehdr/ arphdr
    struct sr_ethernet_hdr *ethhdr = (struct sr_ethernet_hdr *) packet;
    struct sr_arphdr *arphdr = (struct sr_arphdr*) (packet+sizeof(struct sr_ethernet_hdr));


    //search routing table for nexthop (gonna be ether target ip)/ interface.
    char* interface= sr_getInterfaceForARP(sr,ip);
    // if found ip in routing table

    // set type
    ethhdr -> ether_type = htons(ETHERTYPE_ARP); 

    //broadcast/ set  MAC
    memset(ethhdr->ether_dhost, 0xff, ETHER_ADDR_LEN);
    memcpy(ethhdr->ether_shost,sr->if_list->addr,6);

    memcpy(ethhdr->ar_sha,sr->if_list->addr,6);          
    memcpy(ethhdr->ar_tha,,6);

    sr_arphdr -> ar_hrd = ETHER_ADDR_LEN ;          
    sr_arphdr -> ar_pro =htons(ethertype_ip);            
    sr_arphdr -> ar_hln = ETHER_ADDR_LEN ;            
    sr_arphdr -> ar_pln = 4;            
    sr_arphdr -> ar_op = htons(ar_op);            

    sr_arphdr -> ar_sip = sr->if_list->ip;  
    // how to find next hop ip?? 
    sr_arphdr -> ar_tip = ?????????;  


    sr_send_packet(sr,packet,len,interface);

    free(packet);

}
//--------------------------------------------------------------------------------
unsigned short checksum(unsigned short *buf, int count) { 
//     register u long sum = 0;

//     while (count--) { 

//         sum += *buf++;
//         if (sum & 0xFFFF0000) { 
//              carry occurred, so wrap around 
//             sum &= 0xFFFF;
//             sum++; 
//         }

// }
//     return ~(sum & 0xFFFF);
}


// --------------------------------------------------------------------------------
char * sr_getInterfaceForARP(struct sr_instance *sr, uint32_t ip, ) {
    struct sr_rt *rt = sr->routing_table;
    while (rt) {

        if ((ip & rt->mask.s_addr) == rt->dest.s_addr) 
            return rt->interface;
        
        rt = rt->next;
    }

    return NULL;
}

// --------------------------------------------------------------------------------
void testip(struct ip* iphdr){
    printf("total length:  %hhu\n", ntohs(iphdr->ip_len));
    printf("header length in 4bytes word: %u\n", (iphdr->ip_hl));
    printf("type of service:  %hhu\n", iphdr->ip_tos);
    printf("sr_protocol:  %hhu\n", iphdr->ip_p);
    printf("TTL:  %hhu\n", iphdr->ip_ttl);

    
    uint32_t ipsrc = (iphdr->ip_src).s_addr;
    uint32_t ipdst = (iphdr->ip_dst).s_addr;

    printf("%u\n",ipdst);
    // printf("-------------- Sender-IP --------------\n");

    // unsigned char bytes[4];
    // bytes[0] = ipsrc & 0xFF;
    // bytes[1] = (ipsrc >> 8) & 0xFF;
    // bytes[2] = (ipsrc >> 16) & 0xFF;
    // bytes[3] = (ipsrc >> 24) & 0xFF;   
    // printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    // printf("-------------- Sender-IP --------------\n");


    // printf("-------------- Target-IP --------------\n");

    // bytes[0] = ipdst & 0xFF;
    // bytes[1] = (ipdst >> 8) & 0xFF;
    // bytes[2] = (ipdst >> 16) & 0xFF;
    // bytes[3] = (ipdst >> 24) & 0xFF;   
    // printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    // printf("-------------- Target-IP --------------\n");
}

// --------------------------------------------------------------------------------
void testarp(struct sr_arphdr *p,){

    printf("%hu\n", ntohs(p->ar_hrd));
    printf("%hu\n", ntohs(p->ar_pro));

    printf("%hhu\n", ntohs(p->ar_hln));
    printf("%hhu\n", ntohs(p->ar_pln));
    printf("%hu\n", ntohs(p->ar_op));
    printf("%hu\n", ntohs(p->ar_sip));
    printf("%hu\n", ntohs(p->ar_tip));



    // printf("-------------- Ethernet-dest-address --------------\n");
    // printf("%hhu:%hhu:%hhu:%hhu:%hhu:%hhu\n", ethernet_p->ether_dhost[0],
    //                                     fwer e    ethernet_p->ether_dhost[1],
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
}