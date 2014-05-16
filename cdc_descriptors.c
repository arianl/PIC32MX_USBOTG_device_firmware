#include "cdc_descriptors.h"

 const unsigned char const cdc_device_descriptor[] = {
        0x12,                                                           // bLength
        USB_DEVICE_DESCRIPTOR_TYPE,                                    // bDescriptorType
        0x0,                                                           // bcdUSB (low byte)
        0x2,                                                           // bcdUSB (high byte)
        0x02,                                                           // bDeviceClass
        0x00,                                                           // bDeviceSubClass Abstract Control Model
        0x00,                                                           // bDeviceProtocol
        USB_EP0_BUFFER_SIZE,                                    // bMaxPacketSize
        LOWB(USB_VID),                                          // idVendor (low byte)
        HIGHB(USB_VID),                                         // idVendor (high byte)
        LOWB(USB_PID),                                          // idProduct (low byte)
        HIGHB(USB_PID),                                         // idProduct (high byte)
        LOWB(USB_DEV),                                          // bcdDevice (low byte)
        HIGHB(USB_DEV),                                         // bcdDevice (high byte)
        USB_iManufacturer,                                      // iManufacturer
        USB_iProduct,                                           // iProduct
        USB_iSerialNum,                                         // iSerialNumber (none)
        USB_NUM_CONFIGURATIONS                          // bNumConfigurations
};

#define USB_CONFIG_DESC_TOT_LENGTH 67
 const unsigned char const cdc_config_descriptor[] = {
        0x09,                                                           // bLength
        USB_CONFIGURATION_DESCRIPTOR_TYPE,                              // bDescriptorType
        LOWB(USB_CONFIG_DESC_TOT_LENGTH),                               // wTotalLength (low byte), TODO: Automatic calculation - sizeof doesn't work here
        HIGHB(USB_CONFIG_DESC_TOT_LENGTH),                              // wTotalLength (high byte)
        USB_NUM_INTERFACES,                                             // bNumInterfaces
        0x01,                                                           // bConfigurationValue
        0x00,                                                           // iConfiguration (0=none)
        0x40,                                                           // bmAttributes (0x80 = bus powered)
        0x64,                                                           // bMaxPower (in 2 mA units, 50=100 mA)
//Interface0 descriptor starts here
        0x09,                                                           // bLength (Interface0 descriptor starts here)
        USB_INTERFACE_DESCRIPTOR_TYPE,                                  // bDescriptorType
        0x00,                                                           // bInterfaceNumber
        0x00,                                                           // bAlternateSetting
        0x01,                                                           // bNumEndpoints (excluding EP0)
        COMMUNICATIONS_INTERFACE_CLASS,                                 //0x02=com interface
        INTERFACE_SUBCLASS_ABSTRACT_CONTROL_MODEL,
        0x01,    // 01h AT Commands: [V250] etc                         // Communications Interface Class Control Protocol
        0x00,      		                                                // Index of String Descriptor Describing this interface

		0x05,
		CS_INTERFACE,                                                   // bDescriptorType
		HEADER_FUNCTIONAL_DESCRIPTOR,
		LOWB(0x0110),                                                   // USB Class Definitions for Communications Devices Specification release number in binary-coded decimal
        HIGHB(0x0110),                                                  //

		0x04,                                                           // bFunctionLength
        CS_INTERFACE,                                                   // bDescriptorType
        ABSTRACT_CONTROL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR,              // bDescriptorSubtype (CDC abstract control management descriptor)
        0x02,                                                           // bmCapabilities
		
		0x05,                                                           // bFunctionLength
        CS_INTERFACE,                                                   // bDescriptorType
        UNION_FUNCTIONAL_DESCRIPTOR,                                    // bDescriptorSubtype (CDC union descriptor)
        0x00,                                                           // bControlInterface
        0x01,                                                           // bSubordinateInterface0

		0x05,                                                           // bFunctionLength
        CS_INTERFACE,                                                   // bDescriptorType
        CALL_MANAGEMENT_FUNCTIONAL_DESCRIPTOR,                          // bDescriptorSubtype (Call management descriptor)
        0x03,                                                           // The capabilities that this configuration supports:
                                                                        // - Device handles call management itself.
                                                                        // - Device can send/receive call management information over a Data Class interface.
        0x01,

		0x07,                                                           // bLength (Endpoint1 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                                   // bDescriptorType
        0x81,                                                           // bEndpointAddress ep = 1 direction IN
        0x03,                                                           // bmAttributes (0x03=intr)
        LOWB(CDC_NOTICE_BUFFER_SIZE),                                   // wMaxPacketSize (low byte)
        HIGHB(CDC_NOTICE_BUFFER_SIZE),                                  // wMaxPacketSize (high byte)
        0x02,                                                           // bInterval
//Interface1 descriptor
        0x09,                                                           // bLength (Interface1 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,          // bDescriptorType
        0x01,                                                           // bInterfaceNumber
        0x00,                                                           // bAlternateSetting
        0x02,                                                           // bNumEndpoints
        0x0A, //0x0a Base Class 0Ah (CDC-Data)                                  // bInterfaceClass
        0x00,                                                           // bInterfaceSubClass
        0x00,                                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=functional unit, 0xFF=vendor specific)
        0x00,                                                           // iInterface

        0x07,                                                           // bLength (Enpoint2 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                                   // bDescriptorType
        0x02,                                                           // bEndpointAddress (ep = 2 direction = OUT)
        0x02,                                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                                         // wMaxPacketSize (high byte)
        0x00,                                                           // bInterval

        0x07,                                                           // bLength
        USB_ENDPOINT_DESCRIPTOR_TYPE,                                   // bDescriptorType
        0x82,                                                           // bEndpointAddress (ep = 2 direction = IN)
        0x02,                                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                                         // wMaxPacketSize (high byte)
        0x00                                                            // bInterval
};

 const unsigned char const cdc_str_descs[] = {
        4, USB_STRING_DESCRIPTOR_TYPE, LOWB(USB_LANGID_English_United_States), HIGHB(USB_LANGID_English_United_States),
        12, USB_STRING_DESCRIPTOR_TYPE, 'L',0,'Y',0,'2',0,'A',0,'R',0,
        32, USB_STRING_DESCRIPTOR_TYPE, 'P',0,'I',0,'C',0,' ',0,'s',0,'e', 0, 'r',0,'r',0,'a',0,'l',0,' ',0,'p',0,'o',0,'r',0,'t',0,
        18, USB_STRING_DESCRIPTOR_TYPE, '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'4',0
};

const unsigned char const cdc_dev_qualifier_descs[] = {
  10,
  USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
  1,
  1,
  2,
  0,
  0,
  8,
  0,
  0
};

