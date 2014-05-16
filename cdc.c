#include <xc.h>
#include <plib.h>
#include <GenericTypeDefs.h>
#include <string.h>

#include "cdc.h"
#include "usbotg.h"
#include "cdc_descriptors.h"
#include "pic32mxusbotg.h"

/*
struct _cdc_ControlLineState {
    int DTR : 1;
    int RTS : 1;
    int unused1 : 6;
    BYTE unused2;
}; // cls;
 */

static BYTE cdc_acm_in_bufferA[CDC_NOTICE_BUFFER_SIZE];
static BYTE cdc_acm_in_bufferB[CDC_NOTICE_BUFFER_SIZE];

static BYTE cdc_In_bufferA[CDC_BUFFER_SIZE];
static BYTE cdc_In_bufferB[CDC_BUFFER_SIZE];
static BYTE cdc_Out_bufferA[CDC_BUFFER_SIZE];
static BYTE cdc_Out_bufferB[CDC_BUFFER_SIZE];

static struct _CDC_LineCodeing {
  __attribute__((packed))
    unsigned long int dwDTERate;
    BYTE bCharFormat;
    BYTE bParityType;
    BYTE bDataBits;
} linecodeing;

static struct _CDC_ControlLineState {
__attribute__((packed))
    BYTE DTR : 1;
__attribute__((packed))
    BYTE RTS : 1;
__attribute__((packed))
    BYTE unused1 : 6;
    BYTE unused2;
} control_line_state;

enum stopbits {
    one = 0, oneandahalf = 1, two = 2
};

enum parity {
    none = 0, odd = 1, even = 2, mark = 3, space = 4
};
//const char parity_str[] = {'N', 'O', 'E', 'M', 'S'};


static volatile struct usbinbuffer {
    const void* inBufPtr;
	int cnt;
    int rdptr;
	int complete_flag;
} usbinbuf;

static volatile struct usboutbuffer {
    BYTE outBuf[CDC_BUFFER_SIZE];
	int cnt;
    int rdptr;
} usboutbuf;

static BYTE LineStateUpdated = 0;

static SW_BD_ENTRY *OutbdpA, *OutbdpB, *InbdpA, *InbdpB;
static BYTE CDCFunctionError;

static volatile unsigned int cdc_in_dts_flag = 0;
static volatile unsigned int cdc_in_ppi = 0;
static volatile unsigned int cdc_out_dts_flag = 0;
static volatile unsigned int cdc_in_ZLP = 0;

static usb_handler_t cdc_startup_handler = NULL;
static usb_handler_t cdc_reset_handler = NULL;
static usb_handler_t cdc_received_handler = NULL;
static usb_handler_t cdc_transmited_handler = NULL;

void initCDC(void) {

    usb_unset_in_handler(1);
    usb_unset_in_handler(2);
    usb_unset_out_handler(2);

	cdc_in_dts_flag = 0;
//	cdc_out_dts_flag = 0;
	
    linecodeing.dwDTERate = 115200;
    linecodeing.bCharFormat = one;
    linecodeing.bParityType = none;
    linecodeing.bDataBits = 8;

    control_line_state.DTR = 0;
    control_line_state.RTS = 0;

    usb_register_class_setup_handler(cdc_setup);
	usb_register_device_setconfiguration_handler(user_configured_init);
}

