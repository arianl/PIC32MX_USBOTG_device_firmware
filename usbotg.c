#include <xc.h>
#include <GenericTypeDefs.h>
#include <stdlib.h>
#include <strings.h>

#include "usbotg.h"
#include "pic32mxusbotg.h"

BD_ENTRY __attribute__((aligned (512) )) USB_BDT[MAX_CHIP_EP*4] = {{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},
{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL},{0,0, NULL}};

static SW_BD_ENTRY SW_USB_BDT[MAX_CHIP_EP*4];

static BYTE ep0_buff_InA[USB_EP0_BUFFER_SIZE];
static BYTE ep0_buff_InB[USB_EP0_BUFFER_SIZE];
static BYTE ep0_buff_OutA[USB_EP0_BUFFER_SIZE];
static BYTE ep0_buff_OutB[USB_EP0_BUFFER_SIZE];

static SW_BD_ENTRY *EP0_Inbdp; // Dito

static usb_ep_t endpoints[MAX_CHIP_EP];

static const BYTE *usb_device_descriptor;
static const BYTE *usb_config_descriptor;
static const BYTE *usb_string_descriptor;
static const BYTE *usb_device_qualifier_descriptor;
static int usb_num_string_descriptors;

static usb_handler_t sof_handler = NULL;
static usb_trn_handler_t class_setup_handler = NULL, vendor_setup_handler = NULL;
static usb_handler_t device_suspend_handler = NULL, device_resume_handler = NULL;
static usb_handler_t device_error_handler = NULL;
static usb_handler_set_device_config_t device_set_config_handler  = NULL;

static volatile USB_DEVICE_STATE usb_device_state;
static unsigned int usb_addr_pending;
static unsigned int usb_is_suspended = 0;
static volatile unsigned int usbrequesterrorflag = 0;
static const BYTE* usb_desc_ptr = NULL;
static unsigned int usb_desc_len = 0;
static unsigned int usb_device_status = 0;
static unsigned int usb_config_num = 0;
static unsigned int usb_config_id = 0;
static unsigned int usb_interface_num = 0;
static unsigned int EP0_in_dts_flag = 0;
static unsigned int EP0_in_ppi = 0;
static unsigned int usb_set_configuration_request = 0;
static volatile unsigned int usb_trn_status = 0;

void usb_init(const BYTE *device_descriptor,
              const BYTE *config_descriptor,
              const BYTE *string_descriptor,
			  const BYTE *device_qualifier_descriptor,
              int num_string_descriptors,
			  unsigned int status) {

    usb_device_descriptor = device_descriptor;
    usb_config_descriptor = config_descriptor;
    usb_string_descriptor = string_descriptor;
	usb_device_qualifier_descriptor = device_qualifier_descriptor;
    usb_num_string_descriptors = num_string_descriptors;
	usb_config_num = usb_device_descriptor ? usb_device_descriptor[17] : 0;
	usb_device_status = status;
    sof_handler = NULL;
    class_setup_handler = NULL;
    vendor_setup_handler = NULL;
    usb_unset_in_handler(0);
    usb_unset_out_handler(0);
    ClearUSBtoDefault();
}

volatile USB_DEVICE_STATE usb_get_device_state(void)
{
  return usb_device_state;
}

void usb_start(void) {
    EnableUsb(); // Enable USB-hardware
    ConfigureUsbHardware(USB_BDT);
	ResetPPbuffers();
    EnablePacketTransfer();
    
    usb_device_state = DETACHED_STATE;
	if (usb_device_status & USB_DEVICE_SELF_POWERED)
	  usb_device_state = POWERED_STATE;
//	{
//	  while (SingleEndedZeroIsSet()); // Busywait for initial power-up
//	  usb_device_state = ATTACHED_STATE; //JTR2
//	}
}

static void usb_handle_error(void) {
  if (device_error_handler)
	device_error_handler();
    ClearAllUsbErrorInterruptFlags();
}

static void usb_handle_reset(void) {
	if (device_set_config_handler)
	  device_set_config_handler(0, NULL, NULL);
	usb_interface_num = 0;
	usb_addr_pending = 0x00;
    ClearUSBtoDefault();
    EnablePacketTransfer();
	usb_device_state = DEFAULT_STATE;
}


