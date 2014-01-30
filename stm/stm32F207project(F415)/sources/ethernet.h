////////////////////////////////////////////////////////////////////////////////
// ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * 
////////////////////////////////////////////////////////////////////////////////

#define MAC_ADDR0       0x00 
#define MAC_ADDR1       0x40
#define MAC_ADDR2       0x45
#define MAC_ADDR3       0x31
#define MAC_ADDR4       0x32
#define MAC_ADDR5       IP_ADDR3

#define IP_ADDR0        192
#define IP_ADDR1        9
#define IP_ADDR2        206
#define IP_ADDR3        204

#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

#define GW_ADDR0        IP_ADDR0
#define GW_ADDR1        IP_ADDR1
#define GW_ADDR2        IP_ADDR2
#define GW_ADDR3        IP_ADDR3

////////////////////////////// SERV ////////////////////////////////////////////
#define ADR0_TX0_UDP    192
#define ADR1_TX0_UDP    9
#define ADR2_TX0_UDP    206
#define ADR3_TX0_UDP    251 //124
#define PORT_RX0_UDP    30000
#define PORT_TX0_UDP    30000 //30001

////////////////////////////// PTUK ////////////////////////////////////////////
#define ADR0_TX1_UDP    192
#define ADR1_TX1_UDP    9
#define ADR2_TX1_UDP    206
#define ADR3_TX1_UDP    251
#define PORT_RX1_UDP    40000
#define PORT_TX1_UDP    40000

////////////////////////////// SNTP ////////////////////////////////////////////
#define ADR0_TX2_UDP    192
#define ADR1_TX2_UDP    9
#define ADR2_TX2_UDP    206
#define ADR3_TX2_UDP    251
#define PORT_RX2_UDP    50000
#define PORT_TX2_UDP    50000


void UDP_Config(void);
void SendMessage(int id,unsigned int msg_id, void* data, unsigned int len);
void SendMessageExt(int id, unsigned int msg_id, void* data, unsigned int len);
void ReSendUdpData(void);