static int user_configured_init(int cgfidx, SW_BD_ENTRY* usb_main_bdt, unsigned int* intf_num) {
    // After the device is enumerated and configured then we set up non EP0 endpoints.
    // We only enable the endpoints we are using, not all of them.
    // Prior to this they are held in a disarmed state.

    // This function belongs to the current USB function and IS NOT generic. This is CLASS specific
    // and will vary from implementation to implementation.

  LineStateUpdated = 0;

  if (cgfidx > 0 && usb_main_bdt != NULL)
  {
	(*intf_num) = USB_NUM_INTERFACES;
	
    usb_unset_in_handler(1);
    usb_unset_in_handler(2);
    usb_unset_out_handler(2);

    /* Configure buffer descriptors */

    // JTR Setup CDC LINE_NOTICE EP (Interrupt IN)
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->BDCNT = 0;
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_acm_in_bufferA);
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->BDSTAT = 0; // Set DTS => First packet inverts, ie. is Data0
	usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdc_acm_in_bufferA;

    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->BDCNT = 0;
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_acm_in_bufferB);
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->BDSTAT = 0; // Set DTS => First packet inverts, ie. is Data1
    usb_main_bdt[USB_CALC_BD(1, USB_DIR_IN, USB_PP_ODD)].BDADDR = cdc_acm_in_bufferB;

    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].PHY_DB_ENTRY->BDCNT = CDC_BUFFER_SIZE; // JTR N/A endpoints[i].buffer_size;
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_Out_bufferA);
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].PHY_DB_ENTRY->BDSTAT = UOWN | DTSEN;
	usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = cdc_Out_bufferA;

    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_ODD)].PHY_DB_ENTRY->BDCNT = CDC_BUFFER_SIZE; // JTR N/A endpoints[i].buffer_size;
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_ODD)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_Out_bufferB);
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_ODD)].PHY_DB_ENTRY->BDSTAT = UOWN | DATA1 | DTSEN;
	usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_ODD)].BDADDR = cdc_Out_bufferB;


    usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->BDCNT = 0;
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_In_bufferA);
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].PHY_DB_ENTRY->BDSTAT = 0; // Set DTS => First packet inverts, ie. is Data0
	usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)].BDADDR = cdc_In_bufferA;

	usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->BDCNT = 0;
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->PHYBDADDR = (BYTE*)_VirtToPhys(cdc_In_bufferB);
    usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_ODD)].PHY_DB_ENTRY->BDSTAT =  0; // Set DTS => First packet inverts, ie. is Data0
	usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_ODD)].BDADDR = cdc_In_bufferB;

	OutbdpA = &usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
	OutbdpB = &usb_main_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_ODD)];

    InbdpA = &usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];
	InbdpB = &usb_main_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_ODD)];

	USB_UEP1 = USB_EP_IN;
    USB_UEP2 = USB_EP_INOUT;

	cdc_in_ppi = 0;
	cdc_in_dts_flag = 0;
	cdc_out_dts_flag = 0;
	cdc_in_ZLP = 0;

	usbinbuf.complete_flag = true;

	usb_set_in_handler(2, cdc_data_in_handler);
	usb_set_out_handler(2, cdc_data_out_handler);

	if (cdc_startup_handler)
	  cdc_startup_handler();

	return true;
  }
  else
  {
	USB_UEP1 = USB_EP_NONE;
    USB_UEP2 = USB_EP_NONE;

    usb_unset_in_handler(1);
    usb_unset_in_handler(2);
    usb_unset_out_handler(2);

    OutbdpA = NULL;
	OutbdpB = NULL;

    InbdpA = NULL;
	InbdpB = NULL;

    usbinbuf.complete_flag = true;

	if (cdc_reset_handler)
	  cdc_reset_handler();

	if (cdc_received_handler)
	  cdc_received_handler();

	if (cdc_transmited_handler)
	  cdc_transmited_handler();

	return true;
  }

  return false;
}

static void cdc_setup(SW_BD_ENTRY*bdp) {
    BYTE *packet;
    size_t reply_len;
    packet = bdp->BDADDR;
    switch (packet[USB_bmRequestType] & (USB_bmRequestType_TypeMask | USB_bmRequestType_RecipientMask)) {
        case (USB_bmRequestType_Class | USB_bmRequestType_Interface):
            switch (packet[USB_bRequest]) {

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_SEND_ENCAPSULATED_COMMAND:
                    usb_ack_dat1(0);
                    break;

                    //JTR This is just a dummy, nothing defined to do for CDC ACM
                case CDC_GET_ENCAPSULATED_RESPONSE:
                    //usb_ack_zero(rbdp);
                    usb_ack_dat1(0);
                    break;

                case CDC_SET_COMM_FEATURE: // Optional
                case CDC_GET_COMM_FEATURE: // Optional
                case CDC_CLEAR_COMM_FEATURE: // Optional
                    usb_RequestError(); // Not advertised in ACM functional descriptor
                    break;

                case CDC_SET_LINE_CODING: // Optional, strongly recomended
					if (USB_EP0_BUFFER_SIZE - (USB_wLengthHigh + 1) >= sizeof (struct _CDC_ControlLineState))
					{
						memcpy(&linecodeing, (const void *)&packet[USB_wLengthHigh + 1], sizeof (struct _CDC_ControlLineState));
						usb_ack_dat1(0);
			        }
					else
                      usb_set_out_handler(0, cdc_set_line_coding_data); // Register out handler function
					
                    break;

                case CDC_GET_LINE_CODING: // Optional, strongly recomended
                    // JTR reply length (7) is always going to be less than minimum EP0 size (8)

                    reply_len = (size_t)packet[USB_wLength] + (((size_t)packet[USB_wLengthHigh]) << 8);
                    if (sizeof (struct _CDC_LineCodeing) < reply_len) {
                        reply_len = sizeof (struct _CDC_LineCodeing);
                    }

					usb_send_control_data((const void *) &linecodeing, reply_len); // JTR common addition for STD and CLASS ACK
                    
                    break;

                case CDC_SET_CONTROL_LINE_STATE: // Optional

				    memcpy(&control_line_state, (const void *)&packet[USB_wValue], sizeof (struct _CDC_ControlLineState));

                    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK

					LineStateUpdated = 1;

					break;

                case CDC_SEND_BREAK: // Optional
                default:
                    usb_RequestError();
            }
            break;
        default:
            usb_RequestError();
    }
}

