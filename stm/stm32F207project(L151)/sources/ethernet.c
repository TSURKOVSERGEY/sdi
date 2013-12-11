#include "main.h"
#include "ethernet.h"
#include "at24c512.h"

extern struct pbuf*            pout[MAX_UDP_SOCK];
extern struct udp_pcb*         pudp_pcb[MAX_UDP_SOCK];
extern udp_message_struct      tx_udp_msg[MAX_UDP_SOCK];
extern udp_message_struct      rx_udp_msg[MAX_UDP_SOCK];
extern int                     rew_status[MAX_UDP_SOCK];
extern uint32_t                rx_udp_msg_size;
extern uint32_t                tx_udp_msg_size;

extern initial_info_struct     info_ini;
extern uint32_t                msg_id;
extern uint32_t                rx_udp_msg_size;
extern uint32_t                tx_udp_msg_size;
extern ethernet_initial_struct eth_ini_dat;

void ETH_Config(void)
{
  //info_ini.at24_error = AT24_Read(0,(uint8_t*)&eth_ini_dat,sizeof(ethernet_initial_struct));
  
  if(crc32(&eth_ini_dat,sizeof(eth_ini_dat)-4,sizeof(eth_ini_dat)-4) != eth_ini_dat.crc)
  {
    eth_ini_dat.IP_ADR[0] = IP_ADDR0;
    eth_ini_dat.IP_ADR[1] = IP_ADDR1;
    eth_ini_dat.IP_ADR[2] = IP_ADDR2;
    eth_ini_dat.IP_ADR[3] = IP_ADDR3;
    
    eth_ini_dat.GW_ADR[0] = GW_ADDR0;
    eth_ini_dat.GW_ADR[1] = GW_ADDR1;
    eth_ini_dat.GW_ADR[2] = GW_ADDR2;
    eth_ini_dat.GW_ADR[3] = GW_ADDR3;
      
    eth_ini_dat.MASK[0] = NETMASK_ADDR0;
    eth_ini_dat.MASK[1] = NETMASK_ADDR1;
    eth_ini_dat.MASK[2] = NETMASK_ADDR2;
    eth_ini_dat.MASK[3] = NETMASK_ADDR3;

    eth_ini_dat.MAC_ADR[0] = MAC_ADDR0;
    eth_ini_dat.MAC_ADR[1] = MAC_ADDR1;
    eth_ini_dat.MAC_ADR[2] = MAC_ADDR2;
    eth_ini_dat.MAC_ADR[3] = MAC_ADDR3;
    eth_ini_dat.MAC_ADR[4] = MAC_ADDR4;
    eth_ini_dat.MAC_ADR[5] = MAC_ADDR5;
     
    for(int i = 0; i < MAX_UDP_SOCK; i++)
    {
      switch(i)
      {
        case 0:
          eth_ini_dat.UDP_RX_PORT[i] = PORT_RX0_UDP; 
          eth_ini_dat.UDP_TX_PORT[i] = PORT_TX0_UDP; 
          IP4_ADDR(&eth_ini_dat.addr[i],ADR0_TX0_UDP, ADR1_TX0_UDP, ADR2_TX0_UDP, ADR3_TX0_UDP); 
        break;
        case 1:
          eth_ini_dat.UDP_RX_PORT[i] = PORT_RX1_UDP; 
          eth_ini_dat.UDP_TX_PORT[i] = PORT_TX1_UDP; 
          IP4_ADDR(&eth_ini_dat.addr[i],ADR0_TX1_UDP, ADR1_TX1_UDP, ADR2_TX1_UDP, ADR3_TX1_UDP); 
        break;
        case 2:
          eth_ini_dat.UDP_RX_PORT[i] = PORT_RX2_UDP; 
          eth_ini_dat.UDP_TX_PORT[i] = PORT_TX2_UDP; 
          IP4_ADDR(&eth_ini_dat.addr[i],ADR0_TX2_UDP, ADR1_TX2_UDP, ADR2_TX2_UDP, ADR3_TX2_UDP); 
        break;
      }
    }
    
  }

  if(ETH_BSP_Config() == ETH_ERROR)
  {
    info_ini.eth_bsp_error = 1;
  }
  else 
  {
    LwIP_Init();
    UDP_Config();
  }
  
}


void UDP_Config(void)
{
  int i;
  
  for(i = 0; i < MAX_UDP_SOCK; i++)
  {
    if((pudp_pcb[i] = udp_new()) == NULL) 
    {
      info_ini.eth_udp_error[i] = 1;
    }
    else if((udp_bind(pudp_pcb[i],IP_ADDR_ANY,eth_ini_dat.UDP_RX_PORT[i]) != ERR_OK))
    {
      info_ini.eth_udp_error[i] = 1;
    }
  
    switch(i)
    {
      case 0: udp_recv(pudp_pcb[i],&udp_rx0_handler,NULL); break;
      case 1: udp_recv(pudp_pcb[i],&udp_rx1_handler,NULL); break;
      case 2: udp_recv(pudp_pcb[i],&udp_rx2_handler,NULL); break;
    }
  
  }
}


