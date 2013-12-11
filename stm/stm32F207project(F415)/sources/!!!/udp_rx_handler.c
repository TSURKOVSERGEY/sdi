#include "main.h"


extern tcp_message_struct  rx_tcp_msg;

void udp_rx_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  static int i = 0, j = 0, m = 0, n = 0;
  static uint8_t buffer[4096];
  struct pbuf *pbuffer = p;
  uint8_t *prx_tcp_msg;
  
  
  while(1)
  {
    for(m = 0; m < pbuffer->len; m++)
    {
      buffer[n++&0xfff] = *(((uint8_t*)pbuffer->payload)+m);
    }
      
    if(pbuffer->next == NULL) break;
    else pbuffer = pbuffer->next;
  }
  
  prx_tcp_msg = (uint8_t*)&rx_tcp_msg;
     
  for(m  = 0; m < p->tot_len; m++)  
  {
    prx_tcp_msg[i++] = buffer[j++&0xfff];
    
    if(i >= 12) 
    {
      if(i == rx_tcp_msg.msg_len + 12)
      { i = 0;
        tcp_rx_handler();
      }
    }
  }
   
  pbuf_free(p);
}