static void ClearUSBtoDefault(void) {
    int i;
	usb_device_state = DETACHED_STATE;

    SetUsbAddress(0); // After reset we don't have an address
    ResetPPbuffers();
    ClearAllUsbErrorInterruptFlags();

    for (i = 0; i < MAX_CHIP_EP; i++) {
        endpoints[i].out_handler = NULL;
        endpoints[i].in_handler = NULL;
    }

    for (i = 0; i < (2 * 2 * MAX_CHIP_EP); i++) {
        USB_BDT[i].BDSTAT = 0;
		USB_BDT[i].BDCNT = 0;
		USB_BDT[i].PHYBDADDR = NULL;
    }

	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDCNT = USB_EP0_BUFFER_SIZE;
	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDSTAT = DTSEN | UOWN;
	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].PHYBDADDR = (BYTE*)_VirtToPhys(ep0_buff_OutA);
    SW_USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_EVEN)].BDADDR = ep0_buff_OutA;

	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDCNT = USB_EP0_BUFFER_SIZE;
	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDSTAT = DTSEN | UOWN;
	USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].PHYBDADDR = (BYTE*)_VirtToPhys(ep0_buff_OutB);
	SW_USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, USB_PP_ODD)].BDADDR = ep0_buff_OutB;

	USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDCNT = 0;
	USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDSTAT = DTSEN;
	USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].PHYBDADDR = (BYTE*)_VirtToPhys(ep0_buff_InA);
	SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_EVEN)].BDADDR = ep0_buff_InA;

	USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].BDCNT = 0;
	USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].BDSTAT = DTSEN;
    USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].PHYBDADDR = (BYTE*)_VirtToPhys(ep0_buff_InB);
	SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, USB_PP_ODD)].BDADDR = ep0_buff_InB;

// Software BDT	uses virtual addresses
	for (i = 0; i < (2 * 2 * MAX_CHIP_EP); i++) {
        SW_USB_BDT[i].PHY_DB_ENTRY = &USB_BDT[i];
    }

	USB_UEP0 = USB_EP_CONTROL; // Configure Only ep0 At this point.

    usb_addr_pending = 0x00;
	EP0_in_dts_flag = 0;
    EP0_in_ppi = 0;
	usb_config_id = 0;

}

void usb_handler() {

   

    if (TestUsbInterruptFlag(USB_URST)) {
        usb_handle_reset();
        ClearUsbInterruptFlag(USB_URST);
    }

    if (TestUsbInterruptFlag(USB_UERR)) {
        usb_handle_error();
        ClearUsbInterruptFlag(USB_UERR);
    }

	if (TestUsbInterruptFlag(USB_SOF)) {
        /* Start-of-frame */
        if (sof_handler) sof_handler();
        ClearUsbInterruptFlag(USB_SOF);
    }
	
	if (TestUsbInterruptFlag(USB_IDLE)) {
        /* Idle - suspend */
	  if (device_suspend_handler)
		device_suspend_handler();
        ClearUsbInterruptFlag(USB_IDLE);
    }

	if (TestUsbInterruptFlag(USB_RESUME))
	{
	  if (device_resume_handler)
		device_resume_handler();
	  ClearUsbInterruptFlag(USB_RESUME);
	}
    
    if (TestUsbInterruptFlag(USB_STALL)) {
	    //EP0_in_ppi = (~EP0_in_ppi) & 1;
        ClearUsbInterruptFlag(USB_STALL);
    }

	if (TestUsbInterruptFlag(USB_TRN)) {
	    usb_trn_status = GetUsbTransaction();
        usb_handle_transaction();
        ClearUsbInterruptFlag(USB_TRN); // JTR Missing! This is why Ian was only getting one interrupt??
    } // Side effect: advance USTAT Fifo

}