static void cdc_set_line_coding_data(SW_BD_ENTRY* bdp) {

    memcpy(&linecodeing, (const void *) bdp->BDADDR, sizeof (struct _CDC_LineCodeing));
  
    usb_unset_out_handler(0); // Unregister OUT handler; JTR serious bug fix in macro!
    usb_ack_dat1(0); // JTR common addition for STD and CLASS ACK

    //  Force EP0 OUT to the DAT0 state
    //  after we have all our data packets.
    usb_out_control_status(bdp);
}


// JTR keep this fragment as it may be useful later if reworked a little
//void cdc_acm_in( void ) {

// JTR LINE_NOTIFICATION EP has been increased to ten bytes.
// This is because if it were ever to be used it is more likely
// that it will be for the Serial State Notification which has
// two bytes of data to include in the packet.

// Also we will not actually come to this code if the LINE_NOTIFICATION
// endpoint is not already armed and we do not arm this end point
// until we have something to send. Therefore we have a chicken and egg
// deadlock and this function is of no value the way it is currently coded.
// There is no IN token if the endpoint is not armed!
// No IN handler is required nor a state machine as there is no need for a ZLP

/*
if (0) { // Response Available Notification
        // Not TODO: Probably never implement this, we're not a modem.
        // Is this correct placement of the response notification?
        bdp->BDADDR[USB_bmRequestType]	= USB_bmRequestType_D2H | USB_bmRequestType_Class | USB_bmRequestType_Interface;
        bdp->BDADDR[USB_bRequest]		= CDC_RESPONSE_AVAILABLE;
        bdp->BDADDR[USB_wValue]			= 0;
        bdp->BDADDR[USB_wValueHigh]		= 0;
        bdp->BDADDR[USB_wIndex]			= 0;
        bdp->BDADDR[USB_wIndexHigh]		= 0;
        bdp->BDADDR[USB_wLength]		= 0;
        bdp->BDADDR[USB_wLengthHigh]	= 0;
        // JTR past below bdp->BDCNT = 8;
        usb_ack_dat1(bdp, 8);
} else if (0) {	// Network Connection Notification
} else if (0) {	// Serial State Notification
}
} */

/* END OF CDC CLASS REQUESTS HANDLING

 Below are the CDC USBUART functions */

//void sof_counter_handler() {
//    if (SOFCOUNT == 0) return;
//    SOFCOUNT--;
//}

/**********************************************************************************
  Function:
        BYTE WaitOutReady()

  // JTR2 addition
  Summary:
    It is a semi-blocking function. It waits for the cdc Out Buffer to
    become available before returning. However if the USB interrupt is
    not enabled it call the fast USB housekeeper to ensure that the USB
    stack is always serviced in the meantime. If in polling mode and a
    setup packet or bus reset happens it returns FALSE (00) to alert the calling
    function that the transfer may have been interfered with.


 /*****************************************************************************/

void cdc_register_startup_handler(usb_handler_t startup_handler)
{
  cdc_startup_handler = startup_handler;
}

void cdc_register_reset_handler(usb_handler_t reset_handler)
{
  cdc_reset_handler = reset_handler;
}

void cdc_register_data_received(usb_handler_t received_handler)
{
  cdc_received_handler = received_handler;
}

