/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

/**
 * \file
 * This is an implementation of a USB CDC Virtual COM port.
 */

#include <string.h>
#include <M451Series.h>
#include <USB_VirtualCOM.h>
#include <USB.h>

/* Endpoints */
#define USB_VCOM_CTRL_IN_EP  EP0
#define USB_VCOM_CTRL_OUT_EP EP1
#define USB_VCOM_BULK_IN_EP  EP2
#define USB_VCOM_BULK_OUT_EP EP3
#define USB_VCOM_INT_IN_EP   EP4

/* INTSTS masks */
#define USB_VCOM_CTRL_IN_INTSTS  USBD_INTSTS_EP0
#define USB_VCOM_CTRL_OUT_INTSTS USBD_INTSTS_EP1
#define USB_VCOM_BULK_IN_INTSTS  USBD_INTSTS_EP2
#define USB_VCOM_BULK_OUT_INTSTS USBD_INTSTS_EP3
#define USB_VCOM_INT_IN_INTSTS   USBD_INTSTS_EP4

/* Endpoint numbers */
#define USB_VCOM_CTRL_IN_EP_NUM  0x00
#define USB_VCOM_CTRL_OUT_EP_NUM 0x00
#define USB_VCOM_BULK_IN_EP_NUM  0x01
#define USB_VCOM_BULK_OUT_EP_NUM 0x02
#define USB_VCOM_INT_IN_EP_NUM   0x03

/* Maximum packet sizes */
#define USB_VCOM_CTRL_IN_MAX_PKT_SIZE  64
#define USB_VCOM_CTRL_OUT_MAX_PKT_SIZE 64
#define USB_VCOM_BULK_IN_MAX_PKT_SIZE  64
#define USB_VCOM_BULK_OUT_MAX_PKT_SIZE 64
#define USB_VCOM_INT_IN_MAX_PKT_SIZE   8

/* Buffers */
#define USB_VCOM_SETUP_BUF_BASE    0
#define USB_VCOM_SETUP_BUF_SIZE    8
#define USB_VCOM_CTRL_IN_BUF_BASE  (USB_VCOM_SETUP_BUF_BASE + USB_VCOM_SETUP_BUF_SIZE)
#define USB_VCOM_CTRL_IN_BUF_SIZE  USB_VCOM_CTRL_IN_MAX_PKT_SIZE
#define USB_VCOM_CTRL_OUT_BUF_BASE (USB_VCOM_CTRL_IN_BUF_BASE + USB_VCOM_CTRL_IN_BUF_SIZE)
#define USB_VCOM_CTRL_OUT_BUF_SIZE USB_VCOM_CTRL_OUT_MAX_PKT_SIZE
#define USB_VCOM_BULK_IN_BUF_BASE  (USB_VCOM_CTRL_OUT_BUF_BASE + USB_VCOM_CTRL_OUT_BUF_SIZE)
#define USB_VCOM_BULK_IN_BUF_SIZE  USB_VCOM_BULK_IN_MAX_PKT_SIZE
#define USB_VCOM_BULK_OUT_BUF_BASE (USB_VCOM_BULK_IN_BUF_BASE + USB_VCOM_BULK_IN_BUF_SIZE)
#define USB_VCOM_BULK_OUT_BUF_SIZE USB_VCOM_BULK_OUT_MAX_PKT_SIZE
#define USB_VCOM_INT_IN_BUF_BASE   (USB_VCOM_BULK_OUT_BUF_BASE + USB_VCOM_BULK_OUT_BUF_SIZE)
#define USB_VCOM_INT_IN_BUF_SIZE   USB_VCOM_INT_IN_MAX_PKT_SIZE

/* CDC class specific requests */
#define USB_VCOM_REQ_SET_LINE_CODE          0x20
#define USB_VCOM_REQ_GET_LINE_CODE          0x21
#define USB_VCOM_REQ_SET_CONTROL_LINE_STATE 0x22

/* USB VID and PID */
#define USB_VCOM_VID 0x0416
#define USB_VCOM_PID 0xB002

/* Virtual COM index */
#define USB_VCOM_INDEX 0

/**
 * USB device descriptor.
 */
