#include "main.h" 
#include "time.h" 
#include "ethernet.h" 

struct tm       timestruct;     
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DataStruct;
STNP_Struct     SNTP_Msg;

__IO uint32_t dsec = 0;
  
void sntp_handler(void)
{  
  RTC_GetDate(0, &RTC_DataStruct);
  RTC_GetTime(0, &RTC_TimeStruct);
      
  timestruct.tm_sec   = RTC_TimeStruct.RTC_Seconds; 
  timestruct.tm_min   = RTC_TimeStruct.RTC_Minutes;
  timestruct.tm_hour  = RTC_TimeStruct.RTC_Hours; // час-4???
  timestruct.tm_mday  = RTC_DataStruct.RTC_Date;  
  timestruct.tm_mon   = RTC_DataStruct.RTC_Month; // месяц-1
  timestruct.tm_year  = RTC_DataStruct.RTC_Year + 2000 - 1900; // год-1900
  timestruct.tm_isdst = 0;
    
  SNTP_Msg.OriginateTimestampL = mktime(&timestruct);   // время запроса клиента к серверу
  SNTP_Msg.OriginateTimestampH = dsec;                  // 64 бита    доли          
  
  SendMessage(SNTP,GET_TIME,&SNTP_Msg,sizeof(SNTP_Msg));
  
}