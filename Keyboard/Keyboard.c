/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  modifications Copyright 2013  Chad Page, under same license as original

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
 *  Main source file for the Keyboard demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Keyboard.h"

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,
				.ReportINEndpoint             =
					{
						.Address              = KEYBOARD_EPADDR,
						.Size                 = KEYBOARD_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
    };


void CheckKeyboard(void);

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;)
	{
		CheckKeyboard();
		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	// Joystick_Init();
//	LEDs_Init();
//	Buttons_Init();
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



/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
//	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

unsigned char pb = 0;
unsigned char pc = 0;
unsigned char prev[80];

char hidmap[80] = {
	41, 30, 31, 32, 33, 35, 34, 36, 37, 38,
	43, 20, 26, 8 , 21, 28, 23, 24, 12, 18,
	4 , 7 , 22, 11, 9 , 10, 13, 14, 51, 15, 
	29, 27, 6 , 25, 5 , 17, 16, 54, 55, 56,
	56, 81, 30, 31, 32, 33, 49, 46, 39, 45,
	48, 82, 34, 35, 36, 37, 52, 19, 47, 48,
	85, 80, 38, 39, 99, 87, 40, 82, 44, 52,
	41, 79, 48, 86, 88, 54, 42, 81, 80, 79 
};
#if 0
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
#endif

/* 
 * Keyboard checking works by pulling up all pins to 5V, and then grounding out one row at a time.
 *
 * If a key is down, the read value will be 0.  This isn't what I expected to do in the beginning,
 * but it's far more electrically stable.  (idea taken from c64key/c64usb)
 */

void CheckKeyboard(void)
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

	// add settling time for each scan
	_delay_ms(.1);

	cn = (pb * 10) + pc;

	if (pc < 8) {
		pf = PINF;
		p = pf & (1 << pc);	
	} else {
		pf = PINC;
		p = pf & (1 << (pc - 8));	
	}

	prev[cn] = p;
	if (!p) _delay_ms(.1);
}


/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
	uint8_t UsedKeyCodes = 0;
	static uint8_t prevc = 0x10;
	uint8_t i, pc;
	
	pc = PINC;	

	// send caps lock if status of caps lock button changes
	if ((pc & 0x10) != (prevc & 0x10)) {
  		KeyboardReport->KeyCode[UsedKeyCodes++] = 57;
	}
	prevc = pc;

	// odd use of reset key:  f2, f3, f4, f5
	// the reset key is the only one that doesn't map into a 'regular' key, and there are no f-keys, so...
	if (!(pc & 0x40)) {
		if ((pc & 0x04) && (pc & 0x08)) { 
 			KeyboardReport->KeyCode[UsedKeyCodes++] = 62; // F5
		} else if (pc & 0x04) { 
	 		KeyboardReport->KeyCode[UsedKeyCodes++] = 60; // F3
		} else if (pc & 0x08) { 
	 		KeyboardReport->KeyCode[UsedKeyCodes++] = 61; // F4
		} else { 
	 		KeyboardReport->KeyCode[UsedKeyCodes++] = 59; // F2
		}
	}
	
	for (i = 0; (i < 80) && (UsedKeyCodes < 4); i++) {
		if (!prev[i]) {
	  		KeyboardReport->KeyCode[UsedKeyCodes++] = hidmap[i];
		}
	}	

/* modifier keys, port D
	0x04 (1 on press) - right apple
	0x08 " - left apple
	others 0 on press:
	0x80 - shift
	0x40 - reset (only when ctrl pressed)
	0x20 - ctrl
	0x10 - capslock	
*/

	KeyboardReport->Modifier = 0;
	if (!(pc & 0x80)) {
		KeyboardReport->Modifier |= HID_KEYBOARD_MODIFIER_LEFTSHIFT;
	}
	if (!(pc & 0x20)) {
		KeyboardReport->Modifier |= HID_KEYBOARD_MODIFIER_LEFTCTRL;
	}
	
	KeyboardReport->Modifier |= ((pc & 0x04) ? HID_KEYBOARD_MODIFIER_LEFTALT : 0);
	KeyboardReport->Modifier |= ((pc & 0x08) ? HID_KEYBOARD_MODIFIER_RIGHTALT : 0);
	
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
}


