/* USB Serial Example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_serial.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "usb_common.h"
#include "usb_private.h"



/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/


static const uint8_t PROGMEM endpoint_config_table[] = {
	1, EP_TYPE_INTERRUPT_IN,  EP_SIZE(DEBUG_TX_SIZE) | DEBUG_TX_BUFFER,
	1, EP_TYPE_INTERRUPT_OUT, EP_SIZE(DEBUG_RX_SIZE) | DEBUG_RX_BUFFER,
	1, EP_TYPE_BULK_OUT, EP_SIZE(DISK_RX_SIZE) |  DISK_RX_BUFFER,
	1, EP_TYPE_BULK_IN, EP_SIZE(DISK_TX_SIZE) | DISK_TX_BUFFER,
	1, EP_TYPE_INTERRUPT_IN,  EP_SIZE(KEYBOARD_SIZE) | KEYBOARD_BUFFER,
	0
};


/**************************************************************************
 *
 *  Descriptor Data
 *
 **************************************************************************/

// Descriptors are the data that your computer reads when it auto-detects
// this USB device (called "enumeration" in USB lingo).  The most commonly
// changed items are editable at the top of this file.	Changing things
// in here should only be done by those who've read chapter 9 of the USB
// spec and relevant portions of any USB class specifications!

static uint8_t PROGMEM device_descriptor[] = {
	18,					// bLength
	1,					// bDescriptorType
	0x00, 0x02,				// bcdUSB
	0,					// bDeviceClass
	0,					// bDeviceSubClass
	0,					// bDeviceProtocol
	ENDPOINT0_SIZE,				// bMaxPacketSize0
	LSB(VENDOR_ID), MSB(VENDOR_ID),		// idVendor
	LSB(PRODUCT_ID), MSB(PRODUCT_ID),	// idProduct
	0x00, 0x01,				// bcdDevice
	0,					// iManufacturer
	1,					// iProduct
	2,					// iSerialNumber
	1					// bNumConfigurations
};

// Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60
static uint8_t PROGMEM keyboard_hid_report_desc[] = {
	0x05, 0x01,	     // Usage Page (Generic Desktop),
	0x09, 0x06,	     // Usage (Keyboard),
	0xA1, 0x01,	     // Collection (Application),
	0x75, 0x01,	     //   Report Size (1),
	0x95, 0x08,	     //   Report Count (8),
	0x05, 0x07,	     //   Usage Page (Key Codes),
	0x19, 0xE0,	     //   Usage Minimum (224),
	0x29, 0xE7,	     //   Usage Maximum (231),
	0x15, 0x00,	     //   Logical Minimum (0),
	0x25, 0x01,	     //   Logical Maximum (1),
	0x81, 0x02,	     //   Input (Data, Variable, Absolute), ;Modifier byte
	0x95, 0x01,	     //   Report Count (1),
	0x75, 0x08,	     //   Report Size (8),
	0x81, 0x03,	     //   Input (Constant),		    ;Reserved byte
	0x95, 0x05,	     //   Report Count (5),
	0x75, 0x01,	     //   Report Size (1),
	0x05, 0x08,	     //   Usage Page (LEDs),
	0x19, 0x01,	     //   Usage Minimum (1),
	0x29, 0x05,	     //   Usage Maximum (5),
	0x91, 0x02,	     //   Output (Data, Variable, Absolute), ;LED report
	0x95, 0x01,	     //   Report Count (1),
	0x75, 0x03,	     //   Report Size (3),
	0x91, 0x03,	     //   Output (Constant),		     ;LED report padding
	0x95, 0x06,	     //   Report Count (6),
	0x75, 0x08,	     //   Report Size (8),
	0x15, 0x00,	     //   Logical Minimum (0),
	0x25, 0x68,	     //   Logical Maximum(104),
	0x05, 0x07,	     //   Usage Page (Key Codes),
	0x19, 0x00,	     //   Usage Minimum (0),
	0x29, 0x68,	     //   Usage Maximum (104),
	0x81, 0x00,	     //   Input (Data, Array),
	0xc0		     // End Collection
};

