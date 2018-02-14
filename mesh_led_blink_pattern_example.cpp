/*
 * Copyright (c) 2016 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "nanostack/socket_api.h"
#include "mesh_led_blink_pattern_example.h"
#include "common_functions.h"
#include "ip6string.h"
#include "mbed-trace/mbed_trace.h"

static void init_socket();
static void handle_socket();
static void receive();
static void send_message();
static void update_state(uint32_t* pattern, uint8_t color);
static void handle_message(char* msg);

#define multicast_addr_str "ff15::810a:64d1"
#define TRACE_GROUP "example"
#define UDP_PORT 1234
#define MESSAGE_WAIT_TIMEOUT (30.0)
#define MASTER_GROUP 0
#define MY_GROUP 1

#define APP_MASTER 1 

static void sw_handler(void);
static void sw_color_handler(void);
InterruptIn sw(SW1);
InterruptIn sw_color(SW3);

Thread blinker_thread;
Thread eventThread;

DigitalOut led_red(LED_RED, 1);
DigitalOut led_green(LED_GREEN, 0);
DigitalOut led_blue(LED_BLUE, 0);

uint8_t led_color;

/* global variabls for button recording */
Timer timer;
uint8_t recording = 0;
uint32_t blinkTiming[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


uint8_t blinkCount = 0;  //starts at 0 for first knock
uint32_t startTime;
/*          */

NetworkInterface * network_if;
UDPSocket* my_socket;
// queue for sending messages from button press.
EventQueue queue;
// for LED blinking
Ticker ticker;
// Handle for delayed message send
int queue_handle = 0;

uint8_t multi_cast_addr[16] = {0};
uint8_t receive_buffer[200];
// how many hops the multicast message can go
static const int16_t multicast_hops = 10;
bool button_status = 0;


void sw_handler(void) {

  if(recording ==0)
    {
      //this is the first button press
      blinkCount = 0;
      //led1 = 1; //turn off led
      set_led_off();
      
      for(uint8_t i = 0; i<20; i++)
      {
        blinkTiming[i] = 0;
      }
      recording = 1;
      timer.start();
      timer.reset();
      startTime = timer.read_ms();

    }else if(recording == 1)
    {
      //this is not the first button press
      uint32_t now;
      now = timer.read_ms();
      blinkTiming[blinkCount] = (now - startTime);
      blinkCount++;
      startTime = now;  //start time for next period
    }

}

void sw_color_handler(void)
{
  //change color
  led_color++;
  
   if (led_color > 3)
   {
     led_color = 1;
   }
  
}

void start_mesh_led_blink_pattern_example(NetworkInterface * interface)
{

  printf("start_mesh_blink_pattern_example\r\n");
  
  network_if = interface;
  stoip6(multicast_addr_str, strlen(multicast_addr_str), multi_cast_addr);
  init_socket();

  sw.mode(PullUp);
  sw.fall(sw_handler);
  sw_color.mode(PullUp);
  sw_color.fall(sw_color_handler);
  
    recording = 0;
              
    while(1)
    {
        if(recording==1)
        {
            if (blinkCount >= 20 || timer.read_ms() > 5000)  //max presses reached or 5 sec elapsed - done recording
            {
              timer.stop();
              /* re-initialize all variables */
              recording = 0;
              blinkCount = 0;
              
              #if APP_MASTER
                //send pattern to slaves
                queue.call(send_message);
              #endif
              
            }
        }else if(recording == 0)
        {
            while(recording == 0)
            {
              for(uint8_t i = 0;i < 20; i++)
              {
                //blink led
                //led1 = 0;
                set_led_on();
                Thread::wait(20);
                //led1 = 1;
                set_led_off();                
                Thread::wait(blinkTiming[i]);
                if(blinkTiming[i] == 0){
                  break;
                }
  
              }
              Thread::wait(2000);
            }
  
        }
    
      
    }

}

void set_led_color(uint8_t color)
{
    if(color > 0 && color < 4){
       led_color = color;
    }
}

void set_led_off(void)
{
  led_red = LED_OFF;
  led_green = LED_OFF;
  led_blue = LED_OFF;
}

void set_led_on(void)
{   
   if(led_color == LED_COLOR_RED) {
      led_red = LED_ON;
      led_green = LED_OFF;
      led_blue = LED_OFF;
    }
    else if(led_color == LED_COLOR_GREEN) {
      led_red = LED_OFF;
      led_green = LED_ON;
      led_blue = LED_OFF;
    } 
    else if(led_color == LED_COLOR_BLUE) {
      led_red = LED_OFF;
      led_green = LED_OFF;
      led_blue = LED_ON;
    }           
}

static void send_message() {
    
    //tr_debug("send msg");
    printf("send msg\r\n");
        
    char buf[200];
    char blink_string[100];
    int length;

    /**
    * Multicast control message is a NUL terminated string of semicolon separated
    * <field identifier>:<value> pairs.
    *
    * Light control message format:
    * t:lights;g:<group_id>;c:<1|2|3>;p:a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t\0
    */
    
    for(uint8_t j = 0;j < 200; j++)
    {      
      buf[j] = 0;   //clear memory
    }
    for(uint8_t j = 0;j < 100; j++)
    {
      blink_string[j] = 0; //clear memory
    }        
    //construct the string of blink pattern              
    for(uint8_t j = 0;j < 20; j++)
    {      
      snprintf(blink_string, sizeof(blink_string), "%s%d ", blink_string, blinkTiming[j]);   //concatenate
    }
    printf("blink string %s\r\n", blink_string);
    
    length = snprintf(buf, sizeof(buf), "t:lights;g:%03d;c:%d;p:%s", MY_GROUP, led_color, blink_string) + 1;
    
    MBED_ASSERT(length > 0);
    //tr_debug("Sending lightcontrol message, %d bytes: %s", length, buf);
    printf("Sending lightcontrol message, %d bytes: %s\r\n", length, buf);
    
    SocketAddress send_sockAddr(multi_cast_addr, NSAPI_IPv6, UDP_PORT);
    my_socket->sendto(send_sockAddr, buf, 200);
    //After message is sent, it is received from the network
    
}


static void update_state(uint32_t* pattern, uint8_t color) {
  
  recording = 1;
  printf("storing new pattern\r\n");
  set_led_color(color);
  for(uint8_t j = 0;j < 20; j++)
  {
    blinkTiming[j] = pattern[j];
    tr_debug("%d", blinkTiming[j]);
  }
  printf("...done\r\n");  
  recording = 0;
    
}

static void handle_message(char* msg) {
    // Check if this is lights message
    //uint8_t state=button_status;
    uint16_t group=0xffff;
    uint32_t pattern[100];
    uint8_t color;    
    uint8_t pattern_received = 0;

    printf("received msg %s\r\n",msg);
        
    if (strstr(msg, "t:lights;") == NULL) {
       return;
    }

    if (strstr(msg, "c:1;") != NULL) {
        color = 1;
    }
    else if (strstr(msg, "c:2;") != NULL) {
        color = 2;
    }
    else if (strstr(msg, "c:3;") != NULL) {
        color = 3;
    }
    /* store button pattern here */
    char *pattern_ptr = strstr(msg, "p:");
    if (pattern_ptr) {
       char *pEnd;
        // parse string into pattern array
        //printf("converting pattern to integers\r\n");
        //printf("pattern_ptr = %s",pattern_ptr);

       char *token;    
       const char delimiters[] = " .,;:!-p";

       char *rest = pattern_ptr;

       for(uint8_t j = 0;j < 20; j++)
        {
          token = strtok_r(rest, delimiters, &rest); 
        //  printf("%s\r\n",token);
          pattern[j] = atoi(token);

          /* validate integrity of pattern */
          /* drop anything not a valid number in string */
          if (pattern[j] < 0 || pattern[j] > 5000)
          {
             pattern[j] = 0;  
          } 
        }
                
        printf("received pattern:\n\r");
        pattern_received = 1;
        for(uint8_t j = 0;j < 20; j++)
        { 
          printf("%d\n\r", pattern[j]);
    
        }
    }

    
    // 0==master, 1==default group
    char *msg_ptr = strstr(msg, "g:");
    if (msg_ptr) {
        char *ptr;
        group = strtol(msg_ptr, &ptr, 10);
    }

    // in this example we only use one group
    if (group==MASTER_GROUP || group==MY_GROUP) {
        if(pattern_received == 1)
        {
          update_state(pattern, color);
          pattern_received = 0;
        }
    }
}

static void receive() {
    // Read data from the socket
    SocketAddress source_addr;
    memset(receive_buffer, 0, sizeof(receive_buffer));
    bool something_in_socket=true;
    // read all messages
    while (something_in_socket) {
        int length = my_socket->recvfrom(&source_addr, receive_buffer, sizeof(receive_buffer) - 1);
        if (length > 0) {
            int timeout_value = MESSAGE_WAIT_TIMEOUT;
            printf("Packet from %s\r\n", source_addr.get_ip_address());
            timeout_value += rand() % 30;
            printf("Advertisiment after %d seconds\r\n", timeout_value);
            queue.cancel(queue_handle);
            queue_handle = queue.call_in((timeout_value * 1000), send_message);
            // Handle command
            handle_message((char*)receive_buffer);
        }
        else if (length!=NSAPI_ERROR_WOULD_BLOCK) {
            printf("Error happened when receiving %d\n\r", length);
            something_in_socket=false;
        }
        else {
            // there was nothing to read.
            something_in_socket=false;
        }
    }
}

static void handle_socket() {
    // call-back might come from ISR
    queue.call(receive);
}

static void init_socket()
{
    my_socket = new UDPSocket(network_if);
    my_socket->set_blocking(false);
    my_socket->bind(UDP_PORT);
    my_socket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));

    ns_ipv6_mreq_t mreq;
    memcpy(mreq.ipv6mr_multiaddr, multi_cast_addr, 16);
    mreq.ipv6mr_interface = 0;

    my_socket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_JOIN_GROUP, &mreq, sizeof mreq);

    //let's register the call-back function.
    //If something happens in socket (packets in or out), the call-back is called.
    my_socket->sigio(callback(handle_socket));
    
    /* New - put queue in a thread, so main thread doesn't get blocked here */
     eventThread.start(callback(&queue, &EventQueue::dispatch_forever));
     // dispatch forever
     //queue.dispatch();
  
}