static const uint8_t USB_VirtualCOM_DeviceDesc[LEN_DEVICE] = {
	LEN_DEVICE,  /* bLength */
	DESC_DEVICE, /* bDescriptorType */
	0x10, 0x01,  /* bcdUSB */
	0x02,        /* bDeviceClass */
	0x00,        /* bDeviceSubClass */
	0x00,        /* bDeviceProtocol */
	/* bMaxPacketSize0 */
	USB_VCOM_CTRL_IN_MAX_PKT_SIZE,
	/* idVendor */
	USB_VCOM_VID & 0x00FF,
	(USB_VCOM_VID & 0xFF00) >> 8,
	/* idProduct */
	USB_VCOM_PID & 0x00FF,
	(USB_VCOM_PID & 0xFF00) >> 8,
	0x00, 0x03,  /* bcdDevice */
	0x01,        /* iManufacture */
	0x02,        /* iProduct */
	0x03,        /* iSerialNumber */
	0x01         /* bNumConfigurations */
};

/**
 * USB configuration descriptor.
 */
static const uint8_t USB_VirtualCOM_ConfigDesc[0x43] = {
	LEN_CONFIG,     /* bLength */
	DESC_CONFIG,    /* bDescriptorType */
	0x43, 0x00,     /* wTotalLength    */
	0x02,           /* bNumInterfaces  */
	0x01,           /* bConfigurationValue */
	0x00,           /* iConfiguration */
	0xC0,           /* bmAttributes */
	0x32,           /* bMaxPower */

	/* INTERFACE descriptor */
	LEN_INTERFACE,  /* bLength */
	DESC_INTERFACE, /* bDescriptorType */
	0x00,           /* bInterfaceNumber */
	0x00,           /* bAlternateSetting */
	0x01,           /* bNumEndpoints */
	0x02,           /* bInterfaceClass */
	0x02,           /* bInterfaceSubClass */
	0x01,           /* bInterfaceProtocol */
	0x00,           /* iInterface */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x00,           /* Header functional descriptor subtype */
	0x10, 0x01,     /* Communication device compliant to the communication spec. ver. 1.10 */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x01,           /* Call management functional descriptor */
	0x00,           /* BIT0: Whether device handle call management itself. */
	/* BIT1: Whether device can send/receive call management information over a Data Class Interface 0 */
	0x01,           /* Interface number of data class interface optionally used for call management */

	/* Communication Class Specified INTERFACE descriptor */
	0x04,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x02,           /* Abstract control management functional descriptor subtype */
	0x00,           /* bmCapabilities */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* bLength */
	0x24,           /* bDescriptorType: CS_INTERFACE descriptor type */
	0x06,           /* bDescriptorSubType */
	0x00,           /* bMasterInterface */
	0x01,           /* bSlaveInterface0 */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                        /* bLength */
	DESC_ENDPOINT,                       /* bDescriptorType */
	(EP_INPUT | USB_VCOM_INT_IN_EP_NUM), /* bEndpointAddress */
	EP_INT,                              /* bmAttributes */
	USB_VCOM_INT_IN_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
	0x01,                                /* bInterval */

	/* INTERFACE descriptor */
	LEN_INTERFACE,  /* bLength */
	DESC_INTERFACE, /* bDescriptorType */
	0x01,           /* bInterfaceNumber */
	0x00,           /* bAlternateSetting */
	0x02,           /* bNumEndpoints  */
	0x0A,           /* bInterfaceClass */
	0x00,           /* bInterfaceSubClass */
	0x00,           /* bInterfaceProtocol */
	0x00,           /* iInterface */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                         /* bLength */
	DESC_ENDPOINT,                        /* bDescriptorType */
	(EP_INPUT | USB_VCOM_BULK_IN_EP_NUM), /* bEndpointAddress */
	EP_BULK,                              /* bmAttributes */
	USB_VCOM_BULK_IN_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
	0x00,                                 /* bInterval */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                           /* bLength */
	DESC_ENDPOINT,                          /* bDescriptorType */
	(EP_OUTPUT | USB_VCOM_BULK_OUT_EP_NUM), /* bEndpointAddress */
	EP_BULK,                                /* bmAttributes */
	USB_VCOM_BULK_OUT_MAX_PKT_SIZE, 0x00,   /* wMaxPacketSize */
	0x00,                                   /* bInterval */
};

/**
 * USB language string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringLang[4] = {
	4,           /* bLength */
	DESC_STRING, /* bDescriptorType */
	0x09, 0x04
};

/**
 * USB vendor string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringVendor[16] = {
	16,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/**
 * USB product string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringProduct[32] = {
	32,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'U', 0, 'S', 0, 'B', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0
};

/**
 * USB serial string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringSerial[26] = {
	26,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'A', 0, '0', 0, '2', 0, '0', 0, '1', 0, '4', 0, '0', 0, '9', 0, '0', 0, '3', 0, '0', 0, '5', 0
};

/**
 * USB string descriptor pointers.
 */
