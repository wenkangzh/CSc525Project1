#ifndef SR_PACKETHANDLER_H
#define SR_PACKETHANDLER_H



struct ipunit
{	
	// pointer to IP packet

	uint8_t  *packet [300];
	int packetLen [300];
	// dst ip address that used to idetify different IP packet.
	uint32_t sender_ip;

	int counter;

};

struct ipunit* ipbuffer[10];

void handleArp( struct sr_arphdr *,
				struct sr_instance* ,
				struct sr_ethernet_hdr *,
				uint8_t * /* lent */,
				unsigned int len,
				char* /* lent */);


void handleIp(  struct ip* ,
                struct sr_instance* ,
                struct sr_ethernet_hdr *,
                uint8_t * /* lent */,
                unsigned int ,
                char* interface);

void testarp(struct sr_arphdr *p);
void testip(struct ip* );

u_short checksum(u_short  *buf, int count);

uint32_t sr_getInterfaceAndNexthop(struct sr_instance *sr, 
                                uint32_t iphdrDest,
                                char * interface);


void sendArpRequest(
                    struct sr_instance* sr,
                    uint32_t nexthopIP,//pass in final ip in ip header 
                    char* interface // going out inferface
                        );

#endif