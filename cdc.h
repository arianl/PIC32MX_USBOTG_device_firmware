#ifndef __CDC_H__
#define __CDC_H__

#include <xc.h>
#include <GenericTypeDefs.h>

#include "usbotg.h"

/* Set the baud rate configuration bits for enhanced USART. */
void initCDC(void);
bool IsCDCStreamReady(void);
int writeCDCarray(const void* data, int sz, usb_handler_t wait_handler);
int readCDCarray(void* data, int sz, usb_handler_t wait_handler);
void cdc_register_startup_handler(usb_handler_t startup_handler);
void cdc_register_reset_handler(usb_handler_t reset_handler);
void cdc_register_data_received(usb_handler_t received_handler);
void cdc_register_data_transmited(usb_handler_t transmited_handler);


static void cdc_setup(SW_BD_ENTRY*);
static void cdc_set_line_coding_data(SW_BD_ENTRY*);
static int user_configured_init(int, SW_BD_ENTRY*, unsigned int*); // JTR added. Sets up CDC endpoints after device configured.
static void cdc_data_in_handler(SW_BD_ENTRY*);
static void cdc_data_out_handler(SW_BD_ENTRY*);



// CDC Request Codes
#define CDC_SEND_ENCAPSULATED_COMMAND                                           0x00
#define CDC_GET_ENCAPSULATED_RESPONSE                                           0x01
#define CDC_SET_COMM_FEATURE                                                    0x02
#define CDC_GET_COMM_FEATURE                                                    0x03
#define CDC_CLEAR_COMM_FEATURE                                                  0x04
//              RESERVED (future use)                                           0x05-0x0F
#define CDC_SET_AUX_LINE_STATE                                                  0x10
#define CDC_SET_HOOK_STATE                                                      0x11
#define CDC_PULSE_SETUP                                                         0x12
#define CDC_SEND_PULSE                                                          0x13
#define CDC_SET_PULSE_TIME                                                      0x14
#define CDC_RING_AUX_JACK                                                       0x15
//              RESERVED (future use)                                           0x16-0x1F
#define CDC_SET_LINE_CODING                                                     0x20
#define CDC_GET_LINE_CODING                                                     0x21
#define CDC_SET_CONTROL_LINE_STATE                                              0x22
#define CDC_SEND_BREAK                                                          0x23
//              RESERVED (future use)                                           0x24-0x2F
#define CDC_SET_RINGER_PARMS                                                    0x30
#define CDC_GET_RINGER_PARMS                                                    0x31
#define CDC_SET_OPERATION_PARMS                                                 0x32
#define CDC_GET_OPERATION_PARMS                                                 0x33
#define CDC_SET_LINE_PARMS                                                      0x34
#define CDC_GET_LINE_PARMS                                                      0x35
#define CDC_DIAL_DIGITS                                                         0x36
#define CDC_SET_UNIT_PARAMETER                                                  0x37
#define CDC_GET_UNIT_PARAMETER                                                  0x38
#define CDC_CLEAR_UNIT_PARAMETER                                                0x39
#define CDC_GET_PROFILE                                                         0x3A
//              RESERVED (future use)                                           0x3B-0x3F
#define CDC_SET_ETHERNET_MULTICAST_FILTERS                                      0x40
#define CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER                        0x41
#define CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER                        0x42
#define CDC_SET_ETHERNET_PACKET_FILTER                                          0x43
#define CDC_GET_ETHERNET_STATISTIC                                              0x44
//              RESERVED (future use)                                           0x45-0x4F
#define CDC_SET_ATM_DATA_FORMAT                                                 0x50
#define CDC_GET_ATM_DEVICE_STATISTICS                                           0x51
#define CDC_SET_ATM_DEFAULT_VC                                                  0x52
#define CDC_GET_ATM_VC_STATISTICS                                               0x53
//              RESERVED (future use)                                           0x54-0x5F
//              MDLM Semantic-Model specific Requests                           0x60–0x7F
//              RESERVED (future use)                                           0x80-0xFF

// CDC Notification Codes
#define CDC_NETWORK_CONNECTION                                                  0x00
#define CDC_RESPONSE_AVAILABLE                                                  0x01
//              RESERVED (future use)                                           0x02-0x07
#define CDC_AUX_JACK_HOOK_STATE                                                 0x08
#define CDC_RING_DETECT                                                         0x09
//              RESERVED (future use)                                           0x0A-0x1F
#define CDC_SERIAL_STATE                                                        0x20
//              RESERVED (future use)                                           0x21-0x27
#define CDC_CALL_STATE_CHANGE                                                   0x28
#define CDC_LINE_STATE_CHANGE                                                   0x29
#define CDC_CONNECTION_SPEED_CHANGE                                             0x2A
//              RESERVED                                                        0x2B-0x3F
//              MDML SEMANTIC-MODEL-SPECIFIC NOTIFICATION                       0x40-0x5F
//              RESERVED (future use)                                           0x60-0xFF
#endif