static uint8_t PROGMEM debug_hid_report_desc[] = {
	0x06, 0xC9, 0xFF,			// Usage Page 0xFFC9 (vendor defined)
	0x09, 0x04,				// Usage 0x04
	0xA1, 0x5C,				// Collection 0x5C
	0x75, 0x08,				// report size = 8 bits (global)
	0x15, 0x00,				// logical minimum = 0 (global)
	0x26, 0xFF, 0x00,			// logical maximum = 255 (global)
	0x95, DEBUG_TX_SIZE,			// report count (global)
	0x09, 0x75,				// usage (local)
	0x81, 0x02,				// Input
	0x95, DEBUG_RX_SIZE,			// report count (global)
	0x09, 0x76,				// usage (local)
	0x91, 0x02,				// Output
	0x95, 0x04,				// report count (global)
	0x09, 0x76,				// usage (local)
	0xB1, 0x02,				// Feature
	0xC0					// end collection
};



#define CONFIG1_DESC_SIZE		( 9 + 9+7+7 + 9+9+7 + 9+9+7+7 )
#define KEYBOARD_HID_DESC_OFFSET	( 9 + 9+7+7 + 9 )
#define DEBUG_HID_DESC_OFFSET		( 9 + 9+7+7 + 9+9+7 + 9 )

static uint8_t PROGMEM config1_descriptor[CONFIG1_DESC_SIZE] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9,				// bLength;
	2,				// bDescriptorType;
	LSB(CONFIG1_DESC_SIZE),		// wTotalLength
	MSB(CONFIG1_DESC_SIZE),
	NUM_INTERFACE,			// bNumInterfaces
	1,				// bConfigurationValue
	0,				// iConfiguration
	0xC0,				// bmAttributes
	50,				// bMaxPower

	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,				// bLength
	4,				// bDescriptorType
	DISK_INTERFACE,			// bInterfaceNumber
	0,				// bAlternateSetting
	2,				// bNumEndpoints
	0x08,				// bInterfaceClass (8 = Mass Storage)
	0x06,				// bInterfaceSubClass (6 = SCSI transparent)
	0x50,				// bInterfaceProtocol (0x50 = bulk only transport)
	0,				// iInterface
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,				// bLength
	5,				// bDescriptorType
	DISK_RX_ENDPOINT,		// bEndpointAddress
	0x02,				// bmAttributes (0x02=bulk)
	DISK_RX_SIZE, 0,		  // wMaxPacketSize
	0,				// bInterval
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,				// bLength
	5,				// bDescriptorType
	DISK_TX_ENDPOINT | 0x80,	// bEndpointAddress
	0x02,				// bmAttributes (0x02=bulk)
	DISK_TX_SIZE, 0,		// wMaxPacketSize
	0,				// bInterval

	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,				// bLength
	4,				// bDescriptorType
	KEYBOARD_INTERFACE,		// bInterfaceNumber
	0,				// bAlternateSetting
	1,				// bNumEndpoints
	0x03,				// bInterfaceClass (0x03 = HID)
	0x01,				// bInterfaceSubClass (0x01 = Boot)
	0x01,				// bInterfaceProtocol (0x01 = Keyboard)
	0,				// iInterface
	// HID interface descriptor, HID 1.11 spec, section 6.2.1
	9,				// bLength
	0x21,				// bDescriptorType
	0x11, 0x01,			// bcdHID
	0,				// bCountryCode
	1,				// bNumDescriptors
	0x22,				// bDescriptorType
	sizeof(keyboard_hid_report_desc), // wDescriptorLength
	0,
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,				// bLength
	5,				// bDescriptorType
	KEYBOARD_ENDPOINT | 0x80,	// bEndpointAddress
	0x03,				// bmAttributes (0x03=intr)
	KEYBOARD_SIZE, 0,		// wMaxPacketSize
	KEYBOARD_INTERVAL,		// bInterval

	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,				// bLength
	4,				// bDescriptorType
	DEBUG_INTERFACE,		// bInterfaceNumber
	0,				// bAlternateSetting
	2,				// bNumEndpoints
	0x03,				// bInterfaceClass (0x03 = HID)
	0x00,				// bInterfaceSubClass
	0x00,				// bInterfaceProtocol
	0,				// iInterface
	// HID interface descriptor, HID 1.11 spec, section 6.2.1
	9,				// bLength
	0x21,				// bDescriptorType
	0x11, 0x01,			// bcdHID
	0,				// bCountryCode
	1,				// bNumDescriptors
	0x22,				// bDescriptorType
	sizeof(debug_hid_report_desc),	// wDescriptorLength
	0,
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,				// bLength
	5,				// bDescriptorType
	DEBUG_TX_ENDPOINT | 0x80,	// bEndpointAddress
	0x03,				// bmAttributes (0x03=intr)
	DEBUG_TX_SIZE, 0,		// wMaxPacketSize
	DEBUG_TX_INTERVAL,		// bInterval
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,				// bLength
	5,				// bDescriptorType
	DEBUG_RX_ENDPOINT,		// bEndpointAddress
	0x03,				// bmAttributes (0x03=intr)
	DEBUG_RX_SIZE, 0,		// wMaxPacketSize
	DEBUG_RX_INTERVAL,		// bInterval
};

