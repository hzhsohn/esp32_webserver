/* Standard includes. */
#include "string.h"
#include "esp_err.h"
/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include <netinet/in.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/* Utils includes. */
#include "esp_log.h"
#include "event.h"
#include "http.h"
#include "webserver.h"

#define TAG "webserver:"

static uint32_t http_url_length=0;
static uint32_t http_body_length=0;

static char* http_body=NULL;
static char* http_url=NULL;

static int32_t socket_fd, client_fd;
#define RES_HEAD "HTTP/1.1 200 OK\r\nContent-type: %s\r\nTransfer-Encoding: chunked\r\n\r\n"
static const char* HTML="text/html";
static const char* JSON="application/json";
static char chunk_len[15];


const static char not_find_page[] = "<!DOCTYPE html>"
      "<html>\n"
      "<head>\n"
      "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      "	 <link href=\"https://cdn.bootcss.com/bootstrap/4.0.0-alpha.6/css/bootstrap.min.css\" rel=\"stylesheet\">\n"
      "  <script src=\"https://cdn.bootcss.com/bootstrap/4.0.0-alpha.6/js/bootstrap.min.js\"></script>\n"
      "<title>WhyEngineer-ESP32</title>\n"
      "</head>\n"
      "<body>\n"
      "<h1 class=\"container\">Page not Found!</h1>\n"
      "</body>\n"
      "</html>\n";

static int header_value_callback(http_parser* a, const char *at, size_t length){
	// for(int i=0;i<length;i++)
	// 	putchar(at[i]);
	return 0;
}
static int body_callback(http_parser* a, const char *at, size_t length){
    // static uint32_t cnt=0;
    // printf("cnt:%d\n",cnt++);
    //spiRamFifoWrite(at,length);
    http_body=realloc(http_body,http_body_length+length);
    memcpy(http_body+http_body_length,at,length);
    http_body_length+=length;
  //   for(int i=0;i<length;i++)
		// putchar(at[i]);
    return 0;
}
static int url_callback(http_parser* a, const char *at, size_t length){
    // static uint32_t cnt=0;
    // printf("cnt:%d\n",cnt++);
    //spiRamFifoWrite(at,length);
    http_url=realloc(http_url,http_url_length+length);
    memcpy(http_url+http_url_length,at,length);
    http_url_length+=length;
  //   for(int i=0;i<length;i++)
		// putchar(at[i]);
    return 0;
}
static void chunk_end(int socket){
	write(socket, "\r\n0\r\n\r\n", 7);
}

typedef struct
{
  char* url;
  void(*handle)(http_parser* a,char* body);
}HttpHandleTypeDef;

void web_index(http_parser* a,char* body);
void led_ctrl(http_parser* a,char* body);
static void not_find();
const HttpHandleTypeDef http_handle[]={
	{"/",index},
	{"/api/led/",led_ctrl},
};
static void not_find(){
	char *request;
  	asprintf(&request,RES_HEAD,HTML);//html
  	write(client_fd, request, strlen(request));
  	free(request);
  	sprintf(chunk_len,"%x\r\n",strlen(not_find_page));
  	write(client_fd, chunk_len, strlen(chunk_len));
  	write(client_fd, not_find_page, strlen(not_find_page));
  	chunk_end(client_fd);
}
void web_index(http_parser* a,char* body){
	char *request;
  	asprintf(&request,RES_HEAD,HTML);//html
  	write(client_fd, request, strlen(request));
  	free(request);
  	//chunk
  		
}

void led_ctrl(http_parser* a,char* body){

}

