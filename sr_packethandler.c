
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "sr_router.h"
#include "sr_packethandler.h"
#include "sr_if.h"
#include "sr_rt.h"
#include "sr_protocol.h"
#include "arp_cache.h"

void send_icmp_echo_reply(  struct sr_instance* sr, 
                            uint8_t * packet, 
                            unsigned int len, 
                            struct ip* iphdr, 
                            char* interface)
{
    struct sr_icmp_hdr *icmp_p = (struct sr_icmp_hdr *) (packet + sizeof(struct ip) + sizeof(struct sr_ethernet_hdr));
    printf("Here is ICMP Header!!!\n");
    printf("Type: %u\n", icmp_p->type);
    printf("Code: %u\n", icmp_p->code);
    printf("Checksum: %x\n", icmp_p->checksum);
    printf("LEN: %u\n", ntohs(iphdr->ip_len) - 20);
    icmp_p->type = 0;
    struct sr_ethernet_hdr* ether_p = (struct sr_ethernet_hdr*)packet;
    uint8_t temp[ETHER_ADDR_LEN];
    memcpy(temp, ether_p->ether_dhost, 6);
    memcpy(ether_p->ether_dhost, ether_p->ether_shost, 6);
    memcpy(ether_p->ether_shost, temp, 6);

    uint32_t temp_ip = iphdr->ip_src.s_addr;
    iphdr->ip_src.s_addr = iphdr->ip_dst.s_addr;
    iphdr->ip_dst.s_addr = temp_ip;
    iphdr->ip_sum = 0;
    u_short new_checksum = checksum((u_short *)iphdr, iphdr->ip_hl * 2);
    iphdr->ip_sum = htons(new_checksum);
    printf("Sending back echo reply through %s\n", interface);
    sr_send_packet(sr,packet,len,interface);

}