// If you're desperate for a little extra code memory, these strings
// can be completely removed if iManufacturer, iProduct, iSerialNumber
// in the device desciptor are changed to zeros.
struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	int16_t wString[];
};
static struct usb_string_descriptor_struct PROGMEM string0 = {
	4,
	3,
	{0x0409}
};
static struct usb_string_descriptor_struct PROGMEM string1 = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};
static struct usb_string_descriptor_struct PROGMEM string2 = {
	sizeof(STR_SERIAL_NUMBER),
	3,
	STR_SERIAL_NUMBER
};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
static struct descriptor_list_struct {
	uint16_t	wValue;
	uint16_t	wIndex;
	const uint8_t	*addr;
	uint8_t		length;
} PROGMEM descriptor_list[] = {
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
	{0x0200, 0x0000, config1_descriptor, sizeof(config1_descriptor)},
	{0x2200, KEYBOARD_INTERFACE, keyboard_hid_report_desc, sizeof(keyboard_hid_report_desc)},
	{0x2100, KEYBOARD_INTERFACE, config1_descriptor+KEYBOARD_HID_DESC_OFFSET, 9},
	{0x2200, DEBUG_INTERFACE, debug_hid_report_desc, sizeof(debug_hid_report_desc)},
	{0x2100, DEBUG_INTERFACE, config1_descriptor+DEBUG_HID_DESC_OFFSET, 9},
	{0x0300, 0x0000, (const uint8_t *)&string0, 4},
	{0x0301, 0x0409, (const uint8_t *)&string1, sizeof(STR_PRODUCT)},
	{0x0302, 0x0409, (const uint8_t *)&string2, sizeof(STR_SERIAL_NUMBER)},
};
#define NUM_DESC_LIST (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))

static uint8_t PROGMEM scsi_inquiry_response[] = {
	0x00,	// 0x00 = direct-access device
	0x80,	// 0x80 = removable media
	0x04,	// 0x04 = SPC-2 commands
	0x02,	// response format
	0x20,	// # bytes past this point
	0x00,
	0x00,
	0x00,
	'G','e','n','e','r','i','c',' ',
	'U','S','B',' ','F','l','a','s','h',' ','D','i','s','c',' ',' ',
	'1','.','0','0'
};

/**************************************************************************
 *
 *  Variables - these are the only non-stack RAM usage
 *
 **************************************************************************/

// zero when we are not configured, non-zero when enumerated
volatile uint8_t usb_configuration USBSTATE;
volatile uint8_t usb_suspended USBSTATE;

// the time remaining before we transmit any partially full
// packet, or send a zero length packet.
volatile uint8_t debug_flush_timer USBSTATE;

// which modifier keys are currently pressed
// 1=left ctrl,    2=left shift,   4=left alt,	  8=left gui
// 16=right ctrl, 32=right shift, 64=right alt, 128=right gui
uint8_t keyboard_modifier_keys USBSTATE;

// which keys are currently pressed, up to 6 keys may be down at once
uint8_t keyboard_keys[6] USBSTATE;

// protocol setting from the host.  We use exactly the same report
// either way, so this variable only stores the setting since we
// are required to be able to report which setting is in use.
static uint8_t keyboard_protocol USBSTATE;

// the idle configuration, how often we send the report to the
// host (ms * 4) even when it hasn't changed
static uint8_t keyboard_idle_config USBSTATE;

// count until idle timeout
uint8_t keyboard_idle_count USBSTATE;

// 1=num lock, 2=caps lock, 4=scroll lock, 8=compose, 16=kana
volatile uint8_t keyboard_leds USBSTATE;


static volatile uint8_t ms_state USBSTATE;
static volatile uint8_t ms_rdonly USBSTATE;



/**************************************************************************
 *
 *  Public Functions - these are the API intended for the user
 *
 **************************************************************************/



