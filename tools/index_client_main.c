#include <sys/socket.h>    
#include <string.h>    
#include <errno.h>    
#include <stdlib.h>    
#include <unistd.h>    
#include <netdb.h>  
  
#include <event.h>    
#include <event2/event.h>    
  
//     
void error_quit(const char *str)    
{    
    perror(str);    
    exit(1);     
}    
  
// call back when connection state changed    
void eventcb(struct bufferevent *bev, short events, void *ptr)    
{    
    struct event_base *tbase = (struct event_base*)ptr;    

    if ( !(events & BEV_EVENT_CONNECTED) )     
    {    
        bufferevent_free(bev);    
        event_base_loopbreak(tbase);    
        printf("The connect have been shutdown: %X\n", events);    
    }     
}    
  
// server info comes    
void sockreadcb(struct bufferevent *bev, void *ptr)    
{    
    struct evbuffer *input = bufferevent_get_input(bev);    
    evbuffer_write(input, STDOUT_FILENO);    
}    
  
// keyboard info comes   
void stdreadcb(struct bufferevent *bev, void *ptr)    
{    
    struct bufferevent *sockbev = (struct bufferevent*)ptr;     
    struct evbuffer *input = bufferevent_get_input(bev);    
    bufferevent_write_buffer(sockbev, input);    
}    
  
int main(int argc, char **argv)    
{    
    struct sockaddr_in servaddr;    
    struct event *shellev;    
    int res;    
    struct event_base *base;    
    struct bufferevent *sockbev;    
    struct bufferevent *stdbev;    
  
    if(argc != 3)    
        error_quit("Using: mytelnet <Address> <Port>");    
  
    char address[32] = {0};  
  
    // DNS parse
    struct hostent *host = gethostbyname(argv[1]);  
    memset(&servaddr, 0, sizeof(servaddr));        
    servaddr.sin_family = AF_INET;        
    servaddr.sin_port = htons(atoi(argv[2]));  
  
    res = inet_ntop(host->h_addrtype, host->h_addr, address, sizeof(address));  
    if(res == NULL)    
        error_quit("inet_ntop error");       
    res = inet_pton(AF_INET, address, &servaddr.sin_addr);      
    if(res != 1)    
        error_quit("inet_pton error");       
  
    base = event_base_new();    
  
    // connect to the server and listen   
    sockbev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);    
    res = bufferevent_socket_connect(sockbev,    
        (struct sockaddr *)&servaddr, sizeof(servaddr));    
    if ( res < 0 )     
        error_quit("connect error");    
  
    bufferevent_setcb(sockbev, sockreadcb, NULL, eventcb, (void*)base);    
    bufferevent_enable(sockbev, EV_READ);    
    bufferevent_enable(sockbev, EV_WRITE);    
  
    // listen to keyboard   
    stdbev = bufferevent_socket_new(base, STDIN_FILENO, BEV_OPT_CLOSE_ON_FREE);    
    bufferevent_setcb(stdbev, stdreadcb, NULL, NULL, (void*)sockbev);    
    bufferevent_enable(stdbev, EV_READ);    
    bufferevent_enable(stdbev, EV_WRITE);    
  
    // loop    
    event_base_dispatch(base);    
  
    return 0;    
}  
