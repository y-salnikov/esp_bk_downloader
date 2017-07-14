#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

#include <ip_addr.h>

#include <string.h>
#include "www/index.html.c"

#define FLASH __attribute__((section(".irom0.text")))

#define LOCAL_PORT 80

#define startServer_Prio 0
#define startServer_QueueLen 1
#define HTML_CHUNK 256
#define DELAY_US 90
#define OUT0 gpio_output_set(0, BIT13, BIT13, 0)
#define OUT1 gpio_output_set( BIT13, 0, BIT13, 0)
#define DELAY0 ets_delay_us(du)
#define DELAY1 ets_delay_us(du*2)


const uint8 response_ok[] ICACHE_RODATA_ATTR = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
const uint8 response_html[] ICACHE_RODATA_ATTR = "HTTP/1.1 200 OK\r\nServer: ESP8266\r\nContent-Type: text/html\r\n\r\n";
char name[17];
unsigned short adr;
unsigned short length;
unsigned short idx;
uint8 *buf;
uint16_t data_crc;
char sending=0;
unsigned int html_idx,html_to_send;
volatile uint32_t du=DELAY_US;

os_event_t startServer_Queue[startServer_QueueLen];


static void FLASH out_byte(uint8 b)
{
	uint8 i,d;
	d=b;
	for(i=0;i<8;i++)
	{
		OUT0;
		if(d & 0x01)
		{
		  DELAY1;
		  OUT1;
		  DELAY1;
		}
		else
		{
		  DELAY0;
		  OUT1;
		  DELAY0;
		}
		
		d=d>>1;
		OUT0;DELAY0;OUT1;DELAY0;
	}
}


static void FLASH seq(uint16_t l)
{
	uint16_t i;
	for(i=0;i<l;i++)
	{
		OUT0;DELAY0;OUT1;DELAY0;
	}
	OUT0;DELAY1;DELAY1;OUT1;DELAY1;DELAY1;
	OUT0;DELAY1;OUT1;DELAY1;
	OUT0;DELAY0;OUT1;DELAY0;
}


static void FLASH header(void)
{
	uint16_t i;
	seq(8);
	out_byte(adr & 0xff); out_byte(adr>>8);
	out_byte(length & 0xff); out_byte(length >>8);
	for(i=0;i<16;i++) out_byte(name[i]);
}


static void FLASH crc_calc(void)
{
	uint16_t i;
	uint32_t crc;
	uint8 b;
	crc=0;
	for(i=0;i<length;i++)
	{
		b=buf[i];
		crc+=b;
		if(crc>0xFFFF) crc-=0xFFFF;
	}
	data_crc=crc;
}


static void FLASH out_data(void)
{
	uint16_t i;
	for(i=0;i<length;i++) 	out_byte(buf[i]);
	out_byte(data_crc  & 0xff); out_byte(data_crc >>8);
	OUT0;ets_delay_us(du*12);
	OUT1;ets_delay_us(du*12);
}


static void FLASH download(void)
{
	uint16_t i;
//	os_printf("name:\"%s\"\n",name);
//	os_printf("adr:%d\n",adr);
//	os_printf("length:%d\n",length);
	ets_intr_lock();
	crc_calc();
	seq(010000);
	header();
	seq(8);
	out_data();
	seq(512);
	ets_intr_unlock();
	os_free(buf);
}


static char* FLASH get_vars(char *s,unsigned short length)
{
	unsigned short i;
	char *vars=NULL;
	
	for(i=4;i<length;i++)
	{
		if(s[i]=='?') 
		{
		   s[i]=0;
		   vars=&s[i+1];
		}
		if((s[i]==' ') || (s[i]=='\n') || (s[i]=='\r'))
		{
			s[i]=0;
			break;
		}
		
	}
	return vars;
}
static void  FLASH process_header(char *vars)
{
		char *fn_begin,*fn_end, fn_length,i;
		
		fn_begin=strstr(vars,"name=")+8;
		fn_end=strstr(fn_begin,"%22");
		fn_length=fn_end-fn_begin;
		for(i=0;i<16;i++) if(i<fn_length) name[i]=fn_begin[i]; else name[i]=32;
		name[16]=0;
		fn_begin=strstr(vars,"adr=")+4;
		adr=strtol(fn_begin,NULL,10);
		fn_begin=strstr(vars,"length=")+7;
		length=strtol(fn_begin,NULL,10);

		
}