// initialize USB serial
void usb_init(void)
{
	uint8_t u;

	u = USBCON;
	if ((u & (1<<USBE)) && !(u & (1<<FRZCLK))) {
		media_restart();
		return;
	}
	HW_CONFIG();
	USB_FREEZE();				// enable USB
	PLL_CONFIG();				// config PLL
	while (!(PLLCSR & (1<<PLOCK))) ;	// wait for PLL lock
	USB_CONFIG();				// start USB clock
	UDCON = 0;				// enable attach resistor
	usb_configuration = 0;
	usb_suspended = 0;
	debug_flush_timer = 0;
	keyboard_modifier_keys = 0;
	keyboard_keys[0] = 0;
	keyboard_keys[1] = 0;
	keyboard_keys[2] = 0;
	keyboard_keys[3] = 0;
	keyboard_keys[4] = 0;
	keyboard_keys[5] = 0;
	keyboard_protocol = 1;
	keyboard_idle_config = 125;
	keyboard_idle_count = 0;
	keyboard_leds = 0;
	ms_state = MS_STATE_IDLE;
	ms_rdonly = READ_ONLY;
	UDINT = 0;
	UDIEN = (1<<EORSTE)|(1<<SOFE);
	//sei();  // init() in wiring.c does this
	media_init();
}

void usb_shutdown(void)
{
	UDIEN = 0;		// disable interrupts
	UDCON = 1;		// disconnect attach resistor
	USBCON = 0;		// shut off USB periperal
	PLLCSR = 0;		// shut off PLL
	usb_configuration = 0;
	usb_suspended = 1;
}


// Public API functions moved to usb_api.cpp





/**************************************************************************
 *
 *  Storage Media Access Functions
 *
 **************************************************************************/

#if defined(USB_DISK_SDFLASH)
#include "media_sdcard.c"
#elif defined(USB_DISK)
#include "media_internal.c"
#else
#error "No known media type specified\n"
#endif


/**************************************************************************
 *
 *  Private Functions - not intended for general user consumption....
 *
 **************************************************************************/




