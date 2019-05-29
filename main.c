/*
 * h9kx3_band_decoder
 *
 * Created by SQ8KFH on 2019-05-25.
 *
 * Copyright (C) 2019 Kamil Palkowski. All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "can/can.h"

#define STATUS_LED PC6
#define STATUS_LED_DDR DDRC
#define STATUS_LED_PORT PORTC

#define INH_LED PB3
#define INH_LED_DDR DDRB
#define INH_LED_PORT PORTB

#define KEY_LED PB4
#define KEY_LED_DDR DDRB
#define KEY_LED_PORT PORTB

#define INHIBITS PC7
#define INHIBITS_DDR DDRC
#define INHIBITS_PIN PINC
#define INHIBITS_PORT PORTC

#define INHIBITS_IN PC0
#define INHIBITS_IN_DDR DDRC
#define INHIBITS_IN_PIN PINC
#define INHIBITS_IN_PORT PORTC

#define KEYLINE PB5
#define KEYLINE_DDR DDRB
#define KEYLINE_PIN PINB
#define KEYLINE_PORT PORTB

#define PTT_OUT PB6
#define PTT_OUT_DDR DDRB
#define PTT_OUT_PIN PINB
#define PTT_OUT_PORT PORTB

#define KX3_POWER_ON PB7
#define KX3_POWER_ON_DDR DDRB
#define KX3_POWER_ON_PIN PINB
#define KX3_POWER_ON_PORT PORTB

#define KX3_RX PD4
#define KX3_RX_DDR DDRD
#define KX3_RX_PIN PIND
#define KX3_RX_PORT PORTD

#define BAND_0 PD5
#define BAND_0_DDR DDRD
#define BAND_0_PIN PIND
#define BAND_0_PORT PORTD
#define BAND_1 PD6
#define BAND_1_DDR DDRD
#define BAND_1_PIN PIND
#define BAND_1_PORT PORTD
#define BAND_2 PD7
#define BAND_2_DDR DDRD
#define BAND_2_PIN PIND
#define BAND_2_PORT PORTD
#define BAND_3 PB2
#define BAND_3_DDR DDRB
#define BAND_3_PIN PINB
#define BAND_3_PORT PORTB

volatile uint8_t uart_idx = 0;
volatile char uart_buf[40];

volatile uint32_t freq = 0;

ISR(LIN_TC_vect) {
//void kx3_rs() {
//    if (LINSIR & (1<<LRXOK)) {
        char buf = LINDAT;

        if (buf == ';') {
            if (uart_idx >= 37 && uart_buf[uart_idx-37] == 'I' && uart_buf[uart_idx-36] == 'F') {
                uint8_t tmp_id = uart_idx-37;
                uart_buf[tmp_id+13] = '\0';
                freq = atol(&uart_buf[tmp_id+4]);
            }
            else if (uart_idx >= 13 && uart_buf[uart_idx-13] == 'F' && uart_buf[uart_idx-12] == 'A') {
                uint8_t tmp_id = uart_idx-13;
                uart_buf[tmp_id+13] = '\0';
                freq = atol(&uart_buf[tmp_id+4]);
            }
            uart_idx = 0;
        }
        else if (buf == '\0') {
            uart_idx = 0;
        }
        else {
            if (uart_idx >= 40) {
                uart_idx = 0;
            }
            uart_buf[uart_idx] = buf;
            ++uart_idx;
        }
 //   }
}

int main(void) {
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xff;
    DDRE = 0xff;

    STATUS_LED_DDR |= (1<<STATUS_LED);
    INH_LED_DDR |= (1<<INH_LED);
    KEY_LED_DDR |= (1<<KEY_LED);

    INHIBITS_DDR |= (1<<INHIBITS);
    INHIBITS_IN_DDR &= ~(1<<INHIBITS_IN);
    INHIBITS_IN_PORT &= ~(1<<INHIBITS_IN);

    KEYLINE_DDR &= ~(1<<KEYLINE);
    KEYLINE_PORT &= ~(1<<KEYLINE);

    PTT_OUT_DDR |= (1<<PTT_OUT);
    KX3_POWER_ON_DDR |= (1<<KX3_POWER_ON);

    KX3_RX_DDR &= ~(1<<KX3_RX);
    KX3_RX_PORT &= ~(1<<KX3_RX);

    BAND_0_DDR |= (1<<BAND_0);
    BAND_1_DDR |= (1<<BAND_1);
    BAND_2_DDR |= (1<<BAND_2);
    BAND_3_DDR |= (1<<BAND_3);

    CAN_init();
    sei();

    _delay_ms(100);
    CAN_send_turned_on_broadcast();

    uint32_t led_counter = 0x1000;

    LINCR = (1 << LSWRES);
    LINBRRH = 0;
    LINBRRL = 25;
    LINBTR = (1<<LDISR) | (16 << LBT0);
    LINCR = (1<<LENA) | (1<<LCMD2); //UART
    LINCR |= (1<<LCMD1); //only RX, LCONF = 00, 8bit,no-parity,1-stop
    LINENIR = (1<<LENRXOK);

    //KX3_POWER_ON_PORT |= (1<<KX3_POWER_ON);

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

        //kx3_rs();
        if (freq) {
            uint32_t local_freq = freq;
            freq = 0;

            h9msg_t cm;
            CAN_init_new_msg(&cm);

            cm.type = H9MSG_TYPE_REG_INTERNALLY_CHANGED;
            cm.destination_id = H9MSG_BROADCAST_ID;
            cm.dlc = 5;
            cm.data[0]= 10;
            cm.data[1] = local_freq>>24;
            cm.data[2] = local_freq>>16;
            cm.data[3] = local_freq>>8;
            cm.data[4] = local_freq>>0;
            CAN_put_msg(&cm);

            BAND_0_PORT &= ~(1<<BAND_0);
            BAND_1_PORT &= ~(1<<BAND_1);
            BAND_2_PORT &= ~(1<<BAND_2);
            BAND_3_PORT &= ~(1<<BAND_3);

            uint8_t band = 0;

            if (1810000 <= local_freq && local_freq <= 2000000) {
                band = 3;
            }
            else if (3500000 <= local_freq && local_freq <= 3800000) {
                band = 4;
            }
            else if (5351500 <= local_freq && local_freq <= 5366500) {
                band = 5;
            }
            else if (7000000 <= local_freq && local_freq <= 7200000) {
                band = 6;
            }
            else if (10100000 <= local_freq && local_freq <= 10150000) {
                band = 7;
            }
            else if (14000000 <= local_freq && local_freq <= 14350000) {
                band = 8;
            }
            else if (18068000 <= local_freq && local_freq <= 18168000) {
                band = 9;
            }
            else if (21000000 <= local_freq && local_freq <= 21450000) {
                band = 10;
            }
            else if (24890000 <= local_freq && local_freq <= 24990000) {
                band = 11;
            }
            else if (28000000 <= local_freq && local_freq <= 29700000) {
                band = 12;
            }
            else if (50000000 <= local_freq && local_freq <= 52000000) {
                band = 13;
            }
            else if (70000000 <= local_freq && local_freq <= 70500000) {
                band = 14;
            }
            else if (144000000 <= local_freq && local_freq <= 146000000) {
                band = 15;
            }

            if (band & (1<<0)) BAND_0_PORT |= (1<<BAND_0);
            if (band & (1<<1)) BAND_1_PORT |= (1<<BAND_1);
            if (band & (1<<2)) BAND_2_PORT |= (1<<BAND_2);
            if (band & (1<<3)) BAND_3_PORT |= (1<<BAND_3);
        }

        if (INHIBITS_IN_PIN & (1<<INHIBITS_IN)) {
            INH_LED_PORT |= (1<<INH_LED);
        }
        else {
            INH_LED_PORT &= ~(1<<INH_LED);
        }

        if (KEYLINE_PIN & (1<<KEYLINE)) {
            KEY_LED_PORT |= (1<<KEY_LED);
        }
        else {
            KEY_LED_PORT &= ~(1<<KEY_LED);
        }

        if (LINSIR & (1<<LERR)) {
            h9msg_t cm;
            CAN_init_new_msg(&cm);

            cm.type = H9MSG_TYPE_REG_INTERNALLY_CHANGED;
            cm.destination_id = H9MSG_BROADCAST_ID;
            cm.dlc = 2;
            cm.data[0] = 11;
            cm.data[1] = 1;
            CAN_put_msg(&cm);

            LINSIR |= (1<<LERR);
        }

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
