// StmDebuger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <MMSYSTEM.H>
#include <winsock2.h>
#include "StmDebuger.h"

#pragma comment (lib,"WSock32.Lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

#define SB_OK                1
#define SB_NOT_FOUND         2
#define SB_INVALID           3

#define CHECK_CONNECT        1
#define START_AUDIO_STREAM   2
#define STOP_AUDIO_STREAM    3

#define GET_SYS_INFO         20
#define GET_TOTAL_INFO       21

#define GET_SB_HEADER        30
#define GET_PAGE             31
#define GET_TIME             32
#define GET_ETH_PARAM        33


#define GET_SB_HEADER        30
#define GET_PAGE             31
#define GET_TIME             32
#define GET_ETH_PARAM        33
#define GET_BM               34
#define GET_FIXED            35
#define GET_CFI              36


#define SET_GAIN             40
#define SET_TIME             42
#define SET_ETH_PARAM        43
#define FORMAT_MAP_BB        44
#define FORMAT_TWS           45



#define TCP_DATA_SIZE        1026
#define TCP_HEADER_SIZE      12
#define TCP_MESSAGE_SIZE     TCP_HEADER_SIZE + TCP_DATA_SIZE
	
#define ADPCM_BLOCK_SIZE     4048

#define SET_MAC              1
#define SET_GW               2
#define SET_IP               3
#define SET_MASK             4
#define SET_UDP_TX_ADR       5
#define SET_UDP_RX1_PORT     6
#define SET_UDP_TX1_PORT     7
#define SET_UDP_RX2_PORT     8


char STM_ADR[] = {"192.9.206.204"}; 

//#define SERV
#define PTUK
//#define SNTP


#ifdef SERV
  
   #define STM_PORT 30000

#endif
#ifdef PTUK

   #define STM_PORT 40000

#endif
#ifdef SNTP

   #define STM_PORT 50000

#endif


FILE *pf;

/////////////////////////////////////////////////////////////////////////////
#pragma pack(push,1)   
 typedef struct  
 { 
   unsigned int eth_bsp_error;               // (0)     
   unsigned int eth_udp_error[3];            // (1+2+3) 
   unsigned int IS61_error;                  // (4)     
   unsigned int f415_i2c_error;              // (5)    
   unsigned int read_eth_ini_error[2];       // (6+7)   
   unsigned int read_total_time_error[2];    // (8+9)  
   unsigned int read_map_bb_error[2];        // (10+11) 
   unsigned int rtc_error;                   // (12)    
   unsigned int f_write_error;               // (13)    
   unsigned int server_error;                // (14)   
   unsigned int f415_spi1_error;             // (15)  
   unsigned int f415_spi2_error;             // (16)   
   unsigned int f415_adc_config_error;       // (17)   
   unsigned int crc_binar_error;             // (18)    
   unsigned int crc_wr_nand_error;           // (19)    
   unsigned int crc_rd_nand_error;           // (20)  

   unsigned int bad_block_number;            
   unsigned int nand_work_counter[2];     
   unsigned int f415_gain_param[16];      
   unsigned int f415_mode;                 
   unsigned int f207_mode;               
   unsigned int current_time[2];            
   unsigned int total_time[2];   
   
   unsigned int f_o;
   unsigned int f_c;
   unsigned int f_w;






 } total_info_struct;
#pragma pack(pop)

#pragma pack(push,1)  
    struct tcp_message
    {
		unsigned int   msg_crc;
        unsigned int   msg_id;
        unsigned int   msg_len;
        unsigned char  data[TCP_MESSAGE_SIZE];
    } tx_tcp_msg,rx_tcp_msg;
#pragma pack(pop)

#pragma pack(push,1)  
    struct time_struct
    {
		unsigned char  RTC_Hours;
		unsigned char  RTC_Minutes;
        unsigned char  RTC_Seconds;
        unsigned char  RTC_H12;

        unsigned char  RTC_WeekDay;
		unsigned char  RTC_Month;
        unsigned char  RTC_Date;
        unsigned char  RTC_Year;

    } stm_time;
#pragma pack(pop)

#pragma pack(push,1)   
typedef struct 
		{
		  unsigned char MAC_ADR[6];
		  unsigned char GW_ADR[4];
		  unsigned char IP_ADR[4];
		  unsigned char MASK[4];
  		  unsigned int  UDP_RX1_PORT;
		  unsigned int  UDP_RX2_PORT;
		  unsigned int  crc;

		} ethernet_initial_struct;
#pragma pack(pop)

#pragma pack(push,1) 
struct tab_struct
{
	unsigned int unit_index;
	unsigned int sbrw;
	time_struct  time_begin;
    time_struct  time_end;

} ts[2];

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////

CFile       file = NULL;
SOCKET      m_hSocket;
SOCKET      client_socket = NULL; 
SOCKADDR_IN client_addres;

int  param1,param2,param3,param4,param5,param6;
int recive_index = 0;