static int body_done_callback (http_parser* a){
	http_body=realloc(http_body,http_body_length+1);
    http_body[http_body_length]='\0';
  	ESP_LOGI(TAG,"body:%s",http_body);
  	for(int i=0;i<sizeof(http_handle)/sizeof(http_handle[0]);i++){
  		if(strcmp(http_handle[i].url,http_url)==0){
  			http_handle[i].handle(a,http_body);
  			return 0;
  		}
  	}
  	not_find();
  	// char *request;
  	// asprintf(&request,RES_HEAD,HTML);
  	// write(client_fd, request, strlen(request));
   //  free(request);
   //  sprintf(chunk_len,"%x\r\n",strlen(not_find_page));
   //  //ESP_LOGI(TAG,"chunk_len:%s,length:%d",chunk_len,strlen(http_index_hml));
   //  write(client_fd, chunk_len, strlen(chunk_len));
   //  write(client_fd, http_index_hml, strlen(http_index_hml));
   //  write(client_fd, "\r\n", 2);
   //  chunk_end(client_fd);
    //close( client_fd );	
    return 0;
}
static int begin_callback (http_parser* a){
  	//ESP_LOGI(TAG,"message begin");
    return 0;
}
static int header_complete_callback(http_parser* a){
	http_url=realloc(http_url,http_url_length+1);
    http_url[http_url_length]='\0';
	ESP_LOGI(TAG,"url:%s",http_url);
    return 0;
}

static http_parser_settings settings =
{   .on_message_begin = begin_callback
    ,.on_header_field = 0
    ,.on_header_value = header_value_callback
    ,.on_url = url_callback
    ,.on_status = 0
    ,.on_body = body_callback
    ,.on_headers_complete = header_complete_callback
    ,.on_message_complete = body_done_callback
    ,.on_chunk_header = 0
    ,.on_chunk_complete = 0
};

/* Dimensions the buffer into which input characters are placed. */

static struct sockaddr_in server, client;


int creat_socket_server(in_port_t in_port, in_addr_t in_addr)
{
	int socket_fd, on;
	//struct timeval timeout = {10,0};

	server.sin_family = AF_INET;
	server.sin_port = in_port;
	server.sin_addr.s_addr = in_addr;

	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0))<0) {
		perror("listen socket uninit\n");
		return -1;
	}
	on=1;
	//setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int) );
	//CALIB_DEBUG("on %x\n", on);
	if((bind(socket_fd, (struct sockaddr *)&server, sizeof(server)))<0) {
		perror("cannot bind srv socket\n");
		return -1;
	}

	if(listen(socket_fd, 1)<0) {
		perror("cannot listen");
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}



void webserver_task( void *pvParameters ){
	int32_t lBytes;
	ESP_LOGI(TAG,"webserver start");
	uint32_t request_cnt=0;
	
	(void) pvParameters;
	http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = NULL;
    socklen_t client_size=sizeof(client);

	socket_fd = creat_socket_server(htons(80),htonl(INADDR_ANY));
	if( socket_fd >= 0 ){
		/* Obtain the address of the output buffer.  Note there is no mutual
		exclusion on this buffer as it is assumed only one command console
		interface will be used at any one time. */
		char recv_buf[64];
		esp_err_t nparsed = 0;
		/* Ensure the input string starts clear. */
		for(;;){
			client_fd=accept(socket_fd,(struct sockaddr*)&client,&client_size);
			if(client_fd>0L){
				// strcat(outbuf,pcWelcomeMessage);
				// strcat(outbuf,path);
				// lwip_send( lClientFd, outbuf, strlen(outbuf), 0 );
				do{
					lBytes = recv( client_fd, recv_buf, sizeof( recv_buf ), 0 );
					if(lBytes==0){
						close( client_fd );	
						break;
					}
						
					 // for(int i=0;i<lBytes;i++)
						// putchar(recv_buf[i]);
					nparsed = http_parser_execute(&parser, &settings, recv_buf, lBytes);
					
					//while(xReturned!=pdFALSE);
					//lwip_send( lClientFd,path,strlen(path), 0 );
				}while(lBytes > 0 && nparsed >= 0);
				free(http_url);
				free(http_body);
				http_url=NULL;
				http_body=NULL;
				http_url_length=0;
				http_body_length=0;		
			}
			ESP_LOGI(TAG,"request_cnt:%d,socket:%d",request_cnt++,client_fd);
			close( client_fd );
		}
	}

}