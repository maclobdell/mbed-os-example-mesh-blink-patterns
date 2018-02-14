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
static void send_sync(uint8_t sync_num);

static void update_state(uint8_t blink_set);
static void handle_message(char* msg);
static void master_loop_blocking(void);
static void slave_func(void);

#define multicast_addr_str "ff15::810a:64d1"
#define TRACE_GROUP "example"
#define UDP_PORT 1234
#define MESSAGE_WAIT_TIMEOUT (30.0)
#define MASTER_GROUP 0
#define MY_GROUP 1

#define APP_MASTER 1

Thread eventThread;

DigitalOut led_red(LED_RED, 1);
DigitalOut led_green(LED_GREEN, 0);
DigitalOut led_blue(LED_BLUE, 0);

uint8_t led_color;

/* global variabls for button recording */
Timer timer;
uint8_t recording = 0;
uint32_t blinkTiming[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  

uint32_t blinkSets[10] [20] ={                             
                            {100,100,100,100,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},   
                            {200,200,200,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 
                            {300,300,300,300,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  
                            {400,300,200,100,200,300,400,0,0,0,0,0,0,0,0,0,0,0,0,0},   
                            {200,200,400,400,200,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0},   
                            {300,300,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},   
                            {100,200,300,400,100,200,300,400,0,0,0,0,0,0,0,0,0,0,0,0},  
                            {200,200,200,200,200,200,200,200,0,0,0,0,0,0,0,0,0,0,0,0},
                            {300,300,300,300,300,300,300,0,0,0,0,0,0,0,0,0,0,0,0,0},
                            {200,300,200,300,200,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
                            };
uint8_t blinkSets_color[10] = {2,3,1,2,3,2,3,1,2,3};
uint8_t sync_signal = 0xF;

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


void start_mesh_led_blink_pattern_example(NetworkInterface * interface)
{

  printf("start_mesh_blink_pattern_example\r\n");
  
  network_if = interface;
  stoip6(multicast_addr_str, strlen(multicast_addr_str), multi_cast_addr);
  init_socket();

#if APP_MASTER
    master_loop_blocking(); 
#endif    
      
}

void master_loop_blocking(void)
{
  while(1)
  {
    
    for(uint8_t m = 0; m < 10; m++)
    {  //for each blink pattern 

      printf("updating state %d\r\n",m);
      update_state(m);       
           
      //go through cycle 3 times
      for(uint8_t k = 0; k < 3; k++)
      {
        printf("sending sync\r\n");
        queue.call(send_sync,m);
        
        Thread::wait(80);  //wait approx time for sync to reach other devices
                    
          //for each value in pattern
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

void slave_func(void)
{
    //for each value in pattern
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


static void send_sync(uint8_t sync_num) {
    
    //tr_debug("send msg");
    printf("send sync\r\n");
        
    char buf[200];
    int length;

    /**
    * Multicast control message is a NUL terminated string of semicolon separated
    * <field identifier>:<value> pairs.
    *
    * Light control message format:
    * sync:1,2,3,4,5,6,7,7,9
    */
    
    for(uint8_t j = 0;j < 200; j++)
    {      
      buf[j] = 0;   //clear memory
    }
    
    length = snprintf(buf, sizeof(buf), "t:lights;g:%03d;sync:%01d", MY_GROUP, sync_num) + 1;
    
    MBED_ASSERT(length > 0);
    //tr_debug("Sending lightcontrol message, %d bytes: %s", length, buf);
    printf("Sending msg, %d bytes: %s\r\n", length, buf);
    
    SocketAddress send_sockAddr(multi_cast_addr, NSAPI_IPv6, UDP_PORT);
    my_socket->sendto(send_sockAddr, buf, 200);
    //After message is sent, it is received from the network
    
}



static void update_state(uint8_t blink_set) {
  
  printf("updating state\r\n");
  recording = 1;
  set_led_color(blinkSets_color[blink_set]);
   //initialize pattern      
  for(uint8_t i = 0;i < 20; i++)
  {
      blinkTiming[i] = blinkSets[blink_set][i];
  }
  recording = 0;
    
}

static void handle_message(char* msg) {
    // Check if this is lights message
  
#if APP_MASTER == 0
  
    uint16_t group=0xffff;
    uint8_t blink_set = 0;    
    uint8_t stuff_received = 0;

  //  printf("received msg %s\r\n",msg);
        
    if (strstr(msg, "t:lights;") == NULL) {
       return;
    }

    // 0==master, 1==default group
    char *msg_ptr = strstr(msg, "g:");
    if (msg_ptr) {
        char *ptr;
        group = strtol(msg_ptr, &ptr, 10);
    }

    if (strstr(msg, "sync:0") != NULL) {
        sync_signal = 1;
        blink_set = 0;
        stuff_received = 1;
    }
    else if (strstr(msg, "sync:1") != NULL) {
        sync_signal = 1;
        blink_set = 1;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:2") != NULL) {
        sync_signal = 1;
        blink_set = 2;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:3") != NULL) {
        sync_signal = 1;
        blink_set = 3;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:4") != NULL) {
        sync_signal = 1;
        blink_set = 4;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:5") != NULL) {
        sync_signal = 1;
        blink_set = 5;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:6") != NULL) {
        sync_signal = 1;
        blink_set = 6;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:7") != NULL) {
        sync_signal = 1;
        blink_set = 7;     
        stuff_received = 1;   
    }    
    else if (strstr(msg, "sync:8") != NULL) {
        sync_signal = 1;
        blink_set = 8;     
        stuff_received = 1;   
    }
    else if (strstr(msg, "sync:9") != NULL) {
        sync_signal = 1;
        blink_set = 9;     
        stuff_received = 1;   
    }
    
    
    // in this example we only use one group
    if (group==MASTER_GROUP || group==MY_GROUP) {
      
        if(stuff_received == 1)
        {
          printf("received sync %d",blink_set);
          stuff_received = 0;
          update_state(blink_set);
          queue.cancel(slave_handle);
          slave_handle = queue.call(slave_func);
          
        }
    }
#endif    
    
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
            //int timeout_value = MESSAGE_WAIT_TIMEOUT;
            //printf("Packet from %s\r\n", source_addr.get_ip_address());
            //timeout_value += rand() % 30;
            //printf("Advertisiment after %d seconds\r\n", timeout_value);
            //queue.cancel(queue_handle);
            //queue_handle = queue.call_in((timeout_value * 1000), send_message);
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