FILE        *fp;



  const char* error_buffer[] = 
  {	
	" eth_bsp_error",        
	" eth_udp_socet_error (SERVER)",  
	" eth_udp_socet_error (PTUC)", 
	" eth_udp_socet_error (SEV)",           
	" IS61_error",          
    " f415_i2c_error (F207_F415)",
	" read_eth_ini_error (first  copy)",
    " read_eth_ini_error (second copy)",
	" read_total_time_error (first  copy)", 
    " read_total_time_error (second copy)", 
	" read_map_bb_error (first  copy)",       
	" read_map_bb_error (second copy)",       
	" rtc_error",
	" f_write_error",
	" server_error",
	" f415_spi1_error (f415->f207)",
	" f415_spi2_error (f415->pga112)",
	" f415_adc_config_error",
	" crc_binar_error",
	" crc_wr_nand_error",
	" crc_rd_nand_error",

	"\n bad_block_number",
	"\n nand_work_counter (first  plase)",
	"\n nand_work_counter (second plase)",
	"\n f415_gain_param (chanel-0)",
	"\n f415_gain_param (chanel-1)",
	"\n f415_gain_param (chanel-2)",
	"\n f415_gain_param (chanel-3)",
	"\n f415_gain_param (chanel-4)",
	"\n f415_gain_param (chanel-5)",
	"\n f415_gain_param (chanel-6)",
	"\n f415_gain_param (chanel-7)",
	"\n f415_gain_param (chanel-8)",
	"\n f415_gain_param (chanel-9)",
	"\n f415_gain_param (chanel-10)",
	"\n f415_gain_param (chanel-11)",
	"\n f415_gain_param (chanel-12)",
	"\n f415_gain_param (chanel-13)",
	"\n f415_gain_param (chanel-14)",
	"\n f415_gain_param (chanel-15)",
	
	"\n f415_mode"
	"\n f207_mode"
	"\n current_time",
	"\n total_time"
	
  };


/////////////////////////////////////////////////////////////////////////////

void CMD_SetTime(void);
void CMD_GetTime(void);
int  CMD_CheckConnect(void);
void CMD_GetSysInfo(void);
void CMD_GetTotalInfo(void);

void CMD_GetSuperBlock(int f_id1, int f_id2);

void CMD_Start(void);
void CMD_Stop(void);
void CMD_SetEthParam(int mode);
void CMD_GetEthParam(void);
void CMD_FormatMapBB(void);
void CMD_FormatTWS(void);


//int  CMD_SetFixed(unsigned char* pfixed_index);
//int  CMD_GetFixed(unsigned char* pfixed_index);

//int  CMD_GetBmark(int param);
void CMD_SetIP(void);
void CMD_GetIP(void);
void CMD_SetIndex(void);
//void CMD_GetAll(void);
void CMD_fClose(void);
int  CMD_fOpen(void);
//int  CMD_GetCloseFileIndex(unsigned char *pindex);
//int  CMD_SetCloseFileIndex(void);



int  UdpInitial(void);
void SendMessage(unsigned int msg_id, void* data, unsigned int len);
int  RecvMessage(unsigned int msg_id, void* data, unsigned int len);
int  SendRecvMessage(unsigned int send_id,void *psend,unsigned int slen,unsigned int recv_id,void *precv, unsigned int rlen, int time_out);
void PrintFileScrool(unsigned int f_num, unsigned int page_adress, unsigned int total_page);
void CMD_SetSysinfo(void);

unsigned int crc32(void * pcBlock, unsigned short len, unsigned short tot_len);
unsigned int crc32(unsigned int crc, void * pcBlock, unsigned short len, unsigned short tot_len);



/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

void main_handler(void);

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.

		main_handler();
		CString strHello;
		strHello.LoadString(IDS_HELLO);
		cout << (LPCTSTR)strHello << endl;
	}

	return nRetCode;
}

