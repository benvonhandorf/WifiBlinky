#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include <stdio.h>
#include "driver/uart.h"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

static const int pin = 0;

LOCAL struct espconn conn1;
LOCAL esp_udp udp1;

LOCAL void recvCB(void *arg, char *pData, unsigned short len);
LOCAL void eventCB(System_Event_t *event);
LOCAL void setupUDP();
LOCAL void initDone();

static const int clock = 13;
static const int data = 12;
LOCAL void shift(char value) {
	GPIO_OUTPUT_SET(clock, 0); 
	GPIO_OUTPUT_SET(data, value > 0 ? 1 : 0);
	GPIO_OUTPUT_SET(clock, 1); 
}

LOCAL void shiftByte(char byte) {
	int i;
	for(i = 7 ; i >= 0; i--) {
		char bitValue = byte & (0x01 << i);
		shift(bitValue);
	}
}

LOCAL void startTransmission() {
	shiftByte(0x00);
	shiftByte(0x00);
	shiftByte(0x00);
	shiftByte(0x00);
}

LOCAL void endTransmission() {
	shiftByte(0xFF);
	shiftByte(0xFF);
	shiftByte(0xFF);
	shiftByte(0xFF);
}

LOCAL void shiftColor(char brightness, char red, char green, char blue) {
	brightness = brightness | 0xE0;
	shiftByte(brightness);
	shiftByte(blue);
	shiftByte(green);
	shiftByte(red);
}

short r = 128;
short g = 128;
short b = 128;

static const int random_factor = 15;
static const int half_random_factor = 7;

LOCAL void recvCB(void *arg, char *pData, unsigned short len) {
	struct espconn *pEspConn = (struct espconn *) arg;
	os_printf("Received Data - length %d\n", len);

	if(len != 1) {
		return;
	}

	switch(pData[0]) {
		case '0':
			GPIO_OUTPUT_SET(pin, 0); 
			break;
		case '1':
			GPIO_OUTPUT_SET(pin, 1); 
			break;
		case 'r': {
			char random[] = { 0x00, 0x00, 0x00, 0x00 };
			os_get_random(random, 4);
			r = random[0];
			g = random[1];
			b = random[2];
			break;
		}
	}
}

LOCAL void initDone() {
	os_printf("initDone: Configuring station mode\n");
	wifi_set_opmode_current(STATION_MODE);
	struct station_config stationConfig;
	strncpy(stationConfig.ssid, "OBLIVION", 32);
	strncpy(stationConfig.password, "t4unjath0mson", 64);
	wifi_station_set_config_current(&stationConfig);
	wifi_station_connect();
	GPIO_OUTPUT_SET(pin, 0); 
}

LOCAL void setupUDP() {
	conn1.type = ESPCONN_UDP;
	conn1.state = ESPCONN_NONE;
	udp1.local_port = 25867;
	conn1.proto.udp = &udp1;
	espconn_create(&conn1);
	espconn_regist_recvcb(&conn1, recvCB);
	os_printf("Listening for data\n");
}

LOCAL void eventCB(System_Event_t *event) {
	switch(event->event) {
		case EVENT_STAMODE_GOT_IP:
		os_printf("IP: %d.%d.%d.%d", IP2STR(&event->event_info.got_ip.ip));
		GPIO_OUTPUT_SET(pin, 1); 
		setupUDP();
		break;
		default:
		os_printf("Unknown event: %d\n", event->event);
		break;
	}
}



void shiftColorString() {
	startTransmission();

	short localR = r;
	short localG = g;
	short localB = b;

	int i ;
	for(i = 0; i < 10 ; i++) {
		char random[] = { 0x00, 0x00, 0x00, 0x00 };

		os_get_random(random, 4);
		switch(random[0] % 3) {
			case 0:
				localR = MAX(MIN(localR + (random[0] % random_factor) - half_random_factor, 255), 0);
				break ;
			case 1:
				localG = MAX(MIN(localG + (random[1] % random_factor) - half_random_factor, 255), 0);
				break;
			case 2:
				localB = MAX(MIN(localB + (random[2] % random_factor) - half_random_factor, 255), 0);
				break;
		}
		shiftColor(0x04, localR, localG, localB);
	}

	endTransmission();
}

void alterColors(void *arg)
{
	os_printf("alterColors %x %x %x\n", r, g, b);

	char random[] = { 0x00, 0x00, 0x00, 0x00 };
	os_get_random(random, 4);
	switch(random[0] % 3) {
		case 0:
			r = MAX(MIN(r + (random[0] % random_factor) - half_random_factor, 255), 0);
			break ;
		case 1:
			g = MAX(MIN(g + (random[1] % random_factor) - half_random_factor, 255), 0);
			break;
		case 2:
			b = MAX(MIN(b + (random[2] % random_factor) - half_random_factor, 255), 0);
			break;
	}

	shiftColorString();
}


void user_rf_pre_init(void) {

}

static volatile os_timer_t colorShiftTimer;

void user_init(void) {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, 0); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, 0); 
	uart_init(BIT_RATE_74880, BIT_RATE_74880);

	os_printf("Activating GPIO13\n");

	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); \
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO12);

	GPIO_OUTPUT_SET(pin, 1); 

	shiftColorString();

	system_init_done_cb(initDone);
	wifi_set_event_handler_cb(eventCB);

	os_timer_setfn(&colorShiftTimer, (os_timer_func_t *)alterColors, NULL);
  	os_timer_arm(&colorShiftTimer, 100, 1);
}