static void usb_handle_transaction(void) {

  	usbrequesterrorflag = 0;

	EP0_Inbdp = NULL;

	SW_BD_ENTRY* ep_Outbd = &SW_USB_BDT[USB_USTAT2BD(usb_trn_status)];

	if (USB_STAT2EP(usb_trn_status) == 0)
	{
	  switch (ep_Outbd->PHY_DB_ENTRY->BDSTAT & USB_TOKEN_Mask) {
		  case USB_TOKEN_SETUP:

			  //EP0_in_ppi = USB_STAT2PPI(usb_trn_status);
              EP0_Inbdp = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, EP0_in_ppi)]; // All replies in IN direction
			  
			  usb_set_configuration_request = 0;

			  EP0_in_dts_flag = DATA1;

			  usb_handle_setup(ep_Outbd);
			  
			  EnablePacketTransfer();

			break;

		  case USB_TOKEN_OUT:

			  //EP0_in_ppi = USB_STAT2PPI(usb_trn_status);
			  
              EP0_Inbdp = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, EP0_in_ppi)]; // All replies in IN direction
			  
			  EP0_in_dts_flag = DATA1;
			  		  
			  usb_handle_out(0, ep_Outbd);
			break;

		  case USB_TOKEN_IN:
			  EP0_in_ppi = (~EP0_in_ppi) & 1;
			  EP0_Inbdp = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, EP0_in_ppi)]; // All replies in IN direction
			  			  
			  usb_handle_in(0, EP0_Inbdp);
			break;

		   default:
			 break;
			  /* Default case of unknown TOKEN - discard */
	  }

	  
    }
	else
	{
	  switch (ep_Outbd->PHY_DB_ENTRY->BDSTAT & USB_TOKEN_Mask) {
		  case USB_TOKEN_OUT:
			  usb_handle_out(USB_STAT2EP(usb_trn_status), ep_Outbd);
			  break;
		  case USB_TOKEN_IN:
			  usb_handle_in(USB_STAT2EP(usb_trn_status), ep_Outbd);
			  break;
	      default:
			  break;
			  /* Default case of unknown TOKEN - discard */
	  }
	}

}

static void usb_handle_setup(SW_BD_ENTRY* dbp) {

    switch (dbp->BDADDR[USB_bmRequestType] & USB_bmRequestType_TypeMask) {
        case USB_bmRequestType_Standard:
            switch (dbp->BDADDR[USB_bmRequestType] & USB_bmRequestType_RecipientMask) {
                case USB_bmRequestType_Device:
                    usb_handle_StandardDeviceRequest(dbp);
                    break;
                case USB_bmRequestType_Interface:
                    usb_handle_StandardInterfaceRequest(dbp);
                    break;
                case USB_bmRequestType_Endpoint:
                    usb_handle_StandardEndpointRequest(dbp);
                    break;
                default:
                    usb_RequestError();
            }
            break;
        case USB_bmRequestType_Class:
            if (class_setup_handler)
			  class_setup_handler(dbp);
            break;
        case USB_bmRequestType_Vendor:
            if (vendor_setup_handler)
			  vendor_setup_handler(dbp);
            break;
        default:
            usb_RequestError();
    }
    /* Prepare endpoint for new reception */

	if (!usb_set_configuration_request)
	{
	  SW_BD_ENTRY* dbpB = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, (~USB_STAT2PPI(usb_trn_status)) & 1)];

      if (endpoints[0].out_handler)
	  {
		dbp->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
        dbp->PHY_DB_ENTRY->BDSTAT = DATA1 | DTSEN | UOWN;

		if (!(dbpB->PHY_DB_ENTRY->BDSTAT & UOWN))
        {
	      dbpB->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
	      dbpB->PHY_DB_ENTRY->BDSTAT = DATA1 | DTSEN | UOWN;
        }
	  }
	  else
	  {
		dbp->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
        dbp->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;

		if (!(dbpB->PHY_DB_ENTRY->BDSTAT & UOWN))
        {
	      dbpB->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
	      dbpB->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;
        }
	  }
	}
}