int UdpInitial(void)
{
  WSADATA WSAData; 
  
  if(client_socket) 
  {
	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
	  Sleep(300);
  }

  if(WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
  {
	  printf("\r Ошибка инициализации WSAStartup \n"); 
	  return 0;
  }

  client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	
  if(client_socket == INVALID_SOCKET)
  { printf("\r Ошибка создания сокета \n"); 
	return 0;
  }
 
  
  client_addres.sin_family      = AF_INET;
  client_addres.sin_addr.s_addr = INADDR_ANY;
  client_addres.sin_port        = htons(STM_PORT);
   
  if(bind(client_socket,(LPSOCKADDR)&client_addres, sizeof(client_addres)) == SOCKET_ERROR)
  {
	  closesocket(client_socket);
	  printf("\r Ошибка Bind \n"); 
      return 0;
  }

	
  client_addres.sin_addr.s_addr = inet_addr(STM_ADR); 

  return 1;

}

int GetID(char *name)
{
  const char* buffer[] = 
  {	
	"connect",        // 0
	"start",          // 1
    "stop",           // 2
	"info",           // 3
	"total",          // 4
    "gain",           // 5
	"gsb",            // 6
	"settime",        // 7
	"gettime",        // 8
	"eth_mac",        // 9
	"eth_gw",         // 10
    "eth_ip",         // 11
    "eth_mask",       // 12
    "eth_udptxadr",   // 13
    "eth_udprx1port", // 14
    "eth_udptx1port", // 15
    "eth_udprx2port", // 16
    "geteth",         // 17
	"mapbb",          // 18
	"tws",            // 19
	"fix",            // 20
	"iip",            // 21
	"ip",             // 22
	"index",          // 23
	"getall"          // 24
  };

  for(int i = 0; i <= 24; i++ )
  { if((strcmp(buffer[i],name)) == 0) return i;
  }
  
  return -1;
}

/*

void GetAllTimer(void)
{
	int timer = 0;

	while(1)
	{
		Sleep(100);

		if(timer++ == 500)
		{
			timer = 0;
			CMD_GetAll();
		}
	
	
	}


}
*/

void main_handler(void)
{
	char buffer[100];
	char cmd[100];

	printf("\n stmdebuger ver 1.0 udp_port %d \n",STM_PORT);

	printf("\n Wait ");

	for(int i = 0; i < 70; i++)
	{
		printf("*");
		Sleep(30);

	}

	printf("\n");

	fp = fopen("output_debug.txt","w");

    if(!CMD_CheckConnect())
	{
		Sleep(2000);
		return;
	}

	CMD_SetTime();

	while(1)
	{
		printf("\n \r ");
		
		memset(buffer,0,sizeof(buffer));

		gets(buffer); 

		if(buffer[0] != 110) sscanf(buffer,"%s %d %d %d %d",cmd,&param1,&param2,&param3,&param4,&param5,&param6);

   	    switch(GetID(cmd))
		{
		    case 0:  CMD_CheckConnect();         break;
 			case 1:  CMD_Start();                break;
 			case 2:  CMD_Stop();                 break;
 			case 3:  CMD_GetSysInfo();           break;
 			case 4:  CMD_GetTotalInfo();         break;
//			case 5:  SetGain();                  break;
			case 6:  
			
				if(CMD_fOpen())
				{
					CMD_GetSuperBlock(param1,param2); 
					CMD_fClose();
				}
				
			break;
 			case 7:  CMD_SetTime();              break;
 			case 8:  CMD_GetTime();              break;
            case 9:  CMD_SetEthParam(1);         break;
            case 10: CMD_SetEthParam(2);         break;
            case 11: CMD_SetEthParam(3);         break;
            case 12: CMD_SetEthParam(4);         break;
            case 13: CMD_SetEthParam(5);         break;
            case 14: CMD_SetEthParam(6);         break;
            case 15: CMD_SetEthParam(7);         break;
            case 16: CMD_SetEthParam(8);         break;
            case 17: CMD_GetEthParam();          break;
            case 18: CMD_FormatMapBB();          break;
            case 19: CMD_FormatTWS();            break;

//          case 19: CMD_GetBmark(param1);       break;
//          case 20: CMD_SetFixed(NULL);         break;
            case 21: CMD_GetIP();                break;
            case 22: CMD_SetIP();                break;
            case 23: CMD_SetIndex();             break;
            
			case 24: 
			
//				GetAllTimer();
				//CMD_GetAll();               
				
		    break;




			default: printf("\r incorrect command \n"); 	
			break;
		}
	}
}


void CMD_SetIP(void)
{
	sprintf(STM_ADR,"%d.%d.%d.%d",param1 & 0xff,param2 & 0xff,param3 & 0xff,param4 & 0xff);
	printf("\n IP DONE \n");
}

void CMD_SetIndex(void)
{
	recive_index = param1 & 0x1;
	printf("\n INDEX DONE \n");
}

void CMD_GetIP(void)
{
	printf("\n current IP: ");
	printf(STM_ADR); 	
	printf("\n");

}

int GetConnect(void)
{
  WSADATA WSAData;   

  //const char STM_ADR[] = {"192.9.206.202"}; 


  if(client_socket) 
  {
	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
	  Sleep(300);
  }

  if((WSAStartup(MAKEWORD(1,1), &WSAData)) != 0 ) 
  { 
	  printf("\r Failure initial WSAStartupОШИБКА ! ! ! \n"); 
      return 0;
  }
    
  if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
	  printf("\r Failure socket create \n");  
      return 0;
  }

  client_addres.sin_family      = AF_INET;
  client_addres.sin_addr.s_addr = inet_addr(STM_ADR); 
  client_addres.sin_port        = htons(STM_PORT); 

  if((connect(client_socket,(const struct sockaddr*)&client_addres,sizeof(struct sockaddr_in))) != 0 )
  {
	  printf("\r Failure tcp connect \n"); 
	  return 0;
  }

  return 1;
}


void SendMessage(unsigned int msg_id, void* data, unsigned int len)
{
	tx_tcp_msg.msg_id   = msg_id;
	tx_tcp_msg.msg_len  = len;
	if(data)memcpy(&tx_tcp_msg.data[0],data,len);
	tx_tcp_msg.msg_crc  = crc32(&tx_tcp_msg.msg_id,tx_tcp_msg.msg_len + TCP_HEADER_SIZE-4,sizeof(tx_tcp_msg));
    sendto(client_socket,(char*)&tx_tcp_msg,TCP_HEADER_SIZE + tx_tcp_msg.msg_len,0,(PSOCKADDR)&client_addres,sizeof(client_addres));
}

