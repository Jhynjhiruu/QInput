#include "os.h"
#include "os_usb.h"
#include "usb.h"
#include "graph.h"

#define DCACHE_ALIGN	__attribute__ ((aligned (16)))

#define	RIP_BULK_BUFSZ	20*1024	/* bulk buffer size for each direction */

static uchar in_bulk_buf0[RIP_BULK_BUFSZ] DCACHE_ALIGN;
static uchar in_bulk_buf1[RIP_BULK_BUFSZ] DCACHE_ALIGN;
static int cur_inbuf = 0;
static struct {
    uchar *buf;
    int len;	/* amount of valid data after io complete */
} inbuf[2] = { { in_bulk_buf0, 0 }, {in_bulk_buf1, 0 } };

tx tx_buf;

void xinput_query(OSBbUsbInfo *ip)
{
	ip->ua_type = OS_USB_TYPE_DEVICE;
	ip->ua_state = (dev_global_struct.dev_state == USB_DEV_STATE_ATTACHED) ? OS_USB_STATE_ATTACHED : OS_USB_STATE_NULL;
	ip->ua_class = 0xFF;
	ip->ua_subclass = 0xFF;
	ip->ua_protocol = 0xFF;
	ip->ua_vendor = XINPUT_VENDOR_ID;
	ip->ua_product = XINPUT_PRODUCT_ID;
	ip->ua_cfg = 0;		/* XXX */
	ip->ua_ep = 1;		/* XXX */
	ip->ua_speed = OS_USB_FULL_SPEED;	/* XXX */
	ip->ua_mode = OS_USB_MODE_RW;	/* XXX */
	ip->ua_blksize = 8;
	ip->ua_mfr_str = "iQue Ltd";
	ip->ua_prod_str = "iQue Player (disabled)";
	ip->ua_driver_name = "";
	ip->ua_support = TRUE;
}

static uint_8  DevDesc[18] =
{
   /* Length of DevDesc */
   sizeof(DevDesc),
   /* "Device" Type of descriptor */
   1,
   /* BCD USB version */
   USB_uint_16_low(0x200), USB_uint_16_high(0x200),
   /* Device Class is indicated in the interface descriptors */
   0xFF,
   /* Device Subclass is indicated in the interface descriptors */
   0xFF,
   /* RDB device does not use class-specific protocols */
   0xFF,
   /* Max packet size */
   CONTROL_MAX_PKT_SIZE,
   /* Vendor ID */
   USB_uint_16_low(XINPUT_VENDOR_ID), USB_uint_16_high(XINPUT_VENDOR_ID),
   /* Product ID */
   USB_uint_16_low(XINPUT_PRODUCT_ID), USB_uint_16_high(XINPUT_PRODUCT_ID),
   /* BCD Device version */
   USB_uint_16_low(XINPUT_VERSION_ID), USB_uint_16_high(XINPUT_VERSION_ID),
   /* Manufacturer string index */
   0x1,
   /* Product string index */
   0x2,
   /* Serial number string index */
   0x3,
   /* Number of configurations available */
   0x1
};

#define CONFIG_DESC_NUM_INTERFACES  (1)
/* This must be counted manually and updated with the descriptor */
/* 1*Config(9) + 1*Interface(9) + 2*Endpoint(7) = 32 bytes */
#define CONFIG_DESC_SIZE            (49)