static const uint8_t *USB_VirtualCOM_StringDescs[4] = {
	USB_VirtualCOM_StringLang,
	USB_VirtualCOM_StringVendor,
	USB_VirtualCOM_StringProduct,
	USB_VirtualCOM_StringSerial
};

/**
 * USBD information structure.
 */
static const S_USBD_INFO_T USB_VirtualCOM_UsbdInfo = {
	USB_VirtualCOM_DeviceDesc,
	USB_VirtualCOM_ConfigDesc,
	USB_VirtualCOM_StringDescs,
	NULL,
	NULL,
	NULL
};

/**
 * Structure for USB CDC line coding data.
 */
typedef struct {
	/**< Data terminal rate, in bits per second. */
	uint32_t dwDTERate;
	/**< Stop bits. */
	uint8_t bCharFormat;
	/**< Parity. */
	uint8_t bParityType;
	/**< Data bits. */
	uint8_t bDataBits;
} USB_VirtualCOM_LineCoding_t;
#define USB_VirtualCOM_LineCoding_t_SIZE 7

/**
 * Line coding data.
 * This is ignored, but stored for GET_LINE_CODE.
 * Default values: 115200 bauds, 1 stop bit, no parity, 8 data bits.
 */
static USB_VirtualCOM_LineCoding_t lineCoding = {115200, 0, 0, 8};

/**
 * True when USB is ready for a device-to-host packet.
 */
static volatile uint8_t USB_VirtualCOM_canTx = 0;

/**
 * Handler for bulk IN transfers.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleBulkIn() {
	// TODO: implement > 63 byte transfers
	USB_VirtualCOM_canTx = 1;
}

/**
 * Handler for bulk OUT transfers.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleBulkOut() {
	// TODO: implement host-to-device transfers
}

/**
 * USB virtual COM interrupt handler.
 * This is an internal function.
 */
static void USB_VirtualCOM_IRQHandler() {
	uint32_t intSts, busState;

	intSts = USBD_GET_INT_FLAG();
	busState = USBD_GET_BUS_STATE();

	if(intSts & USBD_INTSTS_FLDET) {
		if(USBD_IS_ATTACHED()) {
			// USB plugged in
			USBD_ENABLE_USB();
		}
		else {
			// USB unplugged
			USBD_DISABLE_USB();
		}
	}
	else if(intSts & USBD_INTSTS_BUS) {
		if(busState & USBD_STATE_USBRST) {
			// Bus reset
			USBD_ENABLE_USB();
			USBD_SwReset();
		}
		else if(busState & USBD_STATE_SUSPEND) {
			// Suspend: USB enabled, PHY disabled
			USBD_DISABLE_PHY();
		}
		else if(busState & USBD_STATE_RESUME) {
			// Resume: USB enabled, PHY enabled
			USBD_ENABLE_USB();
		}
	}
	else if(intSts & USBD_INTSTS_USB) {
		if(intSts & USBD_INTSTS_SETUP) {
			// Clear IN/OUT data ready flag for control endpoints
			USBD_STOP_TRANSACTION(USB_VCOM_CTRL_IN_EP);
			USBD_STOP_TRANSACTION(USB_VCOM_CTRL_OUT_EP);

			// Handle setup packet
			USBD_ProcessSetupPacket();
		}
		else if(intSts & USB_VCOM_CTRL_IN_INTSTS) {
			// Handle control IN
			USBD_CtrlIn();
		}
		else if(intSts & USB_VCOM_CTRL_OUT_INTSTS) {
			// Handle control OUT
			USBD_CtrlOut();
		}
		else if(intSts & USB_VCOM_BULK_IN_INTSTS) {
			// Handle bulk IN
			USB_VirtualCOM_HandleBulkIn();
		}
		else if(intSts & USB_VCOM_BULK_OUT_INTSTS) {
			// Handle bulk OUT
			USB_VirtualCOM_HandleBulkOut();
		}
	}

	// Clear flag
	USBD_CLR_INT_FLAG(intSts);
}