int RecvMessage(unsigned int msg_id, void* data, unsigned int len)
{

	int status = 0,time_out = 2, result;

	fd_set  r;
	timeval t;

	while(time_out-- > 0)
    {

       FD_ZERO(&r);

       t.tv_sec  = 0; 
       t.tv_usec = 1000000; // 100 микро секунды

       FD_SET(client_socket,&r);

	   if(SOCKET_ERROR == (result = select (client_socket, &r, NULL, NULL, &t)))
	   {
		   return status;
	   }
	   else if((result != 0) && (FD_ISSET(client_socket,&r)))
	   {
		   time_out = 0;
		   break;
	   }
	}

	if(time_out < 0) return status;
 
	if(recv(client_socket,(char*)&rx_tcp_msg,sizeof(rx_tcp_msg),0) <= 0) return status;

	if(crc32(&rx_tcp_msg.msg_id,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,sizeof(tx_tcp_msg)) == rx_tcp_msg.msg_crc)
	{
		if(rx_tcp_msg.msg_id == msg_id)
		{
			status = 1;

			if(data)
			{
				memcpy(data,&rx_tcp_msg.data[0],len);
			}
				
		}
			else printf("\r Failure msg_id \n"); 
	}
		else printf("\r Failure msg_crc \n"); 
 
  return status;
}


int SendRecvMessage(unsigned int send_id,void *psend,unsigned int slen,unsigned int recv_id,void *precv, unsigned int rlen, int time_out)
{
	while(time_out-- > 0)
	{
		SendMessage(send_id,psend,slen);
		
		if(RecvMessage(recv_id,precv,rlen)) return 1;;
 	}

  return 0;
}


void CMD_GetEthParam(void)
{
	ethernet_initial_struct eis;
 	
	if(!UdpInitial()) return;
 
	if(SendRecvMessage(GET_ETH_PARAM,NULL,0,GET_ETH_PARAM+100,&eis,sizeof(eis),3))
	{
		printf("\n mac_adr:      %d %d %d %d %d %d \n",eis.MAC_ADR[0],eis.MAC_ADR[1],eis.MAC_ADR[2],eis.MAC_ADR[3],eis.MAC_ADR[4],eis.MAC_ADR[5]);
       	printf("\r gw_adr:       %d %d %d %d \n",eis.GW_ADR[0],eis.GW_ADR[1],eis.GW_ADR[2],eis.GW_ADR[3]);
       	printf("\r ip_adr:       %d %d %d %d \n",eis.IP_ADR[0],eis.IP_ADR[1],eis.IP_ADR[2],eis.IP_ADR[3]); 
       	printf("\r mask:         %d %d %d %d \n",eis.MASK[0],eis.MASK[1],eis.MASK[2],eis.MASK[3]);
       	printf("\r udp_rx1_port: %d \n",eis.UDP_RX1_PORT);
       	printf("\r udp_rx2_port: %d \n",eis.UDP_RX2_PORT);

	}
	else
	{
		printf("\n get net param FAILURE \n");
	}
}


void CMD_SetEthParam(int mode)
{
	int msg_len;

	tx_tcp_msg.data[0]  = mode;
	 	
	switch(mode)
	{
	  case SET_MAC:

	   tx_tcp_msg.data[1] = param1;
	   tx_tcp_msg.data[2] = param2;
	   tx_tcp_msg.data[3] = param3;
	   tx_tcp_msg.data[4] = param4;
	   tx_tcp_msg.data[5] = param5;
	   tx_tcp_msg.data[6] = param6;
	   msg_len = 11;
      break;

	  case SET_GW:
	  case SET_IP:
	  case SET_MASK:
	  case SET_UDP_TX_ADR:

  	   tx_tcp_msg.data[1] = param1;
	   tx_tcp_msg.data[2] = param2;
	   tx_tcp_msg.data[3] = param3;
	   tx_tcp_msg.data[4] = param4;
  	   msg_len = 9;
	  break;
	  
	  case SET_UDP_RX1_PORT:
 	  case SET_UDP_TX1_PORT:
	  case SET_UDP_RX2_PORT:

	  *((unsigned int*)&tx_tcp_msg.data[1]) = param1;
  	  msg_len = 5;
	  break;

	}


 	if(!UdpInitial()) return;

	if(SendRecvMessage(SET_ETH_PARAM,NULL,msg_len,SET_ETH_PARAM+100,NULL,0,3))
	{
		printf("\n set_eth_param DONE \n");
	}
	else
	{
		printf("\n set_eth_param FAILURE \n"); 
	}

}


int CMD_CheckConnect(void)
{
	if(!UdpInitial()) return 0;
 
	printf("\n check connect");

	if(SendRecvMessage(CHECK_CONNECT,NULL,0,CHECK_CONNECT+100,NULL,0,3))
	{
		printf(" DONE \n");
		
		return 1;
	}
	else
	{
		printf(" FAILURE \n"); 
	}
	
	return 0;
}