static void usb_handle_StandardDeviceRequest(SW_BD_ENTRY *bdp) {
    BYTE *packet = bdp->BDADDR;
    int i;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = usb_device_status;
            EP0_Inbdp->BDADDR[1] = 0;
            usb_ack_dat1(2);
            break;
        case USB_REQUEST_CLEAR_FEATURE:
	          if (USB_DEVICE_FEATURE_WAKEUP == packet[USB_wValue]) {
                usb_device_status &= ~(USB_DEVICE_FEATURE_WAKEUP);
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_FEATURE:
            if (USB_DEVICE_FEATURE_WAKEUP == packet[USB_wValue]) {
                usb_device_status |= USB_DEVICE_FEATURE_WAKEUP;
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_ADDRESS:
            if (0x00u == packet[USB_wValueHigh] && 0x7Fu >= packet[USB_wValue]) {
                usb_addr_pending = packet[USB_wValue];
				usb_device_state = ADR_PENDING_STATE;
                usb_set_in_handler(0, usb_set_address);
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;


        case USB_REQUEST_GET_DESCRIPTOR:
            switch (packet[USB_bDescriptorType]) {
                case USB_DEVICE_DESCRIPTOR_TYPE: // There is only every one in pratice.

				    usb_desc_ptr = usb_device_descriptor;
                    usb_desc_len = usb_device_descriptor[0];

					if (packet[USB_wLengthHigh] == 0 && packet[USB_wLength] < usb_device_descriptor[0])
                        usb_desc_len = packet[USB_wLength]; // If the HOST asked for LESS then must adjust count to the smaller number
                    break;

                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    if (packet[USB_bDescriptorIndex] >= usb_config_num) {
                        flag_usb_RequestError();
                        break;
                    }

                    usb_desc_ptr = usb_config_descriptor;
                    usb_desc_len = ((unsigned int)usb_config_descriptor[2]) | (((unsigned int)usb_config_descriptor[3]) << 8); // Get WORD length from descriptor always at bytes 2&3 (Low-High)

                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_desc_ptr += usb_desc_len;
                        usb_desc_len = ((unsigned int)usb_desc_ptr[2]) | (((unsigned int)usb_desc_ptr[3]) << 8); // Get (next) WORD length from descriptor always at bytes 2&3 (Low-High)
                    }

                    if ((packet[USB_wLengthHigh] < usb_desc_ptr[3]) ||
                        (packet[USB_wLengthHigh] == usb_desc_ptr[3] && packet[USB_wLength] < usb_desc_ptr[2]))
                        usb_desc_len = ((unsigned int)packet[USB_wLength]) | (((unsigned int)packet[USB_wLengthHigh]) << 8); // If the HOST asked for LESS then must adjust count to the smaller number

                    break;

                case USB_STRING_DESCRIPTOR_TYPE:
                    // TODO: Handle language request. For now return standard language.
                    if (packet[USB_bDescriptorIndex] >= usb_num_string_descriptors) {
                        flag_usb_RequestError();
                        break;
                    }

                    usb_desc_ptr = usb_string_descriptor;
                    usb_desc_len = usb_desc_ptr[0]; // Get BYTE length from descriptor always at byte [0]

                    for (i = 0; i < packet[USB_bDescriptorIndex]; i++) { // Implicit linked list traversal until requested configuration
                        usb_desc_ptr += usb_desc_len;
                        usb_desc_len = usb_desc_ptr[0];
                    }

                    if ((0 == packet[USB_wLengthHigh] && packet[USB_wLength] < usb_desc_ptr[0]))
                        usb_desc_len = packet[USB_wLength];

                    break;

                case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
				  if (usb_device_qualifier_descriptor)
				  {
					usb_desc_ptr = usb_device_qualifier_descriptor;
                    usb_desc_len = usb_device_qualifier_descriptor[0];

					if (packet[USB_wLengthHigh] == 0 && packet[USB_wLength] < usb_device_qualifier_descriptor[0])
                        usb_desc_len = packet[USB_wLength]; 
				  }
				  else
                    flag_usb_RequestError();
                  break;

                case USB_INTERFACE_DESCRIPTOR_TYPE:
	            case USB_ENDPOINT_DESCRIPTOR_TYPE:
				case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE:
				case USB_INTERFACE_POWER_DESCRIPTOR_TYPE:
				case USB_OTG_DESCRIPTOR_TYPE:
				case USB_DEBUG_DESCRIPTOR_TYPE:
				case USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE:
                default:
                    flag_usb_RequestError();
            }

            if (usbrequesterrorflag == 0)
			{
              if (usb_desc_len >= USB_EP0_BUFFER_SIZE)
			  {
				usb_set_in_handler(0, usb_send_desc);
			  	usb_set_out_handler(0, usb_out_control_status);
			  }

			  usb_send_desc(EP0_Inbdp); // Send first part of packet right away, the rest is handled by the EP0 IN handler.
            } 
			else
              usb_RequestError();

			break;

        case USB_REQUEST_GET_CONFIGURATION:
            EP0_Inbdp->BDADDR[0] = usb_config_id;
            usb_ack_dat1(1);
            break;

        case USB_REQUEST_SET_CONFIGURATION:

			if (packet[USB_wValue] != 0) {
			  if (device_set_config_handler && device_set_config_handler(packet[USB_wValue], SW_USB_BDT, &usb_interface_num))
			  {
				usb_set_configuration_request = true;

				usb_config_id = packet[USB_wValue];
				usb_device_state = CONFIGURED_STATE;
				
				usb_unset_out_handler(0);
                usb_unset_in_handler(0);

				ResetPPbuffers();
	            EP0_in_ppi = 0;

				EP0_Inbdp = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_IN, EP0_in_ppi)];

				usb_ack_dat1(0);

				SW_BD_ENTRY* dbpB = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, (~USB_STAT2PPI(usb_trn_status)) & 1)];

				if (!(dbpB->PHY_DB_ENTRY->BDSTAT & UOWN))
				{
				  dbpB->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
				  dbpB->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;
				}

				bdp->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
				bdp->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;
			  }
			  else
				usb_RequestError();

			} else {
				usb_device_state = ADDRESS_STATE;
				usb_ack_dat1(0);
			}
            break;

        case USB_REQUEST_SET_DESCRIPTOR:
        default:
            usb_RequestError();
    }
}

