#ifndef _USB_H
#define _USB_H

#ifndef MIN
#define MIN(a, b)   (((a)<(b))?(a):(b))
#endif

#include "os_usb.h"

#define  CONTROL_MAX_PKT_SIZE          (64)
#define  INTR_MAX_PKT_SIZE             (64)
#define  BULK_MAX_PKT_SIZE             (64)

#define BULK_RDB_EP 1
#define BULK_RDB_OUT_EP	BULK_RDB_EP
#define BULK_RDB_IN_EP	(BULK_RDB_EP+1)

#define XINPUT_VENDOR_ID 0x045E
#define XINPUT_PRODUCT_ID 0x028E
#define XINPUT_VERSION_ID 0x0114

typedef struct txbuf {
    unsigned char type;
    unsigned char size;
    unsigned char buts0;
    unsigned char buts1;
    unsigned char ltrig;
    unsigned char rtrig;
    unsigned char lx_low;
    unsigned char lx_high;
    unsigned char ly_low;
    unsigned char ly_high;
    unsigned char rx_low;
    unsigned char rx_high;
    unsigned char ry_low;
    unsigned char ry_high;
    unsigned char pad[6];
} __attribute__((packed)) tx;

#define _PTR_      *
#define _CODE_PTR_ *

typedef char _PTR_                    char_ptr;    /* signed character       */
typedef unsigned char  uchar, _PTR_   uchar_ptr;   /* unsigned character     */

typedef signed   char   int_8, _PTR_   int_8_ptr;   /* 8-bit signed integer   */
typedef unsigned char  uint_8, _PTR_   uint_8_ptr;  /* 8-bit signed integer   */

typedef          short int_16, _PTR_   int_16_ptr;  /* 16-bit signed integer  */
typedef unsigned short uint_16, _PTR_  uint_16_ptr; /* 16-bit unsigned integer*/

typedef          long  int_32, _PTR_   int_32_ptr;  /* 32-bit signed integer  */
typedef unsigned long  uint_32, _PTR_  uint_32_ptr; /* 32-bit unsigned integer*/

typedef unsigned long  boolean;  /* Machine representation of a boolean */

typedef void _PTR_     pointer;  /* Machine representation of a pointer */

/* IEEE single precision floating point number (32 bits, 8 exponent bits) */
typedef float          ieee_single;

/* IEEE double precision floating point number (64 bits, 11 exponent bits) */
typedef double         ieee_double;

typedef pointer        _usb_host_handle;

typedef pointer _usb_device_handle;

/* USB 1.1 Setup Packet */
typedef struct setup_struct {
	uint_8      REQUESTTYPE;
	uint_8      REQUEST;
	uint_16     VALUE;
	uint_16     INDEX;
	uint_16     LENGTH;
} SETUP_STRUCT, _PTR_ SETUP_STRUCT_PTR;

/*
 * Device specific functions provided by slave USB drivers
 */
struct usbdevfuncs {
	void (*reset_ep0)(_usb_device_handle,boolean,uint_8,uint_8_ptr,uint_32);
	void (*get_desc)(_usb_device_handle,boolean,SETUP_STRUCT_PTR);
	void (*vendor)(_usb_device_handle,boolean,SETUP_STRUCT_PTR);
	void (*initeps)(_usb_device_handle);
	void (*query)(OSBbUsbInfo *);
    void (*stall_ep)(_usb_device_handle, uint_8, boolean);
};

#define  USB_CONTROL_ENDPOINT             (0x03)
#define  USB_BULK_ENDPOINT                (0x04)

#define  USB_STATUS_IDLE                     (0)

#define  USB_RECV                         (0)
#define  USB_SEND                         (1)

#define USB_DEV_STATE_INIT	0
#define USB_DEV_STATE_RESET	1
#define USB_DEV_STATE_ATTACHED	2

extern uint_8 _usb_device_send_data(_usb_device_handle, uint_8, uchar_ptr, uint_32);
extern uint_8 _usb_device_recv_data(_usb_device_handle, uint_8, uchar_ptr, uint_32);
extern uint_8 _usb_device_get_transfer_status(_usb_device_handle, uint_8, uint_8);   
extern uint_8 _usb_device_cancel_transfer(_usb_device_handle, uint_8, uint_8); 
extern uint_8 _usb_device_init_endpoint(_usb_device_handle, uint_8, uint_16, uint_8, uint_8, uint_8);
extern uint_8 _usb_device_register_service(_usb_device_handle, uint_8, void(_CODE_PTR_)(pointer, boolean, uint_8, uint_8_ptr, uint_32));
extern void _usb_device_stall_endpoint(_usb_device_handle, uint_8, uint_8);

typedef struct dev_global_struct_s {
	uint_8			dev_state;
	uint_8			FIRST_SOF;
	uint_8                  num_ifcs;
	struct usbdevfuncs	*funcs;
} DEV_GLOBAL_STRUCT, _PTR_ DEV_GLOBAL_STRUCT_PTR;

extern DEV_GLOBAL_STRUCT dev_global_struct;

#define USB_uint_16_low(x)                   ((x) & 0xFF)
#define USB_uint_16_high(x)                  (((x) >> 8) & 0xFF)

#define swab16(in) ((((in) & 0xff) << 8) | (((in) & 0xff00) >> 8))

void xinput_query(OSBbUsbInfo *ip);

extern void xinput_ch9GetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr);
extern void xinput_reset_ep0(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length);
extern void xinput_init_data_eps(_usb_device_handle handle);
extern void xinput_stall_data_eps(_usb_device_handle handle, uint_8 ep_num, boolean stall);
extern void xinput_query(OSBbUsbInfo *ip);

extern tx tx_buf;

#endif