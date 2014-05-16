/* 
 * File:   pic32mxusbotg.h
 * Author: Family
 *
 * Created on Sekmadienis, 2014, Sausio 19, 17.24
 */

#ifndef PIC32MXUSBOTG_H
#define	PIC32MXUSBOTG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h>
#include <GenericTypeDefs.h>

#define MAX_CHIP_EP 16 // MAX_CHIP_EP is the number of hardware endpoints on the silicon. See picusb.h
#define USB_EP0_BUFFER_SIZE 8

    typedef  struct _BD_ENTRY
    {
        __attribute__((packed))
        volatile WORD BDSTAT;
        __attribute__((packed))
        volatile WORD BDCNT;
        __attribute__((packed))
        unsigned char *PHYBDADDR;
    }  BD_ENTRY;

    typedef  struct _SW_BD_ENTRY
    {
        BD_ENTRY* PHY_DB_ENTRY;
        unsigned char *BDADDR;
    }  SW_BD_ENTRY;

#define _VirtToPhys(p) ((unsigned int)p >= 0x80000000L ? (void*)((unsigned int)p & 0x1FFFFFFFL) : (void*)((unsigned char*)p+0x40000000L))


#define UOWN    0x80
#define DATA1   0x40
#define KEN     0x20
#define NINC    0x10
#define DTSEN   0x08
#define BSTALL  0x04


#define USB_DIR_OUT     0
#define USB_DIR_IN      1
#define USB_PP_EVEN     0
#define USB_PP_ODD      1

/* Bitmasks */
#define USB_UEP_EPHSHK          (0x01)
#define USB_UEP_EPSTALL         (0x02)
#define USB_UEP_EPTXEN          (0x04)
#define USB_UEP_EPRXEN          (0x08)
#define USB_UEP_EPCONDIS        (0x10)
#define USB_UEP_RETRYDIS        (0x40)
#define USB_UEP_LSPD            (0x80)


#define USB_EP_INOUT            (USB_UEP_EPHSHK | USB_UEP_EPRXEN | USB_UEP_EPTXEN | USB_UEP_EPCONDIS)
#define USB_EP_CONTROL          (USB_UEP_EPHSHK | USB_UEP_EPRXEN | USB_UEP_EPTXEN)
#define USB_EP_OUT              (USB_UEP_EPHSHK | USB_UEP_EPTXEN | USB_UEP_EPCONDIS)
#define USB_EP_IN               (USB_UEP_EPHSHK | USB_UEP_EPRXEN  | USB_UEP_EPCONDIS)
#define USB_EP_NONE             (0x00)

typedef volatile unsigned int usb_uep_t;
#define USB_UEP                                 ((usb_uep_t*) (&U1EP0))
#define USB_UEP0                                U1EP0
#define USB_UEP1                                U1EP1
#define USB_UEP2                                U1EP2
#define USB_UEP3                                U1EP3
#define USB_UEP4                                U1EP4
#define USB_UEP5                                U1EP5
#define USB_UEP6                                U1EP6
#define USB_UEP7                                U1EP7
#define USB_UEP8                                U1EP8
#define USB_UEP9                                U1EP9
#define USB_UEP10                               U1EP10
#define USB_UEP11                               U1EP11
#define USB_UEP12                               U1EP12
#define USB_UEP13                               U1EP13
#define USB_UEP14                               U1EP14
#define USB_UEP15                               U1EP15

#define USB_U1OTGCON_VBUSDIS                    (0x1)
#define USB_U1OTGCON_VBUSCHG                    (0x2)
#define USB_U1OTGCON_OTGEN                      (0x4)
#define USB_U1OTGCON_VBUSON                     (0x8)
#define USB_U1OTGCON_DMPULDWN                   (0x10)
#define USB_U1OTGCON_DPPULDWN                   (0x20)
#define USB_U1OTGCON_DMPULUP                    (0x40)
#define USB_U1OTGCON_DPPULUP                    (0x80)

#define USB_U1PWRC_USBPWR                       (0x1)
#define USB_U1PWRC_USUSPEND                     (0x2)
#define USB_U1PWRC_USBBUSY                      (0x8)
#define USB_U1PWRC_USLPGRD                      (0x10)
#define USB_U1PWRC_UACTPND                      (0x80)

#define USB_DETACH                              (0x1)
#define USB_URST                                (0x1)
#define USB_UERR                                (0x2)
#define USB_SOF                                 (0x4)
#define USB_TRN                                 (0x08)
#define USB_IDLE                                (0x10)
#define USB_RESUME                              (0x20)
#define USB_ATTACH                              (0x40)
#define USB_STALL                               (0x80)