void handleIp(  struct ip* iphdr,
                struct sr_instance* sr,
                struct sr_ethernet_hdr *ethhdr,
                uint8_t * packet/* lent */,
                unsigned int len,
                char* interface){


    //verify version = 4.
    if(iphdr->ip_v != 4)
        //drop;
        return;

    //check if the destination is router itself. and TCP/UDP, drop.
    uint32_t ipdst = (iphdr->ip_dst).s_addr;

    int check_dst_ip_in_my_if = 0;
    struct sr_if * use_for_check = sr->if_list;
    while(use_for_check)
    {
        if(use_for_check->ip == iphdr->ip_dst.s_addr){
            check_dst_ip_in_my_if = 1;
            break;
        }
        use_for_check = use_for_check->next;
    }

    if(iphdr->ip_p == IPPROTO_ICMP && check_dst_ip_in_my_if){
        send_icmp_echo_reply(sr, packet, len, iphdr, interface);
        return;
    }

    // tcp = 6 udp =15
    if((ipdst==sr->if_list->ip)&&(iphdr->ip_p==6||iphdr->ip_p==15)){
        //drop packet
        return;
    }

    //decrement then check TTL
    iphdr->ip_ttl -= 1;
    if(iphdr->ip_ttl==0)
        //drop packet
        return;

    else{
        u_short chksum;


        iphdr->ip_sum =0;
        chksum = checksum((u_short *)iphdr, iphdr->ip_hl*2);
        iphdr->ip_sum = htons(chksum);
    }
    //look up routing table/ find ip of next hop
    uint32_t nexhop = sr_getInterfaceAndNexthop(sr, iphdr->ip_dst.s_addr, interface);
    // interface here already got updated

    // see if nexthop's mac address is in arpcache
    unsigned char* ether_dhost = arp_cache_get_ethernet_addr(nexhop);


    // the source address need to be corrosponding to the interface.
    unsigned char *out_addr_in_if = malloc(sizeof(unsigned char) * 6);
    findAddrsForInterface(sr,interface, out_addr_in_if);
    memcpy(ethhdr->ether_shost,out_addr_in_if,6);


    // if we found Mac in arp cache
    if(ether_dhost!=NULL){
        // get the ethernet address from cache to ether header
        memcpy(ethhdr->ether_dhost,ether_dhost,6);

        printf("Sending IP packet with interface %s\n", interface);

        sr_send_packet(sr,packet,len,interface);
    }
    
    // send arp request
    else 
    {
        sendArpRequest(sr,nexhop,interface);

        //save ip packet pointer to ip buffer

        for(int i=0;i<10;i++){
            //find ipbuffer unit for this ip address
            if(ipbuffer[i]->sender_ip==0){
                ipbuffer[i]->sender_ip=iphdr->ip_dst.s_addr;
                ipbuffer[i]->counter ++;
                ipbuffer[i]->packet[ipbuffer[i]->counter] = packet;
                ipbuffer[i]->packetLen[ipbuffer[i]->counter] = len;
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



	    // if arp request, we simply open it, change it and send back.
           
        
        // handleArp(arphdr,sr, ethhdr,packet,len,interface);

    // if arp request, we simply open it, change it and send back.
    if(ntohs(arphdr->ar_op)==ARP_REQUEST && arphdr->ar_tip==sr->if_list->ip){

        printf("Router received an ARP request \n");
        // testarp(packet);

        arphdr->ar_op = htons(ARP_REPLY); // change to arp reply
        // change sender/target info.  IP and MAC
        uint32_t tmp = arphdr->ar_tip;
        arphdr->ar_tip = arphdr->ar_sip;
        arphdr->ar_sip = tmp;
        
        unsigned char *out_addr_in_if = malloc(sizeof(unsigned char) * 6);
        findAddrsForInterface(sr,interface, out_addr_in_if);
        // now out_addr_in_if contains correct hardware address

        //modify the arpMAC
        memcpy(arphdr->ar_tha,arphdr->ar_sha,6);
        memcpy(arphdr->ar_sha,out_addr_in_if,6);


        //modify ethernet header!
        memcpy(ethhdr->ether_dhost,ethhdr->ether_shost,6);
        memcpy(ethhdr->ether_shost,out_addr_in_if,6);

 
        sr_send_packet(sr,packet,len,interface);
        printf(" ARP reply packet sent out \n");
    }

    else if(ntohs(arphdr->ar_op)==ARP_REPLY){
        printf("Router received an ARP reply \n");

        // testarp(packet);

        //if not in the cache:
        unsigned char *MAC = arp_cache_get_ethernet_addr(arphdr->ar_sip);

        if(MAC ==NULL)
            arp_cache_add_mapping(arphdr->ar_sha,arphdr->ar_sip);


        // search ip buffer for arphdr->ar_sip
        for(int i=0;i<10;i++){
            //find ipbuffer unit for this ip address
            if(ipbuffer[i]->sender_ip==arphdr->ar_sip){

                for(int j=0;j<ipbuffer[i]->counter;j++){
                    if(! ipbuffer[i]->packet[j]){
                        continue;
                    }
                    int packetLen = ipbuffer[i]->packetLen[j];
                    struct sr_ethernet_hdr *ether_p = (struct sr_ethernet_hdr *)ipbuffer[i]->packet[j];
                    memcpy(ether_p->ether_dhost, arphdr->ar_sha, 6);
                    sr_send_packet(sr,ipbuffer[i]->packet[j],packetLen,interface);

                    ipbuffer[i]->packetLen[j] =0;
                    ipbuffer[i]->packet[j] =NULL;
                }
                ipbuffer[i]->counter =0;
                ipbuffer[i]->sender_ip=0;
            }
        }
//

    }

 
}

// returns the ip addr and set addr to be ether addr
uint32_t findAddrsForInterface(struct sr_instance *sr, char *interface, unsigned char *addr)
{
    struct sr_if *if_p = sr->if_list;
    while(if_p)
    {
        if(strcmp(interface, if_p->name)==0){
            memcpy(addr, if_p->addr, sizeof(unsigned char) * 6);
            return if_p->ip;
        }
        if_p = if_p->next;
    }
    printf("NO SUCH INTERFACE\n");
    return 0;
}
// --------------------------------------------------------------------------------

void sendArpRequest(
                    struct sr_instance* sr,
                    uint32_t nexthopIP,//pass in final ip in ip header 
                    char* interface // going out inferface
                        ){

     // set len/ malloc packet             
    unsigned int len = sizeof(struct sr_ethernet_hdr) + sizeof(struct sr_arphdr);
    uint8_t * packet = (uint8_t*) malloc (len);   
    // locate ehdr/ arphdr
    struct sr_ethernet_hdr *ethhdr = (struct sr_ethernet_hdr *) packet;
    struct sr_arphdr *arphdr = (struct sr_arphdr*) (packet+sizeof(struct sr_ethernet_hdr));

    // set type
    ethhdr -> ether_type = htons(ETHERTYPE_ARP); 

    //broadcast/ set  MAC
    memset(ethhdr->ether_dhost, 0xff, ETHER_ADDR_LEN);
    // need to find the ether address correspond to the interface
    unsigned char *out_addr_in_if = malloc(sizeof(unsigned char) * 6);
    uint32_t out_ip_in_if = findAddrsForInterface(sr,interface, out_addr_in_if);
    memcpy(ethhdr->ether_shost,out_addr_in_if,6);

    memcpy(arphdr->ar_sha,out_addr_in_if,6);          
    memset(arphdr->ar_tha,0x00,6);

    arphdr -> ar_hrd = htons(ARPHDR_ETHER) ;          
    arphdr -> ar_pro =htons(ETHERTYPE_IP);            
    arphdr -> ar_hln = 6 ;            
    arphdr -> ar_pln = 4;            
    arphdr -> ar_op = htons(ARP_REQUEST);            

    arphdr -> ar_sip = out_ip_in_if; 

    // set target ip to nexthop ip
    arphdr -> ar_tip = nexthopIP;


    sr_send_packet(sr,packet,len,interface);
    printf("Router sent out an ARP request at interface: %s\n", interface);

    // testarp(packet);
    free(packet);

}
//--------------------------------------------------------------------------------
u_short checksum(u_short *buf, int count) {

    int i;
    u_long sum=0;
    for(i=0;i< count;i++){

        sum+= ntohs(*buf);
        buf++;

    }

    return ~((sum +(sum >>16)) & 0xFFFF);

}


// --------------------------------------------------------------------------------
uint32_t sr_getInterfaceAndNexthop(struct sr_instance *sr, 
                                uint32_t iphdrDest,
                                char * interface) {

    struct sr_rt *rt = sr->routing_table;
    char* defautInterface;
    uint32_t defaultNexthop;
    uint32_t nexthop;

    while (rt) {
        if((rt->dest.s_addr) == 0){
            defautInterface = rt->interface;
            defaultNexthop = rt-> gw.s_addr;
            rt = rt->next;
            continue;
        }
        // printf("Comparing routing table\n");
        // sr_print_routing_table(sr);
        if ((iphdrDest & rt->mask.s_addr) == (rt->dest.s_addr & rt->mask.s_addr)){
            //get interface
            strcpy(interface,rt->interface);
            //get nexthop  if this is dest or not
            nexthop = rt->gw.s_addr==0?iphdrDest:rt->gw.s_addr;
            return nexthop;
        }
        
        rt = rt->next;
    }
    // default interface
    rt = sr->routing_table;
    strcpy(interface, defautInterface);
    nexthop = defaultNexthop;



    return nexthop;
}

// --------------------------------------------------------------------------------
void testip(struct ip* iphdr){
    // printf("total length:  %hhu\n", ntohs(iphdr->ip_len));
    // printf("header length in 4bytes word: %u\n", (iphdr->ip_hl));
    // printf("type of service:  %hhu\n", iphdr->ip_tos);
    // printf("sr_protocol:  %hhu\n", iphdr->ip_p);
    // printf("TTL:  %hhu\n", iphdr->ip_ttl);

    
    // uint32_t ipsrc = (iphdr->ip_src).s_addr;
    // uint32_t ipdst = (iphdr->ip_dst).s_addr;

    // printf("%u\n",ipdst);
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
void testarp(uint8_t *packet){

    // printf("%hu\n", ntohs(p->ar_hrd));
    // printf("%hu\n", ntohs(p->ar_pro));

    // printf("%hhu\n", ntohs(p->ar_hln));
    // printf("%hhu\n", ntohs(p->ar_pln));
    // printf("%hu\n", ntohs(p->ar_op));
    // printf("%hu\n", ntohs(p->ar_sip));
    // printf("%hu\n", ntohs(p->ar_tip));


    struct sr_ethernet_hdr *ethernet_p = (struct sr_ethernet_hdr *)packet;
    printf("-------------- Ethernet-dest-address --------------\n");
    printf("%x:%x:%x:%x:%x:%x\n", ethernet_p->ether_dhost[0],
                                ethernet_p->ether_dhost[1],
                                ethernet_p->ether_dhost[2],
                                ethernet_p->ether_dhost[3],
                                ethernet_p->ether_dhost[4],
                                ethernet_p->ether_dhost[5]);
    printf("-------------- Ethernet-dest-address --------------\n");

    printf("-------------- Ethernet-source-address --------------\n");
    printf("%x:%x:%x:%x:%x:%x\n", ethernet_p->ether_shost[0],
                                ethernet_p->ether_shost[1],
                                ethernet_p->ether_shost[2],
                                ethernet_p->ether_shost[3],
                                ethernet_p->ether_shost[4],
                                ethernet_p->ether_shost[5]);
    printf("-------------- Ethernet-source-address --------------\n");

     struct sr_arphdr *p = (struct sr_arphdr *) (sizeof(struct sr_ethernet_hdr) + packet);
    printf("-------------- ARP-OP --------------\n");
    printf("%hu\n", ntohs(p->ar_op));
    printf("-------------- ARP-OP --------------\n");

    printf("-------------- Sender-MAC --------------\n");
    int i;
    for (i=0; i < ETHER_ADDR_LEN; ++i)
    {
        if(i == ETHER_ADDR_LEN-1)
            printf("%x", (p->ar_sha)[i]);
        else
            printf("%x:", (p->ar_sha)[i]);
    }
    printf("\n");
    printf("-------------- Sender-MAC --------------\n");

    printf("-------------- Sender-IP --------------\n");

    unsigned char bytes[4];
    bytes[0] = p->ar_sip & 0xFF;
    bytes[1] = (p->ar_sip >> 8) & 0xFF;
    bytes[2] = (p->ar_sip >> 16) & 0xFF;
    bytes[3] = (p->ar_sip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    printf("-------------- Sender-IP --------------\n");

    printf("-------------- Target-MAC --------------\n");
    for (i=0; i < ETHER_ADDR_LEN; ++i)
    {
        if(i == ETHER_ADDR_LEN-1)
            printf("%x", (p->ar_tha)[i]);
        else
            printf("%x:", (p->ar_tha)[i]);
    }
    printf("\n");
    printf("-------------- Target-MAC --------------\n");

    printf("-------------- Target-IP --------------\n");

    bytes[0] = p->ar_tip & 0xFF;
    bytes[1] = (p->ar_tip >> 8) & 0xFF;
    bytes[2] = (p->ar_tip >> 16) & 0xFF;
    bytes[3] = (p->ar_tip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);    
    printf("-------------- Target-IP --------------\n");
}