static inline void mass_storage_isr(void) __attribute__((always_inline));
static inline void mass_storage_isr(void)
{
	uint8_t n, state;
	const uint8_t *p;
	static uint32_t tag, xfer_len, lba;
	static uint8_t dir, cmd_len, status;
	static uint8_t scsi_sense, scsi_asense;
	static uint16_t sector_count;
	static uint8_t sector_chunk;
	static uint8_t previous_media_state, do_media_status;
	uint8_t cmd;

	UENUM = DISK_RX_ENDPOINT;
	if ((UEIENX & (1<<RXOUTE)) && (UEINTX & (1<<RXOUTI))) {
		n = UEBCLX;
		state = ms_state;
		if (state == MS_STATE_IDLE) {
			if (n != 31) goto error_stall;
			if (UEDATX != 0x55) goto error_stall;
			if (UEDATX != 0x53) goto error_stall;
			if (UEDATX != 0x42) goto error_stall;
			if (UEDATX != 0x43) goto error_stall;
			read_dword_lsbfirst(tag, UEDATX);
			read_dword_lsbfirst(xfer_len, UEDATX);
			dir = UEDATX & 0x80;
			UEDATX; // LUN
			cmd_len = UEDATX;
			status = 0;
			cmd = UEDATX;
			do_media_status = 0;
			if (cmd == SCSI_CMD_REQUEST_SENSE) { // 0x03
				UEINTX = 0x6B;
				UENUM = DISK_TX_ENDPOINT;
				UEDATX = 0x70;
				UEDATX = 0;
				UEDATX = scsi_sense;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = 10;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = 0;
				UEDATX = scsi_asense;
				xfer_len -= 13;
				goto send_finishup;
			}
			scsi_sense = SCSI_SENSE_OK;
			scsi_asense = SCSI_ASENSE_NONE;
			if (cmd == SCSI_CMD_INQUIRY) {	// 0x12
				// http://en.wikipedia.org/wiki/SCSI_Inquiry_Command
				uint16_t len;
				UEDATX;		// ignore request bits
				UEDATX;
				read_word_msbfirst(len, UEDATX);
				UEINTX = 0x6B;
				if (len > sizeof(scsi_inquiry_response))
					len = sizeof(scsi_inquiry_response);
				p = scsi_inquiry_response;
				UENUM = DISK_TX_ENDPOINT;
				for (n=len; n > 0; n--) {
					pgm_read_byte_postinc(UEDATX, p);
				}
				xfer_len -= len;
				goto send_finishup;
			}
			previous_media_state = media_get_state();
			do_media_status = 1;

			// all commands below this point need media access
			// TODO: obtain lock on media, if media already
			// locked, schedule to run later... how??

			if (cmd == SCSI_CMD_TEST_UNIT_READY) { // 0x00
				media_poll();
				UEINTX = 0x3A;
				goto send_status;
			}

			// all commands below this point need the media
			// to be present
			if (!(previous_media_state & MEDIA_PRESENT_MASK)) {
					UEINTX = 0x6B;
					UENUM = DISK_TX_ENDPOINT;
					UEIENX = 0;
					UECONX = (1<<STALLRQ) | (1<<EPEN);
					ms_state = MS_STATE_SEND_STATUS;
					return;
			}

			if (cmd == SCSI_CMD_MODE_SENSE_6  // 0x1A
			  || cmd == SCSI_CMD_MODE_SENSE_10) {	 // 0x5A
				// 1A 00 1C 00 C0 00 - from disk.sys
				// 1A 00 3F 00 C0 00 - from classpnp.sys
				// 1A 00 3F 00 C0 00 - from linux kernel
				// check allocation length...
				UEINTX = 0x6B;
				UENUM = DISK_TX_ENDPOINT;
				UEDATX = 0x03;
				UEDATX = 0x00;
				UEDATX = ms_rdonly ? 0x80 : 0x00; // WP bit
				UEDATX = 0x00;
				xfer_len -= 4;
				goto send_finishup;
			} else if (cmd == SCSI_CMD_READ_CAPACITY) { // 0x25
				uint32_t capacity;
				UEINTX = 0x6B;
				media_poll();
				capacity = media_size();
				UENUM = DISK_TX_ENDPOINT;
				write_dword_msbfirst(capacity - 1, UEDATX);
				UEDATX = 0x00;
				UEDATX = 0x00;
				UEDATX = 0x02;
				UEDATX = 0x00;
				xfer_len -= 8;
				goto send_finishup;
			} else if (cmd == SCSI_CMD_READ_FORMAT_CAPACITIES) { // 0x23
				uint32_t capacity;
				UEINTX = 0x6B;
				media_poll();
				capacity = media_size();
				UENUM = DISK_TX_ENDPOINT;
				UEDATX = 0x00;
				UEDATX = 0x00;
				UEDATX = 0x00;
				UEDATX = 0x08;
				write_dword_msbfirst(capacity, UEDATX);
				UEDATX = 2 + status;	// 2 = formatted, 3 = no media present
				UEDATX = 0x00;
				UEDATX = 0x02;	// 512 byte blocks
				UEDATX = 0x00;
				xfer_len -= 12;
				goto send_finishup;
			} else if (cmd == SCSI_CMD_READ_10) { // 0x28
				// http://en.wikipedia.org/wiki/SCSI_Read_Commands#Read_.2810.29
				UEDATX;
				read_dword_msbfirst(lba, UEDATX);
				UEDATX;
				read_word_msbfirst(sector_count, UEDATX);
				UEINTX = 0x6B;
				if (!(media_get_state() & MEDIA_PRESENT_MASK)) {
					UENUM = DISK_TX_ENDPOINT;
					goto send_finishup;
				}
				media_send_begin(lba);
				sector_chunk = 0;
				ms_state = MS_STATE_SEND_DATA;
				goto send_data;
			} else if (cmd == SCSI_CMD_WRITE_10) { // 0x2A
				UEDATX;
				read_dword_msbfirst(lba, UEDATX);
				UEDATX;
				read_word_msbfirst(sector_count, UEDATX);
				UEINTX = 0x6B;
				sector_chunk = 0;
				media_receive_begin(lba);
				ms_state = MS_STATE_RECEIVE_DATA;
				n = UEBCLX;
				if (n > 0) goto receive_data;
				return;
			} else {
				UEINTX = 0x6B;
				scsi_sense = SCSI_SENSE_ILLEGAL_REQUEST;
				scsi_asense = SCSI_ASENSE_INVALID_COMMAND;
				status = 1;
				goto send_finishup;
			}
		} else if (state == MS_STATE_RECEIVE_DATA) {
			receive_data:
			if (sector_count > 0) {
				media_receive_chunk(lba, sector_chunk);
				xfer_len -= 64;
				if (++sector_chunk >= 8) {
					sector_chunk = 0;
					lba++;
					if (--sector_count == 0) {
						media_receive_end();
						goto send_status;
					}
				}
			} else {
				UEINTX = 0x6B;
			}
		} else {
			UEINTX = 0x6B;
		}
	}
	UENUM = DISK_TX_ENDPOINT;
	if ((UEIENX & (1<<TXINE)) && (UEINTX & (1<<TXINI))) {
		state = ms_state;
		if (state == MS_STATE_SEND_STATUS) goto send_status_force;
		if (state == MS_STATE_SEND_ZEROPAD) goto send_finishup;
		if (state == MS_STATE_SEND_DATA) goto send_data;
		UEIENX = 0;
	}
	return;

send_data:
	UENUM = DISK_TX_ENDPOINT;
	if (sector_count > 0 && (UEINTX & (1<<RWAL))) {
		media_send_chunk(lba, sector_chunk);
		xfer_len -= 64;
		if (++sector_chunk >= 8) {
			sector_chunk = 0;
			lba++;
			if (--sector_count == 0) {
				media_send_end();
				ms_state = MS_STATE_SEND_STATUS;
			}
		}
	}
	UEIENX = (1<<TXINE);
	return;
send_finishup:
	while (xfer_len > 0 && (UEINTX & (1<<RWAL))) {
		UEDATX = 0;
		xfer_len--;
	}
	UEINTX = 0x3A;
	if (xfer_len) {
		ms_state = MS_STATE_SEND_ZEROPAD;
		UEIENX = (1<<TXINE);
		return;
	}
send_status:
	UENUM = DISK_TX_ENDPOINT;
	if (!(UEINTX & (1<<RWAL))) {
		ms_state = MS_STATE_SEND_STATUS;
		UEIENX = (1<<TXINE);
		return;
	}
send_status_force:
	if (do_media_status) {
		uint8_t current_media_state = media_get_state();
		if (status == 0) {
			if (!(current_media_state & MEDIA_PRESENT_MASK)) {
				scsi_sense = SCSI_SENSE_NOT_READY;
				scsi_asense = SCSI_ASENSE_MEDIUM_NOT_PRESENT;
				status = 1;
			} else {
				if (!(previous_media_state & MEDIA_PRESENT_MASK)) {
					scsi_sense = SCSI_SENSE_UNIT_ATTENTION;
					scsi_asense = SCSI_ASENSE_NOT_READY_TO_READY;
					status = 1;
				}
			}
		}
		do_media_status = 0;
	}
	UEDATX = 0x55;
	UEDATX = 0x53;
	UEDATX = 0x42;
	UEDATX = 0x53;
	write_dword_lsbfirst(tag, UEDATX);
	write_dword_lsbfirst(xfer_len, UEDATX);
	UEDATX = status;
	UEINTX = 0x3A;
	UEIENX = 0;
	ms_state = MS_STATE_IDLE;
	return;
error_stall:
	ms_state = MS_STATE_STALLED;
	UENUM = DISK_RX_ENDPOINT;
	UECONX = (1<<STALLRQ) | (1<<EPEN);
	UENUM = DISK_TX_ENDPOINT;
	//UEIENX = 0;
	UECONX = (1<<STALLRQ) | (1<<EPEN);
	return;
}