/*
void CMD_SetSysinfo(void)
{
	if(!UdpInitial()) return;
 
	if(SendRecvMessage(SET_SYS_INFO_0,NULL,0,SET_SYS_INFO_0+100,NULL,0,3))
	{
			
		printf("\n set request l151 done");
	}
	else
	{
		printf("\n set request l151 failure");
	}
}
*/


void CMD_GetSysInfo(void)
{
	printf("\n");

	if(!UdpInitial()) return;


	int cmd = GET_SYS_INFO;

 	SendMessage(cmd,NULL,0);
	
	
	if(RecvMessage(cmd+100,NULL,0) == 0)
	{
		printf("\n GetSysInfo Failure \n ");
		return;
	}


   struct tab
   {   unsigned int unit_index;
       unsigned int sbrw;
	   time_struct  time_begin;
       time_struct  time_end;
   } tab[2];
			   
			   
  memcpy(tab,&rx_tcp_msg.data[0],sizeof(tab));

   for(int i = 0; i < 2; i++)
   {
	   printf("\n unit_index  %d ",i);
       printf("\n current_write_index  %d \n",tab[i].unit_index);
	   printf("\r total files write  %d   \n",tab[i].sbrw);
       printf("\r time begin %dh : %dm : %ds   %d-%d-%d \n",tab[i].time_begin.RTC_Hours,tab[i].time_begin.RTC_Minutes,tab[i].time_begin.RTC_Seconds,tab[i].time_begin.RTC_Date,tab[i].time_begin.RTC_Month,tab[i].time_begin.RTC_Year+2000);
       printf("\r time end   %dh : %dm : %ds   %d-%d-%d \n",tab[i].time_end.RTC_Hours,tab[i].time_end.RTC_Minutes,tab[i].time_end.RTC_Seconds,tab[i].time_end.RTC_Date,tab[i].time_end.RTC_Month,tab[i].time_end.RTC_Year+2000);
   }

}


void CMD_GetTotalInfo(void)
{
	printf("\n");

 	total_info_struct tws;
	unsigned int *p = (unsigned int*) &tws;
	
	if(!UdpInitial()) return;

 	SendMessage(GET_TOTAL_INFO,NULL,0);
	
	
	if(RecvMessage(GET_TOTAL_INFO+100,&tws,sizeof(total_info_struct)) == 0)
	{
		printf("\n GetTotalInfo Failure \n ");
		return;
	}


	for(int i = 0; i < 21; i++)
	{
		
		
		if(*(p+i) == 0)
		{
			printf("\n OK  ");
		}
		else
		{
			printf("\n FAIL");
		}

		printf(error_buffer[i]);

	}

	printf("\n");
	printf("\n nand_work_counter = nand_0 = %d  nand_1 = %d",tws.nand_work_counter[0],tws.nand_work_counter[1]);

	unsigned int total_hour =  tws.total_time[0] / 3600;
	unsigned int total_min  = (tws.total_time[0] / 60) - (total_hour * 60);

    printf("\n total time work = %d (hour) %d (min)",total_hour,total_min);


	Sleep(0);
}


int GetTime(void)
{
	static int time[3] = {0,0,0}; 
  
	int dt = 0;
  
	if (time[1] == 0)
	{
		time[1] = time[2] = timeGetTime();
	}
    else
	{
		time[0] = timeGetTime();
        
		dt = time[0] - time[2];
        
		if(dt > 50)
		{
			time[2] = time[0];
		}
   
		else dt = 0;
  }
 return dt;
}


			
void PrintSBHeaderInfo(void *p)
{	
	#pragma pack(push,1)  
	struct super_block_header_struct
	{
		unsigned char  status;
		time_struct    time_open;
		time_struct    time_close;
		unsigned int   sb_num;
        unsigned int   page_real_write;

	} *psbh = (struct super_block_header_struct*) p;
    #pragma pack(pop) 

	printf("\n");
	printf("\n");
    printf("\r file number     %d / %d \n",recive_index,psbh->sb_num);
 	printf("\r time open       %d:%d:%d   %d.%d.%d \n",psbh->time_open.RTC_Hours,psbh->time_open.RTC_Minutes,psbh->time_open.RTC_Seconds,psbh->time_open.RTC_Date,psbh->time_open.RTC_Month,psbh->time_open.RTC_Year+2000);
 	printf("\r time close      %d:%d:%d   %d.%d.%d \n",psbh->time_close.RTC_Hours,psbh->time_close.RTC_Minutes,psbh->time_close.RTC_Seconds,psbh->time_close.RTC_Date,psbh->time_close.RTC_Month,psbh->time_close.RTC_Year+2000);
    printf("\r page_real_write %d \n",psbh->page_real_write);

	if(pf) fprintf(pf,"id = %d     fnum = %d     time = %d:%d:%d  - %d.%d.%d \n",recive_index,psbh->sb_num,psbh->time_open.RTC_Hours,psbh->time_open.RTC_Minutes,psbh->time_open.RTC_Seconds,psbh->time_close.RTC_Hours,psbh->time_close.RTC_Minutes,psbh->time_close.RTC_Seconds);


}


