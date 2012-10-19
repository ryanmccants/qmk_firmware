/*
Copyright 2012 Jun WAKO <wakojun@gmail.com>

This software is licensed with a Modified BSD License.
All of this is supposed to be Free Software, Open Source, DFSG-free,
GPL-compatible, and OK to use in both free and proprietary applications.
Additions and corrections to this file are welcome.


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

* Neither the name of the copyright holders nor the names of
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "serial.h"

/*
 *  Stupid Inefficient Busy-wait Software Serial
 *  is still useful for negative logic signal like Sun protocol not supported by hardware USART.
 */

#define WAIT_US     (1000000/SERIAL_BAUD)

void serial_init(void)
{
    SERIAL_RXD_INIT();
}

// RX ring buffer
#define RBUF_SIZE   8
static uint8_t rbuf[RBUF_SIZE];
static uint8_t rbuf_head = 0;
static uint8_t rbuf_tail = 0;

uint8_t serial_recv(void)
{
    uint8_t data = 0;
    if (rbuf_head == rbuf_tail) {
        return 0;
    }

    data = rbuf[rbuf_tail];
    rbuf_tail = (rbuf_tail + 1) % RBUF_SIZE;
    return data;
}

//ISR(INT2_vect)
ISR(SERIAL_RXD_VECT)
{
    SERIAL_RXD_INT_ENTER()

    uint8_t data = 0;
#ifdef SERIAL_BIT_ORDER_MSB
    uint8_t pos = 0x80;
#else
    uint8_t pos = 0x01;
#endif
    // to center of start bit
    _delay_us(WAIT_US/2);
    do {
        // to center of next bit
        _delay_us(WAIT_US);

        if (SERIAL_RXD_PIN&(1<<SERIAL_RXD_BIT)) {
            data |= pos;
        }
#ifdef SERIAL_BIT_ORDER_MSB
        pos >>= 1;
#else
        pos <<= 1;
#endif
    } while (pos);
    // to center of stop bit
    _delay_us(WAIT_US);

#ifdef SERIAL_NEGATIVE_LOGIC
    data = ~data;
#endif

    uint8_t next = (rbuf_head + 1) % RBUF_SIZE;
    if (next != rbuf_tail) {
        rbuf[rbuf_head] = data;
        rbuf_head = next;
    }

    SERIAL_RXD_INT_EXIT();
}