// Misc functions to wait for ready and send/receive packets
static inline void usb_wait_in_ready(void)
{
	while (!(UEINTX & (1<<TXINI))) ;
}
static inline void usb_send_in(void)
{
	UEINTX = ~(1<<TXINI);
}
static inline void usb_wait_receive_out(void)
{
	while (!(UEINTX & (1<<RXOUTI))) ;
}
static inline void usb_ack_out(void)
{
	UEINTX = ~(1<<RXOUTI);
}



// USB Endpoint Interrupt - endpoint 0 is handled here.  The
// other endpoints are manipulated by the user-callable
// functions, and the start-of-frame interrupt.
//
static inline void endpoint0_isr(void) __attribute__((always_inline));
static inline void endpoint0_isr(void)
{
	uint8_t intbits;
	const uint8_t *list;
	const uint8_t *cfg;
	uint8_t i, n, len, en;
	uint8_t *p;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t desc_val;
	const uint8_t *desc_addr;
	uint8_t	desc_length;

	UENUM = 0;
	intbits = UEINTX;
	if (intbits & (1<<RXSTPI)) {
		bmRequestType = UEDATX;
		bRequest = UEDATX;
		read_word_lsbfirst(wValue, UEDATX);
		read_word_lsbfirst(wIndex, UEDATX);
		read_word_lsbfirst(wLength, UEDATX);
		UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI));
		if (bRequest == GET_DESCRIPTOR) {
			list = (const uint8_t *)descriptor_list;
			for (i=0; ; i++) {
				if (i >= NUM_DESC_LIST) {
					UECONX = (1<<STALLRQ)|(1<<EPEN);  //stall
					return;
				}
				pgm_read_word_postinc(desc_val, list);
				if (desc_val != wValue) {
					list += sizeof(struct descriptor_list_struct)-2;
					continue;
				}
				pgm_read_word_postinc(desc_val, list);
				if (desc_val != wIndex) {
					list += sizeof(struct descriptor_list_struct)-4;
					continue;
				}
				pgm_read_word_postinc(desc_addr, list);
				desc_length = pgm_read_byte(list);
				break;
			}
			len = (wLength < 256) ? wLength : 255;
			if (len > desc_length) len = desc_length;
			list = desc_addr;
			do {
				// wait for host ready for IN packet
				do {
					i = UEINTX;
				} while (!(i & ((1<<TXINI)|(1<<RXOUTI))));
				if (i & (1<<RXOUTI)) return;	// abort
				// send IN packet
				n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
				for (i = n; i; i--) {
					pgm_read_byte_postinc(UEDATX, list);
				}
				len -= n;
				usb_send_in();
			} while (len || n == ENDPOINT0_SIZE);
			return;
		}
		if (bRequest == SET_ADDRESS) {
			usb_send_in();
			usb_wait_in_ready();
			UDADDR = wValue | (1<<ADDEN);
			return;
		}
		if (bRequest == SET_CONFIGURATION && bmRequestType == 0) {
			usb_configuration = wValue;
			debug_flush_timer = 0;
			usb_send_in();
			cfg = endpoint_config_table;
			for (i=1; i<NUM_ENDPOINTS; i++) {
				UENUM = i;
				pgm_read_byte_postinc(en, cfg);
				UECONX = en;
				if (en) {
					pgm_read_byte_postinc(UECFG0X, cfg);
					pgm_read_byte_postinc(UECFG1X, cfg);
				}
			}
			UERST = 0x1E;
			UERST = 0;
			UENUM = DISK_RX_ENDPOINT;
			UEIENX = (1<<RXOUTE);
			return;
		}
		if (bRequest == GET_CONFIGURATION && bmRequestType == 0x80) {
			usb_wait_in_ready();
			UEDATX = usb_configuration;
			usb_send_in();
			return;
		}
		if (bRequest == GET_STATUS) {
			usb_wait_in_ready();
			i = 0;
			if (bmRequestType == 0x82) {
				UENUM = wIndex;
				if (UECONX & (1<<STALLRQ)) i = 1;
				UENUM = 0;
			}
			UEDATX = i;
			UEDATX = 0;
			usb_send_in();
			return;
		}
		if ((bRequest == CLEAR_FEATURE || bRequest == SET_FEATURE)
		  && bmRequestType == 0x02 && wValue == 0) {
			i = wIndex & 0x7F;
			if (i >= 1 && i <= MAX_ENDPOINT) {
				usb_send_in();
				UENUM = i;
				if (bRequest == SET_FEATURE) {
					UECONX = (1<<STALLRQ)|(1<<EPEN);
				} else {
					UECONX = (1<<STALLRQC)|(1<<RSTDT)|(1<<EPEN);
					UERST = (1 << i);
					UERST = 0;
					if (i == DISK_TX_ENDPOINT) {
						UEIENX = (1<<TXINE);
					}
				}
				return;
			}
		}
		if (wIndex == KEYBOARD_INTERFACE) {
			if (bmRequestType == 0xA1) {
				if (bRequest == HID_GET_REPORT) {
					usb_wait_in_ready();
					UEDATX = keyboard_modifier_keys;
					UEDATX = 0;
					for (i=0; i<6; i++) {
						UEDATX = keyboard_keys[i];
					}
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_IDLE) {
					usb_wait_in_ready();
					UEDATX = keyboard_idle_config;
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_PROTOCOL) {
					usb_wait_in_ready();
					UEDATX = keyboard_protocol;
					usb_send_in();
					return;
				}
			}
			if (bmRequestType == 0x21) {
				if (bRequest == HID_SET_REPORT) {
					usb_wait_receive_out();
					keyboard_leds = UEDATX;
					usb_ack_out();
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_IDLE) {
					keyboard_idle_config = (wValue >> 8);
					keyboard_idle_count = 0;
					//usb_wait_in_ready();
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_PROTOCOL) {
					keyboard_protocol = wValue;
					//usb_wait_in_ready();
					usb_send_in();
					return;
				}
			}
		}
		if (wIndex == DEBUG_INTERFACE) {
			if (bRequest == HID_GET_REPORT && bmRequestType == 0xA1) {
				len = wLength;
				do {
					// wait for host ready for IN packet
					do {
						i = UEINTX;
					} while (!(i & ((1<<TXINI)|(1<<RXOUTI))));
					if (i & (1<<RXOUTI)) return;	// abort
					// send IN packet
					n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
					for (i = n; i; i--) {
						UEDATX = 0;
					}
					len -= n;
					usb_send_in();
				} while (len || n == ENDPOINT0_SIZE);
				return;
			}
			if (bRequest == HID_SET_REPORT && bmRequestType == 0x21) {
				if (wValue == 0x0300 && wLength == 0x0004) {
					uint8_t b1, b2, b3, b4;
					usb_wait_receive_out();
					b1 = UEDATX;
					b2 = UEDATX;
					b3 = UEDATX;
					b4 = UEDATX;
					usb_ack_out();
					usb_send_in();
					if (b1 == 0xA9 && b2 == 0x45 && b3 == 0xC2 && b4 == 0x6B)
						_reboot_Teensyduino_();
					if (b1 == 0x8B && b2 == 0xC5 && b3 == 0x1D && b4 == 0x70)
						_restart_Teensyduino_();
				}
			}
		}
		if (wIndex == DISK_INTERFACE) {
			if (bRequest == MS_BULK_ONLY_RESET && bmRequestType == 0x21) {
				usb_wait_in_ready();
				ms_state = MS_STATE_IDLE;
				usb_send_in();
				return;
			}
		}
	}
	UECONX = (1<<STALLRQ) | (1<<EPEN);	// stall
}