void PrintFileScrool(unsigned int f_num, unsigned int page_adress, unsigned int total_page)
{

    static int page_index = 1;
	static int page_adress_old = 0;

	unsigned int step = total_page / 10;

	if(page_adress_old > page_adress)
	{
		page_index = 1;
	}
	
	if(page_adress >= step * page_index)
	{
		printf("\r file %d recive   %d%% ",f_num,page_index*10);
		page_index++;
	}

	page_adress_old = page_adress;
}


	
int TimeDetect(time_struct t0, time_struct t1)
{
//	if(t0.RTC_Year > t1.RTC_Year) return 0;
//	else if(t0.RTC_Month > t1.RTC_Month) return 0;
//	     else if(t0.RTC_Date > t1.RTC_Date) return 0;

	unsigned int ft0,ft1; 
		
	ft0 = (t0.RTC_Hours * 60 * 60) + (t0.RTC_Minutes * 60) + t0.RTC_Seconds;

	ft1 = (t1.RTC_Hours * 60 * 60) + (t1.RTC_Minutes * 60) + t1.RTC_Seconds;

	fprintf(pf,"\r t0 = %d t1 = %d \n",ft0,ft1);

	if(ft0 < ft1) return 1;

//			   else if(t0.RTC_Hours > t1.RTC_Hours) return 0;
//					else if(t0.RTC_Minutes > t1.RTC_Minutes) return 0;
//						  else if(t0.RTC_Seconds > t1.RTC_Seconds) return 0;

	return 0;
}

/*

void CMD_GetAll(void)
{
	static int i = 0;
	char buffer[100];

	sprintf(buffer,"time_debug%d.txt",i++);

	pf = fopen(buffer,"w");

	unsigned char time_out = 0;
	unsigned char fixed_status[2] = {0,0};
    unsigned char close_file_index = 1;

	if(CMD_SetFixed(&fixed_status[0]) == 0) return;

	while(fixed_status[0] != fixed_status[1])
	{
	    Sleep(100);

	    if(CMD_GetFixed(&fixed_status[1]) == 0)	
		{
			return;
		}
		else if(time_out++ > 15) 
		{
			printf("\n time_out FAILURE \n");
			return;
		}

	}

	
	if(CMD_GetBmark(0) == 0) return;

	
	if(CMD_fOpen() == 0) return;

///////////////////////////////////// ЧТЕНИЕ ///////////////////////////////////// 


	if((ts[0].sbrw > 0) && (ts[1].sbrw == 0)) // Читаю только Nand-0
	{
		recive_index = 0;
//	    CMD_GetSuperBlock(ts[0].file_index[0],ts[0].file_index[1]);
	}
	else if((ts[1].sbrw > 0) && (ts[0].sbrw == 0)) // Читаю только Nand-1
	{
		recive_index = 1;
//	    CMD_GetSuperBlock(ts[1].file_index[0],ts[1].file_index[1]);
	}
	else 
	{
		if(TimeDetect(ts[0].time_begin,ts[1].time_begin)) // Читаю  Nand-0 -> Nand-1
		{
			printf("\r t[0] -> t[1] \n");      // ( t0 < t1 )
	    	fprintf(pf,"\r t[0] -> t[1] \n");

    		if(ts[0].sbrw > 0)
			{
				recive_index = 0;
//	            CMD_GetSuperBlock(ts[0].file_index[0],ts[0].file_index[1]);
			}
	
			if(ts[1].sbrw > 0)
			{
				recive_index = 1;
//	            CMD_GetSuperBlock(ts[1].file_index[0],ts[1].file_index[1]);
			}

		}
  	    else
		{
			printf("\r t[1] -> t[0] \n");
    		fprintf(pf,"\r t[1] -> t[0] \n");

	    	if(ts[1].sbrw > 0)
			{
				recive_index = 1;
//	            CMD_GetSuperBlock(ts[1].file_index[0],ts[1].file_index[1]);
			}
	
    		if(ts[0].sbrw > 0)
			{
				recive_index = 0;
//	            CMD_GetSuperBlock(ts[0].file_index[0],ts[0].file_index[1]);
			}
		}
	}

 
	CMD_fClose();

	fclose(pf);
}
	
*/

int CMD_fOpen(void)
{
	if(file.Open("nand_page.dat",CFile::modeCreate | CFile::modeWrite) == NULL)
	{
		printf("\r Failure open file \n");
		return 0;
	}

	return 1;
}


void CMD_fClose(void)
{
   file.Close();
}