static uint_8 ConfigDesc[CONFIG_DESC_SIZE] =
{
    /* Configuration Descriptor - always 9 bytes */
    9,
    /* "Configuration" type of descriptor */
    2,
    /* Total length of the Configuration descriptor */
    USB_uint_16_low(CONFIG_DESC_SIZE), USB_uint_16_high(CONFIG_DESC_SIZE),
    /* NumInterfaces */
    1,
    /* Configuration Value */
    1,
    /* Configuration Description String Index */
    0,
    /* Attributes:  Bus-powered */
    0x80,
    /* Current draw from bus == 500mA (units of 2mA) */
    250,
    /* ----------------- Interfaces ------------------ */
    /* Interface 0 Descriptor - always 9 bytes */
    9,
    /* "Interface" type of descriptor */
    4,
    /* Number of this interface */
    0,
    /* Alternate Setting */
    0,
    /* Number of endpoints on this interface */
    2,
    /* Interface Class: Vendor specific */
    0xFF,
    /* Interface Subclass: Vendor specific */
    0x5D,
    /* Interface Protocol: standard RDB protocol */
    0x01,
    /* Interface Description String Index */
    0,
   
   
    // Unknown Descriptor (If0)
    0x11,        // bLength
    0x21,        // bDescriptorType
    0x00, 0x01, 0x01, 0x25,  // ???
    0x81,        // bEndpointAddress (IN, 1)
    0x14,        // bMaxDataSize
    0x00, 0x00, 0x00, 0x00, 0x13,  // ???
    0x02,        // bEndpointAddress (OUT, 2)
    0x08,        // bMaxDataSize
    0x00, 0x00,  // ???
	// Endpoint 1: Control Surface Send
	0x07,        // bLength
	0x05,        // bDescriptorType (ENDPOINT)
	0x81,        // bEndpointAddress (IN, 1)
	0x03,        // bmAttributes
	0x20, 0x00,  // wMaxPacketSize
	0x04,        // bInterval

	// Endpoint 1: Control Surface Receive
	0x07,        // bLength
	0x05,        // bDescriptorType (ENDPOINT)
	0x02,        // bEndpointAddress (OUT, 2)
	0x03,        // bmAttributes
	0x20, 0x00,  // wMaxPacketSize
	0x08,        // bInterval

};

/* number of strings in the table not including 0 or n. */
#define USB_STR_NUM	2

/*
** if the number of strings changes, look for USB_STR_0 everywhere and make 
** the obvious changes.  It should be found in 3 places.
*/

/* Languages supported = American English (0x0409) */
static uint_8 USB_STR_0[ 4] = {sizeof(USB_STR_0), 0x03, 0x09, 0x04};
static uint_8 USB_STR_1[46] = {sizeof(USB_STR_1), 0x03,
      '©',0,'M',0,'i',0,'c',0,'r',0,'o',0,'s',0,'o',0,	/* 16 chars per row */
      'f',0,'t',0,' ',0,'C',0,'o',0,'r',0,'p',0,'o',0,
      'r',0,'a',0,'t',0,'i',0,'o',0,'n',0};
static uint_8 USB_STR_2[22] = {sizeof(USB_STR_2), 0x03,
      'C',0,'o',0,'n',0,'t',0,'r',0,'o',0,'l',0,'l',0,
      'r',0,'t',0};
static uint_8 USB_STR_n[24] = {sizeof(USB_STR_n), 0x03,
      'i',0,'Q',0,'u',0,'e',0,' ',0,'P',0,'l',0,'a',0,
      'y',0,'e',0,'r',0};

static const uint_8_ptr USB_STRING_DESC[USB_STR_NUM+2] =
{
   (uint_8_ptr)USB_STR_0,
   (uint_8_ptr)USB_STR_1,
   (uint_8_ptr)USB_STR_2,
   (uint_8_ptr)USB_STR_n
};

void xinput_service_bulk_ep
   (
      _usb_device_handle   handle,
      boolean              setup,
      uint_8               direction,
      uint_8_ptr           buffer,
      uint_32              length      
   )
{
	/*
	 * Process input and post another buffer
	 */
    printstr(white, 12, 3, " Communicating... ");
	if (direction == USB_RECV) {
		inbuf[cur_inbuf].len = length;
		/* post the other buffer */
                cur_inbuf = ~cur_inbuf & 1;
                _usb_device_recv_data(handle, BULK_RDB_IN_EP, 
              	    inbuf[cur_inbuf].buf, RIP_BULK_BUFSZ);
		/* call ip6 routine to deal with the new data */
		
        (void)_usb_device_send_data(handle, BULK_RDB_OUT_EP, (unsigned char *)(&tx_buf), 20);
        // not entirely sure why this is needed but eh
		return;
	}

	if (direction == USB_SEND) {
        (void)_usb_device_send_data(handle, BULK_RDB_OUT_EP, (unsigned char *)(&tx_buf), 20);
        return;
	}
}

static char DevDescShort[8];