void udp_rx0_handler(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  int i;
  uint8_t* prx_udp_msg = (uint8_t*)&rx_udp_msg[SERV];
  struct pbuf *pbuffer = p;
 
   if(p->tot_len <= rx_udp_msg_size) 
   {
     while(1)
     {
       for(i = 0; i < pbuffer->len; i++)
       {
         prx_udp_msg[i] = *(((uint8_t*)pbuffer->payload)+i);
       }
      
       if(pbuffer->next == NULL) break;
       else pbuffer = pbuffer->next;
     }
    
    cmd_handler(SERV);    
   }
  pbuf_free(p);

}
 
void udp_rx1_handler(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  int i;
  uint8_t* prx_udp_msg = (uint8_t*)&rx_udp_msg[PTUK];
  struct pbuf *pbuffer = p;
 
   if(p->tot_len <= rx_udp_msg_size) 
   {
     while(1)
     {
       for(i = 0; i < pbuffer->len; i++)
       {
         prx_udp_msg[i] = *(((uint8_t*)pbuffer->payload)+i);
       }
      
       if(pbuffer->next == NULL) break;
       else pbuffer = pbuffer->next;
     }
    
    cmd_handler(PTUK);   
   }
  pbuf_free(p);

}

void udp_rx2_handler(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  int i;
  uint8_t* prx_udp_msg = (uint8_t*)&rx_udp_msg[SNTP];
  struct pbuf *pbuffer = p;
 
   if(p->tot_len <= rx_udp_msg_size) 
   {
     while(1)
     {
       for(i = 0; i < pbuffer->len; i++)
       {
         prx_udp_msg[i] = *(((uint8_t*)pbuffer->payload)+i);
       }
      
       if(pbuffer->next == NULL) break;
       else pbuffer = pbuffer->next;
     }
    
    cmd_handler(SNTP);   
   }
  pbuf_free(p);  
}

void ReSendUdpData(void)
{
  for(int i = 0; i < MAX_UDP_SOCK; i++)
  {
    if(rew_status[i] == 1)
    {
      pout[i]->tot_len = tx_udp_msg[i].msg_len + UDP_HEADER_SIZE;
    
      if(pbuf_take(pout[i],(uint8_t*)&tx_udp_msg[i],tx_udp_msg[i].msg_len + UDP_HEADER_SIZE) == ERR_OK)
      {
      //  if(udp_sendto(pudp_pcb[i],pout[i],&re_addr[i],re_port[i]) != ERR_OK)  rew_status[i] = 2;
        if(udp_sendto(pudp_pcb[i],pout[i],&eth_ini_dat.addr[i],eth_ini_dat.UDP_TX_PORT[i]) != ERR_OK) rew_status[i] = 2;
      }
    }
    else if(rew_status[i] == 2)
    {
    //  if(udp_sendto(pudp_pcb[i],pout[i],&re_addr[i],re_port[i]) == ERR_OK) rew_status[i] = 0;
      if(udp_sendto(pudp_pcb[i],pout[i],&eth_ini_dat.addr[i],eth_ini_dat.UDP_TX_PORT[i]) != ERR_OK) rew_status[i] = 0;
    }
  }
}


void SendMessage(int id, unsigned int msg_id, void* data, unsigned int len)
{
  if(data)memcpy(&tx_udp_msg[id].data[0],data,len);
  
  tx_udp_msg[id].msg_id  = msg_id;
  tx_udp_msg[id].msg_len = len;
  tx_udp_msg[id].msg_crc = crc32((uint8_t*)&tx_udp_msg[id]+4,(UDP_HEADER_SIZE-4) + tx_udp_msg[id].msg_len,UDP_MESSAGE_SIZE); 
  
  pbuf_free(pout[id]);
  pout[id] = pbuf_alloc(PBUF_TRANSPORT,tx_udp_msg_size,PBUF_RAM);
  
  pout[id]->tot_len = len + UDP_HEADER_SIZE;
     
  if(pbuf_take(pout[id],(uint8_t*)&tx_udp_msg[id],tx_udp_msg[id].msg_len + UDP_HEADER_SIZE) == ERR_OK)
  {
     if(udp_sendto(pudp_pcb[id],pout[id],&eth_ini_dat.addr[id],eth_ini_dat.UDP_TX_PORT[id]) != ERR_OK)  rew_status[id] = 2;
  }
  else rew_status[id] = 1;
}

void SendMessageExt(int id, unsigned int msg_id, void* data, unsigned int len)
{
  if(data)memcpy(&tx_udp_msg[id].data[0],data,len);

  tx_udp_msg[id].msg_id  = msg_id;
  tx_udp_msg[id].msg_len = len;
  tx_udp_msg[id].msg_crc = crc32((uint8_t*)&tx_udp_msg[id]+4,(UDP_HEADER_SIZE-4) + tx_udp_msg[id].msg_len,UDP_MESSAGE_SIZE); 
  
  pbuf_free(pout[id]);
  pout[id] = pbuf_alloc(PBUF_TRANSPORT,tx_udp_msg_size,PBUF_RAM);
  
  pout[id]->tot_len = len + UDP_HEADER_SIZE;
     
  if(pbuf_take(pout[id],(uint8_t*)&tx_udp_msg[id],tx_udp_msg[id].msg_len + UDP_HEADER_SIZE) == ERR_OK)
  {
     if(udp_sendto(pudp_pcb[id],pout[id],&eth_ini_dat.addr[id],eth_ini_dat.UDP_TX_PORT[id]) != ERR_OK)  rew_status[id] = 2;
  }
  else rew_status[id] = 1;
}


