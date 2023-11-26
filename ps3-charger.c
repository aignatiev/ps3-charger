/* ATMEGA328PB code to emulate enough of a USB host to get a PS3
   controller to charge.

   Copyright (c) 2010 Jim Paris <jim@jtan.com>
	Copyright (c) 2023 Alexandre Ignatiev <alexandre.ignatiev@gmail.com>
   BSD license
*/

#define F_CPU	12000000UL

#include <avr/io.h>
#include <util/delay.h>

#define MASK_DP0	(1 << 7)
#define MASK_DP1	(1 << 2)
#define MASK_DP2	(1 << 3)
#define MASK_DP3	(1 << 6)

#define MASK_DM0	(1 << 1)
#define MASK_DM1	(1 << 4)
#define MASK_DM2	(1 << 5)
#define MASK_DM3	(1 << 0)

#define MASK_D0	(MASK_DP0 | MASK_DM0)
#define MASK_D1	(MASK_DP1 | MASK_DM1)
#define MASK_D2	(MASK_DP2 | MASK_DM2)
#define MASK_D3	(MASK_DP3 | MASK_DM3)

#define MASK_DP (MASK_DP0 | MASK_DP1 | MASK_DP2 | MASK_DP3)
#define MASK_DM (MASK_DM0 | MASK_DM1 | MASK_DM2 | MASK_DM3)
#define MASK_BOTH (MASK_DP | MASK_DM)
#define MASK_NONE 0
#define MASK_OUT MASK_BOTH
#define MASK_IN MASK_NONE
#define FS_J MASK_DP
#define FS_K MASK_DM
#define SE0 0

#define J "out %[_portb], %[_j]\n"
#define K "out %[_portb], %[_k]\n"
#define X "out %[_portb], %[_x]\n"
#define IN "out %[_ddrb], %[_in]\n"
#define OUT "out %[_ddrb], %[_out]\n"
#define D "nop\n" // delay 1 bit time

#define asmblock(str) asm volatile (str \
	: \
	: [_portb] "I" (_SFR_IO_ADDR(PORTB)), \
	  [_ddrb] "I" (_SFR_IO_ADDR(DDRB)), \
	  [_j] "r" (FS_J), \
	  [_k] "r" (FS_K), \
	  [_x] "r" (SE0), \
	  [_in] "r" (MASK_IN), \
	  [_out] "r" (MASK_OUT) \
	: "memory")

/* Send a bus reset */
static void send_reset(void)
{
	/* SE0 for >= 10 ms */
	PORTB = SE0;
	DDRB = MASK_OUT;
	_delay_ms(10);
	DDRB = MASK_IN;
}

/* Wait for next frame and send SOF.
   Returns 0 if the device is not present. */
static int send_SOF(int count)
{
	uint8_t status = (PINB & MASK_BOTH);

	while (count--) {
		_delay_ms(1);
		if ((PINB & MASK_BOTH) != status)
			return 0;
		asmblock(
			OUT
			// send SOF 1337
			K J K J K J K K K J J K J J K K K J K K K K J K K J J J J J J K X X J
			IN
			X
		);
	}
	return 1;
}

/* Send SET_ADDRESS */
static void send_SET_ADDRESS(void)
{
	asmblock(
		OUT
		// send SETUP
		K J K J K J K K K J J J K K J K J K J K J K J K J K J K K J K J X X J
		// delay a little
		D D D D D D D D
		// send DATA0
		K J K J K J K K K K J K J K K K J K J K J K J K K J J K J K J K K J K
		J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J
		K J K J K J K J K J J J K K J J J J J K K J K K J K X X J
		IN
		X
		// device will send ACK now (within 16 bit times)
	);
	// wait until new frame; an immediate IN will just get NAKed
	send_SOF(1);
	asmblock(
		OUT
		// send IN
		K J K J K J K K K J K K J J J K J K J K J K J K J K J K K J K J X X J
		// device will send DATA1 now.
		IN
		// delay for the expected 35 bits of data
		X D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D
		// wait 12 more bit times (16 is max turnaround)
		D D D D D D D D D D D D
		OUT
		// send ACK
		K J K J K J K K J J K J J K K K X X J
		IN
		X
	);
}

/* Send SET_CONFIGURATION */
static void send_SET_CONFIGURATION(void)
{
	asmblock(
		OUT
		// send SETUP
		K J K J K J K K K J J J K K J K K J K J K J K J K J K K J J J J X X J
		// delay a little
		D D D D D D D D
		// send DATA0
		K J K J K J K K K K J K J K K K J K J K J K J K K J K K J K J K K J K
		J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J K J
		K J K J K J K J K J J J J K J J K J J K K J K K J K X X J
		IN
		X
		// device will send ACK now (within 16 bit times)
	);
	// wait until new frame; an immediate IN will just get NAKed
	send_SOF(1);
	asmblock(
		OUT
		// send IN
		K J K J K J K K K J K K J J J K K J K J K J K J K J K K J J J J X X J
		// device will send DATA1 now.
		IN
		// delay for the expected 35 bits of data
		X D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D D
		// wait 12 more bit times (16 is max turnaround)
		D D D D D D D D D D D D
		OUT
		// send ACK
		K J K J K J K K J J K J J K K K X X J
		IN
		X
	);
}

int main(void) {
	// Clear prescaler (no need to adjust the fuses)
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;

	// Calibrate internal RC osc to 12MHz
	OSCCAL = 0xcd;  // 12.02 MHz measured

	// Tristate all D+ and D-
	DDRB = MASK_IN;
	PORTB = SE0;

	DDRD = 1;	// Status LED output

	_delay_ms(100);

	for (;;) {
		// if the device doesn't appear present, do nothing
		while ((PINB & MASK_D0) != MASK_DP0 && (PINB & MASK_D1) != MASK_DP1 && (PINB & MASK_D2) != MASK_DP2 && (PINB & MASK_D3) != MASK_DP3)
			_delay_ms(1);

		// perform reset
		_delay_ms(100);
		send_reset();

		PIND = 1;	// Toggle status LED

		// perform basic enumeration
		send_SOF(10);
		send_SET_ADDRESS();
		send_SOF(3);
		send_SET_CONFIGURATION();

		// now send empty frames as long as the device is present.
		while (send_SOF(1))
			continue;
	}
}
