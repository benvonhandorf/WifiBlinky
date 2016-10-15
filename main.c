#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include "driver/uart.h"

#define LED_GPIO 13

LOCAL struct espconn conn1;
LOCAL esp_udp udp1;

LOCAL void recvCB(void *arg, char *pData, unsigned short len);
LOCAL void eventCB(System_Event_t *event);
LOCAL void setupUDP();
LOCAL void initDone();

LOCAL void recvCB(void *arg, char *pData, unsigned short len) {
	struct espconn *pEspConn = (struct espconn *) arg;
	os_printf("Received Data - length %d\n", len);

	if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & (1 << LED_GPIO)) {
		// set gpio low
		gpio_output_set(0, (1 << LED_GPIO), 0, 0); 
	} else {
		// set gpio high
		gpio_output_set((1 << LED_GPIO), 0, 0, 0);
	}
}

LOCAL void initDone() {
	wifi_set_opmode_current(STATION_MODE);
	struct station_config stationConfig;
	strncpy(stationConfig.ssid, "OBLIVION", 32);
	strncpy(stationConfig.password, "t4unjath0mson", 64);
	wifi_station_set_config_current(&stationConfig);
	wifi_station_connect();
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
		setupUDP();
		break;
	}
}

void user_rf_pre_init(void) {

}

void user_init(void) {
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); 

	system_init_done_cb(initDone);
	wifi_set_event_handler_cb(eventCB);
}