static void  FLASH get_data(char *vars)
{
	char *d_begin,*d_end;
	uint8 b;
	d_end=strstr(vars,"data=")+10;
	do
	{
		d_begin=d_end+1;
		b=strtol(d_begin,&d_end,10);
		if(idx<length) buf[idx++]=b;
	}while(strcmp(d_end,"%5D%22"));
}

static void  FLASH process_data(char *path,char *vars)
{
	if(strcmp(path,"/header")==0)
	{
		length=0;
		process_header(vars);
		if(length)
		{
			idx=0;
			buf=(uint8 *)os_malloc(length);
			if(buf==NULL)
			{
				os_printf("Can't allocate %d bytes of RAM\n",length);
				while(1);
			}
			
		}
	}else
	if(strcmp(path,"/data")==0)
	{
		get_data(vars);
	}else
	if(strcmp(path,"/end")==0)
	{
		download();
	}
	
}




static void FLASH dataRecvCallback(void *arg, char *pusrdata, unsigned short length)
{
    unsigned int i;
    struct espconn *pCon = arg;
    char *path,*vars;
    
    
    vars=get_vars(pusrdata,length);
    path=pusrdata+4;
    
    if(vars==NULL)
    {
    	sending=1;
    	html_idx=0;
    	html_to_send=os_strlen(html);
    	espconn_sent(pCon, response_html, os_strlen(response_html));
    }
    else
    {
    	process_data(path,vars);
    	espconn_sent(pCon, response_ok, os_strlen(response_ok));
    }
//    espconn_disconnect(pCon);
}



static void FLASH dataSentCallback(void *arg)
{
	struct espconn *pCon = arg;
	unsigned int l;
	uint8 *p;
  if(sending==0) espconn_disconnect(pCon);
  else
  {
  	if(html_to_send>HTML_CHUNK)
  	{
  		l=HTML_CHUNK;
  	}
  	else
  	{
  		l=html_to_send;
  		sending=0;
  	}
  	p=html+html_idx;
  	html_idx+=l;
  	html_to_send-=l;
  	espconn_sent(pCon, p, l);
  }
}

static void FLASH  connectionCallback(void *arg)
{
    struct espconn *pCon = arg;
    char string[10] = "OK";

    espconn_regist_recvcb(pCon, dataRecvCallback);
    espconn_regist_sentcb(pCon, dataSentCallback);
//    espconn_sent(pCon, string, os_strlen(string));
//    espconn_disconnect(pCon);
}

static void  FLASH startServer(os_event_t *events){
    struct espconn *pCon = NULL;
    pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
    ets_memset(pCon, 0, sizeof(struct espconn));
    espconn_create(pCon);
    pCon->type = ESPCONN_TCP;
    pCon->state = ESPCONN_NONE;
    pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pCon->proto.tcp->local_port = LOCAL_PORT;
    pCon->proto.tcp->remote_port = LOCAL_PORT;
    espconn_regist_connectcb(pCon, connectionCallback);
    // Start listening
    espconn_accept(pCon);
    // After 15 seconds close TCP connection
    espconn_regist_time(pCon, 9999, 0);
}


void FLASH  user_init()
{
	const char ssid[32] = "<your wifi sid>";
	const char password[32] = "<your password>";

	struct station_config stationConf;
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	os_printf("\n");
	wifi_set_opmode( STATION_MODE );
	os_memcpy(&stationConf.ssid, ssid, 32);
	os_memcpy(&stationConf.password, password, 32);
	wifi_station_set_config(&stationConf);
	wifi_station_connect();
	

  // init gpio sussytem
  gpio_init();
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
  ets_wdt_disable();
  
  system_os_task(startServer, startServer_Prio, startServer_Queue, startServer_QueueLen);
  system_os_post(startServer_Prio, 0, 0 );
}