void CMD_GetSuperBlock(int f_id1, int f_id2)
{
    #define TIME_OUT 5

	unsigned char adpcm_block[ADPCM_BLOCK_SIZE];

	unsigned int* padpcm_crc = (unsigned int*)&adpcm_block[ADPCM_BLOCK_SIZE-4];

    unsigned int page_adr,time_out = 0;

	unsigned int get_sb_header_msg[2];

	memset(&adpcm_block,0,sizeof(adpcm_block));

	#define PAGE_HEADER_SIZE 6
    
	#pragma pack(push,1)  
	struct page_header_struct
	{
        unsigned char  page_index;
        unsigned char  unit_index;
        unsigned int   page_address;
	}   
	*pph = (struct page_header_struct*) &rx_tcp_msg.data[0];
    #pragma pack(pop) 

	#pragma pack(push,1)  
	struct super_block_header_struct
	{
		unsigned char  status;
		time_struct    time_open;
		time_struct    time_close;
		unsigned int   sb_num;
        unsigned int   page_real_write;

	} sbh;
    #pragma pack(pop) 

	if(f_id1 > f_id2) 
	{
		printf("\r Failure set address \n");
	    return;
	}

	printf("\n recive file %d - %d \n",f_id1,f_id2);

	while(f_id1 <= f_id2)
	{
		get_sb_header_msg[0] = recive_index;
		get_sb_header_msg[1] = f_id1;

					
		if(SendRecvMessage(GET_SB_HEADER,&get_sb_header_msg,8,GET_SB_HEADER+100,&sbh,sizeof(sbh),3))
		{
			if(sbh.status == SB_OK)
			{
				page_adr = 0;
		    	f_id1++;
				PrintSBHeaderInfo(&sbh);
			}
			else if(sbh.status == SB_NOT_FOUND)
			{
				printf("\n file ( %d ) not found  \n",f_id1);
				f_id1++;
				continue;
			}
			else if(sbh.status == SB_INVALID)
			{
				printf("\n");
				printf("\n file ( %d ) invalid \n",f_id1);
				f_id1++;
				continue;
			}
		}
        else
		{   printf("\n get_sb_header ( %d ) Failure \n",f_id1);
		    continue;
		}

 
		while(page_adr <= sbh.page_real_write)
		{			
			if(time_out-- == 0) 
			{
				SendMessage(GET_PAGE,&page_adr,4);
				time_out = TIME_OUT;
			}

			if(RecvMessage(GET_PAGE+100,NULL,0)) 
			{
				switch(pph->page_index)
				{
				   case 4: 

					 memcpy(adpcm_block,&rx_tcp_msg.data[PAGE_HEADER_SIZE],ADPCM_BLOCK_SIZE / 4);
			 		 time_out = TIME_OUT;

                   break;
			       case 3:

					 memcpy(adpcm_block + (ADPCM_BLOCK_SIZE / 4),&rx_tcp_msg.data[PAGE_HEADER_SIZE],ADPCM_BLOCK_SIZE / 4);
					 time_out = TIME_OUT;
					 
				   break;
				   case 2: 

				     memcpy(adpcm_block + (ADPCM_BLOCK_SIZE / 4 * 2),&rx_tcp_msg.data[PAGE_HEADER_SIZE],ADPCM_BLOCK_SIZE / 4); 
					 time_out = TIME_OUT;

				   break;
			       case 1: 
							
				     memcpy(adpcm_block + (ADPCM_BLOCK_SIZE / 4 * 3),&rx_tcp_msg.data[PAGE_HEADER_SIZE],ADPCM_BLOCK_SIZE / 4);
	    				
					 if(crc32(adpcm_block,ADPCM_BLOCK_SIZE-4,ADPCM_BLOCK_SIZE) == *padpcm_crc)
					 {
					    file.Write(adpcm_block,ADPCM_BLOCK_SIZE);
					    PrintFileScrool(f_id1-1,page_adr++,sbh.page_real_write);
			
					 }
					 else
					 {
					     printf("\n file %d page %d crc error \n",f_id1-1,page_adr++);
					 }

				     time_out = 0;
					 continue;

				   break;

				   default:

				    printf("\n file %d page %d crc error (message from stm) \n",f_id1-1,page_adr++);
					time_out = 0;
				   continue;

				   break;
				}
						 	 
			}
			else
			{
					OutputDebugString("\n rcv err");
			
			}
		}
	}

	printf("\n");
	printf("\n recive done \n");

}



void CMD_SetTime(void)
{
	int status = 0;

	CTime pc_time;

	pc_time = pc_time.GetCurrentTime(); 

	stm_time.RTC_Seconds = (unsigned char) pc_time.GetSecond();
    stm_time.RTC_Minutes = (unsigned char) pc_time.GetMinute();
	stm_time.RTC_Hours   = (unsigned char) pc_time.GetHour();

	stm_time.RTC_Year    = (unsigned char) pc_time.GetYear() - 2000;
	stm_time.RTC_Month   = (unsigned char) pc_time.GetMonth();
	stm_time.RTC_Date    = (unsigned char) pc_time.GetDay();
	stm_time.RTC_WeekDay = (unsigned char) pc_time.GetDayOfWeek();
	stm_time.RTC_H12     = (unsigned char) pc_time.GetGmtTm();

 	if(!UdpInitial()) return;

	for(int i = 0; i < 3; i++)
	{
		SendMessage(SET_TIME,&stm_time,8);
		
		if(status = RecvMessage(SET_TIME+100,NULL,0)) 
		{
			printf("\n %d:%d:%d   %d.%d.%d \n",stm_time.RTC_Hours,stm_time.RTC_Minutes,stm_time.RTC_Seconds,stm_time.RTC_Date,stm_time.RTC_Month,stm_time.RTC_Year+2000);
			break;
		}
	}

	if(!status)
	{
		printf("\n set_time FAILURE \n"); 
	}


}