void cdc_register_data_transmited(usb_handler_t transmited_handler)
{
  cdc_transmited_handler = transmited_handler;
}

bool IsCDCStreamReady(void)
{
  return usb_get_device_state() == CONFIGURED_STATE;
}

static inline SW_BD_ENTRY* check_in_bdps(unsigned int ppi)
{
  if (!(InbdpA->PHY_DB_ENTRY->BDSTAT & UOWN) && !ppi )
	return InbdpA;
  if (!(InbdpB->PHY_DB_ENTRY->BDSTAT & UOWN) && ppi )
	return InbdpB;
  return NULL;
}

static inline SW_BD_ENTRY* check_out_bdps(void)
{
  if (!(OutbdpA->PHY_DB_ENTRY->BDSTAT & UOWN) && (OutbdpA->PHY_DB_ENTRY->BDSTAT & DATA1) == cdc_out_dts_flag )
	return OutbdpA;
  if (!(OutbdpB->PHY_DB_ENTRY->BDSTAT & UOWN) && (OutbdpB->PHY_DB_ENTRY->BDSTAT & DATA1) == cdc_out_dts_flag  )
	return OutbdpB;
  return NULL;
}

static void cdc_data_out_handler(SW_BD_ENTRY* bdp)
{
  if (cdc_received_handler)
	cdc_received_handler();
}

static void cdc_data_in_handler(SW_BD_ENTRY* bdp)
{
  SW_BD_ENTRY* bdpB = check_in_bdps(cdc_in_ppi);

  if (bdpB)
  {
	if (usbinbuf.cnt > 0)
	{
	  int c = usbinbuf.cnt <= CDC_BUFFER_SIZE ? usbinbuf.cnt : CDC_BUFFER_SIZE;
	  memcpy(bdpB->BDADDR, usbinbuf.inBufPtr + usbinbuf.rdptr, c);
	  bdpB->PHY_DB_ENTRY->BDCNT = c;

	  usbinbuf.cnt -= c;
	  usbinbuf.rdptr += c;


	  bdpB->PHY_DB_ENTRY->BDSTAT = cdc_in_dts_flag | DTSEN | UOWN;

	  cdc_in_dts_flag = (~cdc_in_dts_flag) & DATA1;
	  cdc_in_ppi = (~cdc_in_ppi) & 1;
	}
	else if (cdc_transmited_handler)
	{
	  cdc_transmited_handler();
	  return;
	}
		
	if (usbinbuf.cnt > 0)
	{
	  bdpB = check_in_bdps(cdc_in_ppi);
	  if (bdpB)
	  {
		int c = usbinbuf.cnt <= CDC_BUFFER_SIZE ? usbinbuf.cnt : CDC_BUFFER_SIZE;
		memcpy(bdp->BDADDR, usbinbuf.inBufPtr + usbinbuf.rdptr, c);
		bdp->PHY_DB_ENTRY->BDCNT = c;

		usbinbuf.cnt -= c;
		usbinbuf.rdptr += c;


		bdp->PHY_DB_ENTRY->BDSTAT = cdc_in_dts_flag | DTSEN | UOWN;

		cdc_in_dts_flag = (~cdc_in_dts_flag) & DATA1;
		cdc_in_ppi = (~cdc_in_ppi) & 1;
	  }
	}
  }
}

static SW_BD_ENTRY* WaitForInBD(usb_handler_t wait_handler)
{
  SW_BD_ENTRY* inbdp = check_in_bdps(cdc_in_ppi);

  while (!inbdp)
  {
	if (wait_handler)
	  wait_handler();

	if (usb_get_device_state() != CONFIGURED_STATE)
	  return NULL;

	inbdp = check_in_bdps(cdc_in_ppi);
  }

  return inbdp;
}