static void usb_handle_StandardInterfaceRequest(SW_BD_ENTRY *bdp) {
    BYTE *packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = 0x00;
            EP0_Inbdp->BDADDR[1] = 0x00;
            usb_ack_dat1(2);
            break;
        case USB_REQUEST_GET_INTERFACE:
            if (usb_interface_num > packet[USB_bInterface]) {
                // TODO: Implement alternative interfaces, or move responsibility to class/vendor functions.
                EP0_Inbdp->BDADDR[0] = 0;
                usb_ack_dat1(1);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_SET_INTERFACE:
            if (usb_interface_num > packet[USB_bInterface] && 0u == packet[USB_wValue]) {
                // TODO: Implement alternative interfaces...
                usb_ack_dat1(0);
            } else
                usb_RequestError();
            break;
        case USB_REQUEST_CLEAR_FEATURE: // JTR N/A for interface
        case USB_REQUEST_SET_FEATURE: // This is correct and finished code.
        default:
            usb_RequestError();
    }
}

static void usb_handle_StandardEndpointRequest(SW_BD_ENTRY *bdp) {
    BYTE *packet;
    BYTE epnum;
    BYTE dir;
    SW_BD_ENTRY *epbd;
    usb_uep_t *pUEP;

    packet = bdp->BDADDR;

    switch (packet[USB_bRequest]) {
        case USB_REQUEST_GET_STATUS:
            EP0_Inbdp->BDADDR[0] = 0x00; // Assume no stall
            EP0_Inbdp->BDADDR[1] = 0x00; // Same for stall or not
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &SW_USB_BDT[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            if (epbd->PHY_DB_ENTRY->BDSTAT &= ~BSTALL)
                EP0_Inbdp->BDADDR[0] = 0x01; // EVEN BD is stall flag set?
            
            usb_ack_dat1(2);
            break;

        case USB_REQUEST_CLEAR_FEATURE:
            // As this is really is an application event and there
            // should be a call back and protocol for handling the
            // possible lost of a data packet.
            // TODO: ping-ping support.

            epnum = packet[USB_wIndex] & 0x0F; // JTR Added V0.2 after microchip stuff up with their documentation.
			pUEP = USB_UEP;
            pUEP += epnum;
            *pUEP &= ~USB_UEP_EPSTALL;

            dir = packet[USB_wIndex] >> 7;
            epbd = &SW_USB_BDT[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            epbd->PHY_DB_ENTRY->BDSTAT &= ~BSTALL;
            if (dir) epbd->PHY_DB_ENTRY->BDSTAT |= DATA1; // JTR added IN EP set DTS as it will be toggled to zero next transfer
            if (0 == dir) epbd->PHY_DB_ENTRY->BDSTAT &= ~DATA1; // JTR added

            usb_ack_dat1(0);
            break;


        case USB_REQUEST_SET_FEATURE:
            epnum = packet[USB_wIndex] & 0x0F;
            dir = packet[USB_wIndex] >> 7;
            epbd = &SW_USB_BDT[USB_CALC_BD(epnum, dir, USB_PP_EVEN)];
            epbd->PHY_DB_ENTRY->BDSTAT |= BSTALL;
            
            usb_ack_dat1(0);
            break;
        case USB_REQUEST_SYNCH_FRAME:
        default:
            usb_RequestError();
    }
}

void usb_handle_in(unsigned int epn, SW_BD_ENTRY* dbd) {
    if (endpoints[epn].in_handler) {
        endpoints[epn].in_handler(dbd);
    }
}

void usb_handle_out(unsigned int epn, SW_BD_ENTRY* dbd) {
    if (endpoints[epn].out_handler) {
        endpoints[epn].out_handler(dbd);
    }
}

void usb_register_sof_handler(usb_handler_t handler) {
    sof_handler = handler;
}

void usb_register_class_setup_handler(usb_trn_handler_t handler) {
    class_setup_handler = handler;
}

void usb_register_vendor_setup_handler(usb_trn_handler_t handler) {
    vendor_setup_handler = handler;
}

void usb_set_in_handler(unsigned int ep, usb_trn_handler_t in_handler) {
    endpoints[ep].in_handler = in_handler;
}

void usb_set_out_handler(unsigned int ep, usb_trn_handler_t out_handler) {
    endpoints[ep].out_handler = out_handler;
}

void usb_register_device_suspend_handler(usb_handler_t handler)
{
  device_suspend_handler = handler;
}

void usb_register_device_resume_handler(usb_handler_t handler)
{
  device_resume_handler = handler;
}

void usb_register_device_setconfiguration_handler(usb_handler_set_device_config_t handler)
{
  device_set_config_handler = handler;
}

void usb_register_device_error_handler(usb_handler_t handler)
{
  device_error_handler = handler;
}

// JTR New added helper function use extensively by the standard and class
// request handlers. All status IN packets are DAT1 as is the first DATA packet
// of a IN transfer. Currently with this CDC stack the only IN DATA transfers
// that are > 8 bytes is the descriptor transfer and these are transfered in
// usb_send_rom()

void usb_ack_dat1(int bdcnt) {

    EP0_Inbdp->PHY_DB_ENTRY->BDCNT = bdcnt;
    EP0_Inbdp->PHY_DB_ENTRY->BDSTAT = DATA1 | UOWN | DTSEN;
}

void usb_send_control_data(const void* data, int len)
{
  usb_desc_ptr = data;
  usb_desc_len = len;

  usb_set_in_handler(0, usb_send_desc);

  if (usb_desc_len >= USB_EP0_BUFFER_SIZE)
    usb_set_out_handler(0, usb_out_control_status);

  usb_send_desc(EP0_Inbdp);
}

void usb_RequestError(void) {

  usbrequesterrorflag = true;

  usb_unset_in_handler(0);
  usb_unset_out_handler(0);

  EP0_Inbdp->PHY_DB_ENTRY->BDCNT = 0;
  EP0_Inbdp->PHY_DB_ENTRY->BDSTAT = UOWN | BSTALL;
}

static void usb_set_address(SW_BD_ENTRY* bdp) {
    if (0x00u == usb_addr_pending) {
        usb_device_state = DEFAULT_STATE;
    } else {
        usb_device_state = ADDRESS_STATE;
    }
    SetUsbAddress(usb_addr_pending);
    usb_addr_pending = 0xFF;
    usb_unset_in_handler(0); // Unregister handler
}

static void usb_send_desc(SW_BD_ENTRY* bdp) {

    unsigned int i;
    unsigned int packet_len;

	if (usb_desc_len < USB_EP0_BUFFER_SIZE)
	  usb_unset_in_handler(0);

	if (usb_desc_len) {
        packet_len = (usb_desc_len < USB_EP0_BUFFER_SIZE) ? usb_desc_len : USB_EP0_BUFFER_SIZE; // JTR changed from MAX_BUFFER_SIZE

		memcpy(bdp->BDADDR, usb_desc_ptr, packet_len);
	
    } else {
        usb_unset_in_handler(0);
		packet_len = 0;
    }

	bdp->PHY_DB_ENTRY->BDCNT = packet_len; // Packet length always less then 512 on endpoint 0
	bdp->PHY_DB_ENTRY->BDSTAT = EP0_in_dts_flag | UOWN | DTSEN;

	EP0_in_dts_flag = (~EP0_in_dts_flag) & DATA1;

	usb_desc_ptr += packet_len;
	usb_desc_len -= packet_len;
}

void usb_out_control_status(SW_BD_ENTRY* dbp)
{
  usb_unset_out_handler(0);
  usb_unset_in_handler(0);

  SW_BD_ENTRY* dbpB = &SW_USB_BDT[USB_CALC_BD(0, USB_DIR_OUT, (~USB_STAT2PPI(usb_trn_status)) & 1)];

  if (!(dbpB->PHY_DB_ENTRY->BDSTAT & UOWN))
  {
	dbpB->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
	dbpB->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;
  }

  dbp->PHY_DB_ENTRY->BDCNT = USB_EP0_BUFFER_SIZE;
  dbp->PHY_DB_ENTRY->BDSTAT = DTSEN | UOWN;
}

volatile unsigned int usb_get_trn(void)
{
  return usb_trn_status;
}