#define USB_U1CON_SOFEN                         (0x01)
#define USB_U1CON_USBEN                         (0x01)
#define USB_U1CON_PPBRST                        (0x02)
#define USB_U1CON_RESUME                        (0x04)
#define USB_U1CON_HOSTEN                        (0x08)
#define USB_U1CON_USBRST                        (0x10)
#define USB_U1CON_TOKBUSY                       (0x20)
#define USB_U1CON_PKTDIS                        (0x20)
#define USB_U1CON_SE0                           (0x40)
#define USB_U1CON_JSTATE                        (0x80)


#define UsbInterruptFlags()                     (U1IR)
#define UsbErrorInterruptFlags()                (UEIR)
#define ClearGlobalUsbInterruptFlag()           INTClearFlag(INT_USB)
#define TestUsbInterruptFlag(x)                 (U1IR&(x))
#define ClearUsbInterruptFlag(x)                U1IR = (x)
#define ClearAllUsbInterruptFlags()             U1IR = 0xFFFFFFFF
#define ClearUsbErrorInterruptFlag(x)           U1EIR = (x)
#define ClearAllUsbErrorInterruptFlags()        U1EIR = 0xFFFFFFFF
#define DisableGlobalUsbInterrupt()             INTEnable(INT_USB, INT_DISABLED)
#define DisableUsbInterrupt(x)                  U1IE &= ~(x)
#define DisableAllUsbInterrupts()               U1EIE = 0
#define DisableUsbErrorInterrupt(x)             U1EIE &= ~(x)
#define DisableAllUsbErrorInterrupts()          U1EIE = 0
#define EnableUsbGlobalInterrupt()              INTEnable(INT_USB, INT_ENABLED)
#define SetUsbGlobalInterruptPriority(x, y)     do { INTSetVectorPriority(INT_USB_1_VECTOR, x); INTSetVectorSubPriority(INT_USB_1_VECTOR, y); } while(0)
#define EnableUsbPerifInterrupts(x)             U1IE |= (x)
#define TestGlobalUsbInterruptEnable()          INTGetFlag(INT_USB)
#define EnableAllUsbInterrupts()                U1IE = 0xFF
#define EnableUsbErrorInterrupt(x)              U1EIE |= (x)
#define EnableAllUsbErrorInterrupts()           U1EIE = 0xFF

#define ResetPPbuffers()                        do {U1CONSET = USB_U1CON_PPBRST; U1CONCLR = USB_U1CON_PPBRST;} while(0)
#define SingleEndedZeroIsSet()                  ((U1CON & 0x40) > 0)
#define EnablePacketTransfer()                  U1CONCLR = (0x20)
#define EnableUsb()                             do {U1PWRCCLR = USB_U1PWRC_USBPWR; while(U1PWRC & USB_U1PWRC_USBBUSY){Nop();}; U1PWRCSET = USB_U1PWRC_USBPWR;} while(0)
//#define SignalResume()                          do {U1CONbits.RESUME = 1; delay_ms(10); U1CONbits.RESUME = 0;} while(0)
//#define SuspendUsb()                            U1PWRCbits.USUSPND = 1
//#define WakeupUsb()                             do {U1PWRCbits.USUSPND = 0; while(USB_RESUME_FLAG){USB_RESUME_FLAG = 0;}} while(0)


/* UADDR */
#define SetUsbAddress(x)                        (U1ADDR = (x))
#define GetUsbAddress()                         (U1ADDR)

/* USTAT */

// JTR moved to usb_stack.h
//typedef unsigned char usb_status_t;

#define GetUsbTransaction()                     (U1STAT)
#define USB_STAT2EP(x)                          (((x)>>4)&0x0F) //((x>>3)&0x0F)   JTR PIC24 fixups
#define USB_STAT2DIR(x)                         (((x)>>3)&0x01) //((x>>2)&0x01)
#define USB_STAT2PPI(x)                         (((x)>>2)&0x01) //((x>>1)&0x01)
#define USB_USTAT2BD(x)                         (((x)>>2)&0x3F)
#define USB_CALC_BD(x, y, z)                    (((x) << 2) + ((y) << 1) + (z))
#define DIRBIT                                  0x08

#define ConfigureUsbHardware(x)          do { \
                                                unsigned int addr = (unsigned int)_VirtToPhys((x)); \
                                                addr = addr >> 9; \
                                                U1CNFG1 = 0; \
                                                U1BDTP1 = addr << 1; \
                                                U1BDTP2 = addr >> 7; \
                                                U1BDTP3 = addr >> 15; \
                                                U1OTGCON = USB_U1OTGCON_DPPULUP | USB_U1OTGCON_DMPULDWN | USB_U1OTGCON_OTGEN; \
                                                U1CON = USB_U1CON_USBEN; \
                                                 \
                                        } while(0)

#ifdef	__cplusplus
}
#endif

#endif	/* PIC32MXUSBOTG_H */

