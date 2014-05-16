/*
This work is licensed under the Creative Commons Attribution 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/
or send a letter to
        Creative Commons,
        171 Second Street,
        Suite 300,
        San Francisco,
        California,
        94105,
        USA.
*/

#ifndef __CDC_DESCRIPTORS_H__
#define __CDC_DESCRIPTORS_H__

// JTR v0.1a

#include "usbotg.h"

/* String identifiers */
#define USB_iManufacturer               1u
#define USB_iProduct                    2u
#define USB_iSerialNum                  3u

#define USB_NUM_CONFIGURATIONS          1u
#define USB_NUM_INTERFACES              2u
#define CDC_BUFFER_SIZE                 64u
#define CDC_NOTICE_BUFFER_SIZE          16u


#define USB_VID                         0x04D8 //USB-IF, Inc. has licensed USB Vendor ID No. 0x04D8 (?VID?) to Microchip Technology Inc. (?Microchip?).
#define USB_PID                         0XFD00
#define USB_DEV                         0x0001

#define COMMUNICATIONS_INTERFACE_CLASS                        0x02

#define INTERFACE_SUBCLASS_ABSTRACT_CONTROL_MODEL             0x02

#define CS_INTERFACE                                          0x24

#define HEADER_FUNCTIONAL_DESCRIPTOR                          0x00 //which marks the beginning of the concatenated set of functional descriptors for the interface.
#define CALL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR                 0x01
#define ABSTRACT_CONTROL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR     0x02
#define DIRECT_LINE_MANAGEMENT_FUNCTIONAL_DESCRIPTOR          0x03
#define TELEPHONE_RINGER_FUNCTIONAL_DESCRIPTOR                0x04
#define TEL_CALL_AND_LINE_STATE_REP_CAP_FUNCTIONAL_DESCRIPTOR 0x05
#define UNION_FUNCTIONAL_DESCRIPTOR                           0x06
#define COUNTRY_SELECTION_FUNCTIONAL_DESCRIPTOR               0x07
#define TELEPHONE_OPERATIONAL_MODES_FUNCTIONAL_DESCRIPTOR     0x08
#define USB_TERMINAL_FUNCTIONAL_DESCRIPTOR                    0x09
#define NETWORK_CHANNEL_TERMINAL_DESCRIPTOR                   0x0A
#define PROTOCOL_UNIT_FUNCTIONAL_DESCRIPTOR                   0x0B
#define EXTENSION_UNIT_FUNCTIONAL_DESCRIPTOR                  0x0C
#define MULTI_CHANNEL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR        0x0D
#define CAPI_CONTROL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR         0x0E
#define ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR             0x0F
#define ATM_NETWORKING_FUNCTIONAL_DESCRIPTOR                  0x10
#define WIRELESS_HANDSET_CONTROL_MODEL_FUNCTIONAL_DESCRIPTOR  0x11

 extern const unsigned char const cdc_device_descriptor[];
 extern const unsigned char const cdc_config_descriptor[];
 extern const unsigned char const cdc_str_descs[];
 extern const unsigned char const cdc_dev_qualifier_descs[];

#endif 