/**
 * USB CDC class request handler.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleClassRequest() {
	USB_SetupPacket_t setupPacket;

	USBD_GetSetupPacket((uint8_t *) &setupPacket);

	if(setupPacket.bmRequestType & 0x80) {
		// Transfer direction: device to host
		switch(setupPacket.bRequest) {
			case USB_VCOM_REQ_GET_LINE_CODE:
				if(setupPacket.wIndex == USB_VCOM_INDEX) {
					// Copy line coding data to USB buffer
					USBD_MemCopy((uint8_t *) (USBD_BUF_BASE + USB_VCOM_CTRL_IN_BUF_BASE),
						(uint8_t *) &lineCoding, USB_VirtualCOM_LineCoding_t_SIZE);
				}

				// Data stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, USB_VirtualCOM_LineCoding_t_SIZE);

				// Status stage
				USBD_PrepareCtrlOut(NULL, 0);
				break;
			default:
				// Setup error, stall the device
				USBD_SetStall(USB_VCOM_CTRL_IN_EP);
				USBD_SetStall(USB_VCOM_CTRL_OUT_EP);
				break;
		}
	}
	else {
		// Transfer direction: host to device
		switch(setupPacket.bRequest) {
			case USB_VCOM_REQ_SET_CONTROL_LINE_STATE:
				// Control signals are ignored
				// Status stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, 0);
				break;
			case USB_VCOM_REQ_SET_LINE_CODE:
				if(setupPacket.wIndex == USB_VCOM_INDEX) {
					// Prepare for line coding copy
					USBD_PrepareCtrlOut((uint8_t *) &lineCoding, USB_VirtualCOM_LineCoding_t_SIZE);
				}

				// Status stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, 0);
				break;
			default:
				// Setup error, stall the device
				USBD_SetStall(USB_VCOM_CTRL_IN_EP);
				USBD_SetStall(USB_VCOM_CTRL_OUT_EP);
				break;
		}
	}
}

void USB_VirtualCOM_Init() {
	// Open USB
	USBD_Open(&USB_VirtualCOM_UsbdInfo, USB_VirtualCOM_HandleClassRequest, NULL);

	// Initialize setup packet buffer
	USBD->STBUFSEG = USB_VCOM_SETUP_BUF_BASE;

	// Control IN endpoint
	USBD_CONFIG_EP(USB_VCOM_CTRL_IN_EP, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | USB_VCOM_CTRL_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_CTRL_IN_EP, USB_VCOM_CTRL_IN_BUF_BASE);
	// Control OUT endpoint
	USBD_CONFIG_EP(USB_VCOM_CTRL_OUT_EP, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | USB_VCOM_CTRL_OUT_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_CTRL_OUT_EP, USB_VCOM_CTRL_OUT_BUF_BASE);

	// Bulk IN endpoint
	USBD_CONFIG_EP(USB_VCOM_BULK_IN_EP, USBD_CFG_EPMODE_IN | USB_VCOM_BULK_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_BULK_IN_EP, USB_VCOM_BULK_IN_BUF_BASE);
	// Bulk OUT endpoint
	USBD_CONFIG_EP(USB_VCOM_BULK_OUT_EP, USBD_CFG_EPMODE_OUT | USB_VCOM_BULK_OUT_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_BULK_OUT_EP, USB_VCOM_BULK_OUT_BUF_BASE);
	USBD_SET_PAYLOAD_LEN(USB_VCOM_BULK_OUT_EP, USB_VCOM_BULK_OUT_MAX_PKT_SIZE);

	// Interrupt IN endpoint
	USBD_CONFIG_EP(USB_VCOM_INT_IN_EP, USBD_CFG_EPMODE_IN | USB_VCOM_INT_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_INT_IN_EP, USB_VCOM_INT_IN_BUF_BASE);

	// Start USB
	USBD_Start();

	// Enable USB interrupt
	NVIC_EnableIRQ(USBD_IRQn);

	USB_VirtualCOM_canTx = 1;
}

void USB_VirtualCOM_Send(const uint8_t *buf, uint32_t size) {
	// Wait for USB to be ready
	while(!USB_VirtualCOM_canTx);
	USB_VirtualCOM_canTx = 0;

	// TODO: support > 63 byte transfers
	size = Minimum(size, USB_VCOM_BULK_IN_MAX_PKT_SIZE - 1);

	// Transfer buffer
	USBD_MemCopy((uint8_t *) (USBD_BUF_BASE + USB_VCOM_BULK_IN_BUF_BASE), (uint8_t *) buf, size);
	USBD_SET_PAYLOAD_LEN(USB_VCOM_BULK_IN_EP, size);
}

void USB_VirtualCOM_SendString(const char *str) {
	USB_VirtualCOM_Send((uint8_t *) str, strlen(str));
}

/**
 * Global USB interrupt handler.
 * TODO: make this more flexible when more USB drivers are introduced.
 */
void USBD_IRQHandler() {
	// This handler should be fast.
	// Be careful with display debugging in here, as it may throw off USB timing.
	USB_VirtualCOM_IRQHandler();
}