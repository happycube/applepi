// cbp - this was development test code

/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the VirtualSerial demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "VirtualSerial.h"

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = 0,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs
 */
static FILE USBSerialStream;


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;)
	{
		CheckJoystickMovement();

		/* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
		CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
//	LEDs_Init();
	USB_Init();

	// port B is X0..7
	PORTB = 0x00;
	DDRB = 0x00;
	PORTB = 0xFF;

	// port C is Y8..9, other pins	
	PORTC = 0x00;
	DDRC = 0x00;
	PORTC = 0xFF;

	// port F is Y0..7	
	PORTF = 0x00;
	DDRF = 0x00;
	PORTF = 0xFF;
	
}


char tohex(unsigned char n)
{
	if (n < 10) 
		return '0' + n;
	else 
		return 'A' + n - 10;
}

unsigned char pb = 0;
unsigned char pc = 0;
unsigned char prev[80];

#define DWN -1
#define UP  -2
#define LFT -3
#define RHT -4
#define DEL -5

char hidmap[80] = {
	41, 30, 31, 32, 33, 34, 36, 35, 37, 38,
	43, 20, 26, 8 , 21, 28, 23, 24, 12, 18,
	4, 7, 22, 11, 9, 10, 14, 51, 15, 
	29, 27, 6, 25, 22, 17, 16, 54, 55, 56,
	56, 81, 30, 31, 32, 33, 49, 46, 30, 45,
	48, 82, 34, 35, 36, 37, 52, 19, 47, 48,
	85, 80, 38, 39, 99, 87, 40, 82, 44, 52,
	41, 79, 48, 86, 88, 54, 42, 81, 80, 79 
};

/* modifier keys, port D
	0x04 (1 on press) - right apple
	0x08 " - left apple
	others 0 on press:
	0x80 - shift
	0x40 - reset (only when ctrl pressed)
	0x20 - ctrl
	0x10 - capslock	
*/

char keymap[80] = {
	16 , '1', '2', '3', '4', '6', '5', '7', '8', '9',
	9  , 'q', 'w', 'e', 'r', 'y', 't', 'u', 'i', 'o',
	'a', 'd', 's', 'h', 'f', 'g', 'j', 'k', ';', 'l',
	'z', 'x', 'c', 'v', 's', 'n', 'm', ',', '.', '/',
	'/', DWN, '0', '1', '2', '3', '\\', '+', '0', '-',
	')', UP , '4', '5', '6', '7', '\'', 'p', '[', ']',
	'*', LFT, '8', '9', '.', '+','\n', UP , ' ', '\'',
	16 , RHT, '(', '-','\n', ',', DEL, DWN, LFT, RHT
};

char keymap_s[80] = {
	16 , '!', '@', '#', '$', '^', '%', '&', '*', '(',
	9  , 'Q', 'W', 'E', 'R', 'Y', 'T', 'U', 'I', 'O',
	'A', 'D', 'S', 'H', 'F', 'G', 'J', 'K', ':', 'L',
	'Z', 'X', 'C', 'V', 'S', 'N', 'M', '<', '>', '?',
	'/', DWN, '0', '1', '2', '3', '|', '=', ')', '_',
	')', UP , '4', '5', '6', '7', '~', 'p', '{', '}',
	'*', LFT, '8', '9', '.', '+','\n', UP , ' ', '\"',
	16 , RHT, '(', '-','\n', ',', DEL, DWN, LFT, RHT
};

/** Checks for changes in the position of the board joystick, sending strings to the host upon each change. */
void CheckJoystickMovement(void)
{
	unsigned char p, pf;
	unsigned int cn;

	pc++;
	if (pc >= 10) {
		pb++;
		if (pb >= 8) pb = 0;
		
		pc = 0;
		DDRB = 1 << pb;
		PORTB = ~(1 << pb);
	}

	// need busy-loop for settling time, otherwise [x]0 and [x+1]0 alias
	_delay_ms(.1);

	cn = (pb * 10) + pc;

	if (pc < 8) {
		pf = PINF;
		p = pf & (1 << pc);	
	} else {
		pf = PINC;
		p = pf & (1 << (pc - 8));	
	}

	if (p != prev[cn]) {
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(pb & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(pc & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ' ');
		
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(prev[cn] >> 4));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(prev[cn] & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ' ');
	
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(p >> 4));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(p & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ' ');
		
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(pf >> 4));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(pf & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ' ');

		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(PINC >> 4));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, tohex(PINC & 0x0f));
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ' ');
	
		if (PINC & 0x80) {	
			if (PINC & 0x10) {
				CDC_Device_SendByte(&VirtualSerial_CDC_Interface, keymap[cn]);
			} else {
				CDC_Device_SendByte(&VirtualSerial_CDC_Interface, toupper(keymap[cn]));
			}
		} else {
			CDC_Device_SendByte(&VirtualSerial_CDC_Interface, keymap_s[cn]);
		}

//	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, 'a');
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\r');
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\n');

		prev[cn] = p;
	}
/*
	PORTB = 0x00;	
	PORTC = 0x00;	
	PORTD = 0x00;	
	PORTF = 0x00;	
	_delay_ms(1);
*/
//	CDC_Device_SendString(&VirtualSerial_CDC_Interface, "huh\r\n");
//	fputs(ostr, &USBSerialStream);
/*
	if ((ReportString != NULL) && (ActionSent == false))
	{
		ActionSent = true;

		 Write the string to the virtual COM port via the created character stream 
		fputs(ReportString, &USBSerialStream);

		 Alternatively, without the stream: 
		// CDC_Device_SendString(&VirtualSerial_CDC_Interface, ReportString);
	}
*/
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
//	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
//	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