void CMD_FormatMapBB(void)
{
	int status = 0;

 	if(!UdpInitial()) return;


	for(int i = 0; i < 3; i++)
	{
		SendMessage(FORMAT_MAP_BB,NULL,0);
		
		if(status = RecvMessage(FORMAT_MAP_BB+100,NULL,0)) 
		{
			printf("\r format map_bb DONE\n");
			break;
		}
	}

	if(!status)
	{
		printf("\r format map_bb FAILURE \n");
	}
}


void CMD_FormatTWS(void)
{
	int status = 0;

 	if(!UdpInitial()) return;


	for(int i = 0; i < 3; i++)
	{
		SendMessage(FORMAT_TWS,NULL,0);
		
		if(status = RecvMessage(FORMAT_TWS+100,NULL,0)) 
		{
			printf("\r format tws DONE\n");
			break;
		}
	}

	if(!status)
	{
		printf("\r format tws FAILURE \n");
	}
}

/*
int CMD_GetBmark(int param)
{


	return 0;
}
*/

void CMD_GetTime(void)
{

	if(!UdpInitial()) return;


	if(SendRecvMessage(GET_TIME,NULL,0,GET_TIME+100,&stm_time,sizeof(stm_time),3))
	{
		printf("\n %dh : %dm : %ds   %d-%d-%d \n",stm_time.RTC_Hours,stm_time.RTC_Minutes,stm_time.RTC_Seconds,stm_time.RTC_Date,stm_time.RTC_Month,stm_time.RTC_Year+2000);
	}
    else
	{
		printf("\n get time FAILURE \n");
	}

}

/*
int CMD_SetFixed(unsigned char* pfixed_index)
{
	static unsigned char fixed_index = 1;

	if(SendRecvMessage(SET_FIXED,&fixed_index,1,SET_FIXED+100,NULL,0,3))
	{
		printf("\n set fixed DONE <fixed_index = %d> \n",fixed_index);
        if(pfixed_index)
		{
			*pfixed_index = fixed_index;
		}
		fixed_index++;
		return 1;
	}    
	else 
	{
		printf("\n set fixed FAILURE \n"); 
	}

	return 0;

}

int CMD_GetFixed(unsigned char *pfixed_index)
{

	if(SendRecvMessage(GET_FIXED,NULL,0,GET_FIXED+100,pfixed_index,1,3))
	{	printf("\n get fixed DONE <fixed_index = %d> \n",*pfixed_index);
		return 1;
	}    
	else 
	{
		printf("\n get fixed FAILURE \n"); 
	}

	return 0;
}


int CMD_SetCloseFileIndex(void)
{

	if(SendRecvMessage(SET_CFI,NULL,0,SET_CFI+100,NULL,0,3))
	{	printf("\n close stm file DONE \n");
		return 1;
	}    
	else 
	{
		printf("\n close stm file  FAILURE \n"); 
	}

	return 0;
}


int CMD_GetCloseFileIndex(unsigned char *pindex)
{

	if(SendRecvMessage(GET_CFI,NULL,0,GET_CFI+100,pindex,1,3))
	{
		return 1;
	}    
	else 
	{
		printf("\n get index FAILURE \n"); 
	}

	return 0;
}
*/

void CMD_Start(void)
{
	if(!UdpInitial()) return;

	if(SendRecvMessage(START_AUDIO_STREAM,NULL,0,START_AUDIO_STREAM+100,NULL,0,3))
	{
		printf("\n start record DONE \n");
	}
    else 
	{
		printf("\n start record FAILURE \n"); 
	}
	
}


void CMD_Stop(void)
{
	if(!UdpInitial()) return;

	if(SendRecvMessage(STOP_AUDIO_STREAM,NULL,0,STOP_AUDIO_STREAM+100,NULL,0,3))
	{
		printf("\n stop record DONE \n");
	}
    else 
	{
		printf("\n stop record FAILURE \n"); 
	}
	
}


unsigned int crc32(void * pcBlock, unsigned short len, unsigned short tot_len)
{
	if(len > tot_len) return 0;

	unsigned char *p = (unsigned char*)pcBlock;
	
	static unsigned int Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

    unsigned int crc = -1;
    while (len--)
        crc = (crc >> 8) ^ Crc32Table[(crc ^ *p++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}


unsigned int crc32(unsigned int crc, void * pcBlock, unsigned short len, unsigned short tot_len)
{
	if(len > tot_len) return 0;

	unsigned char *p = (unsigned char*)pcBlock;
	
	static unsigned int Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

    while (len--)
        crc = (crc >> 8) ^ Crc32Table[(crc ^ *p++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