// USB Endpoint Interrupt
ISR(USB_COM_vect)
{
	uint8_t epnum_save;

	epnum_save = UENUM;
	endpoint0_isr();
	mass_storage_isr();
	UENUM = epnum_save;
}



// USB Device Interrupt - handle all device-level events
// the transmit buffer flushing is triggered by the start of frame
//
ISR(USB_GEN_vect)
{
	uint8_t intbits, t, i;
	static uint8_t div4=0;

	intbits = UDINT;
	UDINT = 0;
	if (intbits & (1<<EORSTI)) {
		UENUM = 0;
		UECONX = 1;
		UECFG0X = EP_TYPE_CONTROL;
		UECFG1X = EP_SIZE(ENDPOINT0_SIZE) | EP_SINGLE_BUFFER;
		UEIENX = (1<<RXSTPE);
		usb_configuration = 0;
		UENUM = DISK_RX_ENDPOINT;
		UEIENX = 0;
		UENUM = DISK_TX_ENDPOINT;
		UEIENX = 0;
		ms_state = MS_STATE_IDLE;
	}
	if ((intbits & (1<<SOFI)) && usb_configuration) {
		t = debug_flush_timer;
		if (t) {
			debug_flush_timer = -- t;
			if (!t) {
				UENUM = DEBUG_TX_ENDPOINT;
				while ((UEINTX & (1<<RWAL))) {
					UEDATX = 0;
				}
				UEINTX = 0x3A;
			}
		}
		if (keyboard_idle_config && (++div4 & 3) == 0) {
			UENUM = KEYBOARD_ENDPOINT;
			if (UEINTX & (1<<RWAL)) {
				keyboard_idle_count++;
				if (keyboard_idle_count == keyboard_idle_config) {
					keyboard_idle_count = 0;
					UEDATX = keyboard_modifier_keys;
					UEDATX = 0;
					for (i=0; i<6; i++) {
						UEDATX = keyboard_keys[i];
					}
					UEINTX = 0x3A;
				}
			}
		}
	}
	if (intbits & (1<<SUSPI)) {
		// USB Suspend (inactivity for 3ms)
		UDIEN = (1<<WAKEUPE);
		usb_configuration = 0;
		usb_suspended = 1;
		#if (F_CPU >= 8000000L)
		// WAKEUPI does not work with USB clock freeze 
		// when CPU is running less than 8 MHz.
		// Is this a hardware bug?
		USB_FREEZE();			// shut off USB
		PLLCSR = 0;			// shut off PLL
		#endif
		// to properly meet the USB spec, current must
		// reduce to less than 2.5 mA, which means using
		// powerdown mode, but that breaks the Arduino
		// user's paradigm....
	}
	if (usb_suspended && (intbits & (1<<WAKEUPI))) {
		// USB Resume (pretty much any activity)
		#if (F_CPU >= 8000000L)
		PLL_CONFIG();
		while (!(PLLCSR & (1<<PLOCK))) ;
		USB_CONFIG();
		#endif
		UDIEN = (1<<EORSTE)|(1<<SOFE)|(1<<SUSPE);
		usb_suspended = 0;
		return;
	}
}