int writeCDCarray(const void* data, int sz, usb_handler_t wait_handler)
{
  SW_BD_ENTRY* inbdp = NULL;

  if (IsCDCStreamReady() && InbdpA && InbdpB)
  {
	while(!usbinbuf.complete_flag)
	{
	  if (wait_handler)
		wait_handler();
	  
	  if (usb_get_device_state() != CONFIGURED_STATE)
		return -1;
	}

	inbdp = WaitForInBD(wait_handler);

	if (inbdp == NULL)
	  return -1;

	if (sz <= CDC_BUFFER_SIZE)
	{
	  if (sz)
	    memcpy(inbdp->BDADDR, data, sz);
	  
	  inbdp->PHY_DB_ENTRY->BDCNT = sz;

	  inbdp->PHY_DB_ENTRY->BDSTAT = cdc_in_dts_flag | DTSEN | UOWN;

	  cdc_in_dts_flag = (~cdc_in_dts_flag) & DATA1;
	  cdc_in_ppi = (~cdc_in_ppi) & 1;

      return sz;
	}
	else
	{
	  usbinbuf.inBufPtr = data;
	  usbinbuf.cnt = sz;
	  usbinbuf.rdptr = 0;
	  usbinbuf.complete_flag = false;

	  memcpy(inbdp->BDADDR, usbinbuf.inBufPtr, CDC_BUFFER_SIZE);
	  inbdp->PHY_DB_ENTRY->BDCNT = CDC_BUFFER_SIZE;

	  usbinbuf.cnt -= CDC_BUFFER_SIZE;
	  usbinbuf.rdptr += CDC_BUFFER_SIZE;

	  inbdp->PHY_DB_ENTRY->BDSTAT = cdc_in_dts_flag | DTSEN | UOWN;

	  cdc_in_dts_flag = (~cdc_in_dts_flag) & DATA1;
	  cdc_in_ppi = (~cdc_in_ppi) & 1;
	  
	  while(!usbinbuf.complete_flag)
	  {
		if (wait_handler)
		  wait_handler();
		
		if (usb_get_device_state() != CONFIGURED_STATE)
		  return -1;
	  }

	  return sz;
	}
  }
  return -1;
}

int readCDCarray(void* data, int sz, usb_handler_t wait_handler)
{
  SW_BD_ENTRY* outbdp = NULL;
  int c = 0;

  if (usboutbuf.cnt > 0)
  {
	c = sz <= usboutbuf.cnt ? sz : usboutbuf.cnt;
	memcpy(data, (const void*)&usboutbuf.outBuf[usboutbuf.rdptr], c);

	usboutbuf.cnt -= c;
	usboutbuf.rdptr += c;

	if (c == sz)
	  return sz;

	sz -= c;
  }

  if (IsCDCStreamReady() && OutbdpA && OutbdpB)
  {
	outbdp = check_out_bdps();

	while(!outbdp)
	{
	  if (wait_handler)
	    wait_handler();

	  if (usb_get_device_state() != CONFIGURED_STATE)
		return -1;

	  outbdp = check_out_bdps();
	}

	if (sz >= outbdp->PHY_DB_ENTRY->BDCNT)
	{
	  if (outbdp->PHY_DB_ENTRY->BDCNT > 0)
	    memcpy(data, outbdp->BDADDR, outbdp->PHY_DB_ENTRY->BDCNT);
	  sz = outbdp->PHY_DB_ENTRY->BDCNT;

	  cdc_out_dts_flag = (~outbdp->PHY_DB_ENTRY->BDSTAT) & DATA1;

	  outbdp->PHY_DB_ENTRY->BDCNT = CDC_BUFFER_SIZE;
	  outbdp->PHY_DB_ENTRY->BDSTAT = (outbdp->PHY_DB_ENTRY->BDSTAT & DATA1) | DTSEN | UOWN;
	  
	  return sz;
	}
	else
	{
	  if (outbdp->PHY_DB_ENTRY->BDCNT > 0)
	    memcpy((void*)usboutbuf.outBuf, (const void*)outbdp->BDADDR, outbdp->PHY_DB_ENTRY->BDCNT);

	  usboutbuf.cnt = outbdp->PHY_DB_ENTRY->BDCNT;
	  usboutbuf.rdptr = 0;

	  cdc_out_dts_flag = (~outbdp->PHY_DB_ENTRY->BDSTAT) & DATA1;

	  outbdp->PHY_DB_ENTRY->BDCNT = CDC_BUFFER_SIZE;
	  outbdp->PHY_DB_ENTRY->BDSTAT = (outbdp->PHY_DB_ENTRY->BDSTAT & DATA1) | DTSEN | UOWN;

	  if (usboutbuf.cnt > 0)
		memcpy(data, (const void*)&usboutbuf.outBuf[usboutbuf.rdptr], sz);

	  usboutbuf.rdptr += sz;
	  usboutbuf.cnt -= sz;

	  return sz;
	}
  }
  else if (c > 0)
	return c;

  return -1;
}