void xinput_ch9GetDescription
   (
      _usb_device_handle handle,
      boolean setup,
      SETUP_STRUCT_PTR setup_ptr
   )
{
	uint_16 value;
	int len;

   if (setup) {
      /* Load the appropriate string depending on the descriptor requested.*/
      value = swab16(setup_ptr->VALUE) & 0xFF00;
      len = swab16(setup_ptr->LENGTH);

      switch (value) {

         case 0x0100:
	    if (len <= 8) {
		    bcopy((void *)&DevDesc, (void *)&DevDescShort[0], 8);
		    DevDescShort[0] = 8;
            	    _usb_device_send_data(handle, 0, (uchar_ptr)&DevDescShort[0], 8);
	    } else {
                    _usb_device_send_data(handle, 0, (uchar_ptr)&DevDesc,
                               MIN(len, sizeof(DevDesc)));
	    }
            break;

         case 0x0200:
            _usb_device_send_data(handle, 0, (uchar_ptr)&ConfigDesc,
               MIN(len, sizeof(ConfigDesc)));
            break;

         case 0x0300:
            value = swab16(setup_ptr->VALUE) & 0x00FF;
            if (value > USB_STR_NUM) {
               _usb_device_send_data(handle, 0, USB_STRING_DESC[USB_STR_NUM+1],
                  MIN(len, USB_STRING_DESC[USB_STR_NUM+1][0]));
            } else {
               _usb_device_send_data(handle, 0,
                  USB_STRING_DESC[value],
                  MIN(len, USB_STRING_DESC[value][0]));
            }
            break;

         default:
            _usb_device_stall_endpoint(handle, 0, 0);
            return;
      }
      /* status phase */
      _usb_device_recv_data(handle, 0, 0, 0);
      dev_global_struct.dev_state = USB_DEV_STATE_ATTACHED;
   }
   return;
}

void xinput_reset_ep0
   (
      /* [IN] Handle of the USB device */
      _usb_device_handle   handle,
   
      /* [IN] Unused */
      boolean              setup,
   
      /* [IN] Unused */
      uint_8               direction,
   
      /* [IN] Unused */
      uint_8_ptr           buffer,
   
      /* [IN] Unused */
      uint_32              length      
   )
{
   _usb_device_init_endpoint(handle, 0, CONTROL_MAX_PKT_SIZE, 0,
         USB_CONTROL_ENDPOINT, 0);
   _usb_device_init_endpoint(handle, 0, CONTROL_MAX_PKT_SIZE, 1,
         USB_CONTROL_ENDPOINT, 0);

   return;
}

void xinput_init_data_eps(_usb_device_handle handle)
{
	uint_16 ep_num, max_pkt;
	uint_8 ep_type;
	int status;

    ep_num = BULK_RDB_EP;
    ep_type = USB_BULK_ENDPOINT;
    max_pkt = BULK_MAX_PKT_SIZE;
    _usb_device_init_endpoint(handle, BULK_RDB_IN_EP, max_pkt, USB_RECV, ep_type, 0);
    _usb_device_init_endpoint(handle, ep_num, max_pkt, USB_SEND, ep_type, 0);
    _usb_device_register_service(handle, ep_num, xinput_service_bulk_ep);
    _usb_device_register_service(handle, BULK_RDB_IN_EP, xinput_service_bulk_ep);
    status = _usb_device_get_transfer_status(handle, BULK_RDB_IN_EP, USB_RECV);
	if (status != USB_STATUS_IDLE) {
		//printf(">>>rdbs_init_eps: bulk recv already pending!\n");
	} else {
        printstr(white, 12, 3, "     Got USB      ");
		//printf(">>>rdbs_init_eps: post bulk recv buf for ep%d\n", ep_num);
		cur_inbuf = 0;
            	_usb_device_recv_data(handle, BULK_RDB_IN_EP, inbuf[cur_inbuf].buf, RIP_BULK_BUFSZ);
        }
}

void xinput_reset_data_eps(_usb_device_handle handle)
{
    uint_16 ep_num;

    ep_num = BULK_RDB_EP;
    _usb_device_cancel_transfer(handle, ep_num, USB_RECV);
    _usb_device_cancel_transfer(handle, ep_num, USB_SEND);
    //_usb_device_unregister_service(handle, ep_num);
}

void xinput_stall_data_eps(_usb_device_handle handle, uint_8 ep_num, boolean stall)
{
    if ((ep_num == BULK_RDB_EP) && (stall == 0)) {
        /* RDP endpoint unstalled - try to get it back into a known state */
        xinput_reset_data_eps(handle);
        xinput_init_data_eps(handle);
    }
}
