#include <xc.h>
#include <plib.h>
#include <GenericTypeDefs.h>
#include <string.h>

#include "cdc.h"
#include "usbotg.h"
#include "pic32mxusbotg.h"
#include "cdc_stream.h"

#include "../TNKernel-PIC32-master/tn.h"

extern TN_EVENT* event_USB_CDC_IN;
extern TN_EVENT* event_USB_CDC_OUT;

static void usb_in_wait_handler(void)
{
  unsigned int pattern = 0;
  tn_event_wait(event_USB_CDC_IN, 1, TN_EVENT_WCOND_OR, &pattern, TN_WAIT_INFINITE);
  tn_event_clear(event_USB_CDC_IN, 0);
}

static void usb_out_wait_handler(void)
{
  unsigned int pattern = 0;
  tn_event_wait(event_USB_CDC_OUT, 1, TN_EVENT_WCOND_OR, &pattern, TN_WAIT_INFINITE);
  tn_event_clear(event_USB_CDC_OUT, 0);
}

int readCDC(void* data, int sz)
{
  return readCDCarray(data, sz, usb_out_wait_handler);
}

int writeCDC(const void* data, int sz)
{
  return writeCDCarray(data, sz, usb_in_wait_handler);
}