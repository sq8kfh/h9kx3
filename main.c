/*
 * Project:
 *
 * Created: 
 * Author : SQ8KFH
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "can/can.h"

#define STATUS_LED PC6
#define STATUS_LED_DDR DDRC
#define STATUS_LED_PIN PINC
#define STATUS_LED_PORT PORTC

int main(void) {
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xff;
    DDRE = 0xff;

    STATUS_LED_DDR |= (1<<STATUS_LED);

    CAN_init();
    sei();

    _delay_ms(100);
    CAN_send_turned_on_broadcast();

    uint32_t led_counter = 0x1000;

    while (1) {
        if (led_counter == 0) {
            STATUS_LED_PORT ^= (1<<STATUS_LED);
            if (STATUS_LED_PORT & (1<<STATUS_LED)) {
                led_counter = 0x1000;
            }
            else {
                led_counter = 0x80000;
            }
        }
        --led_counter;

        h9msg_t cm;
        if (CAN_get_msg(&cm)) {
            STATUS_LED_PORT |= (1<<STATUS_LED);
            led_counter = 0x10000;
            /*if (cm.type == H9MSG_TYPE_GET_REG &&
                     (cm.destination_id == can_node_id || cm.destination_id == H9MSG_BROADCAST_ID)) {
                h9msg_t cm_res;
                CAN_init_response_msg(&cm, &cm_res);
                cm_res.data[0] = cm.data[0];
                switch (cm.data[0]) {
                    case 10:
                        //cm_res.dlc = 2;
                        //cm_res.data[1] = ?;
                        CAN_put_msg(&cm_res);
                        break;
                }
            }
            else if (cm.type == H9MSG_TYPE_SET_REG && cm.destination_id == can_node_id) {
                h9msg_t cm_res;
                CAN_init_response_msg(&cm, &cm_res);
                cm_res.data[0] = cm.data[0];
                switch (cm.data[0]) {
                    case 10:
                        //cm_res.dlc = 2;
                        //cm_res.data[1] = ?;
                        CAN_put_msg(&cm_res);
                        break;
                }
            }*/
        }
    }
}
