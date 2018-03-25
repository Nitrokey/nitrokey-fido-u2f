/*******************************************************************************
 * @file usbconfig.h
 * @brief USB protocol stack library, application supplied configuration options.
 *******************************************************************************/

//=============================================================================
// inc/config/usbconfig.h: generated by Hardware Configurator
//
// This file will be regenerated when saving a document. leave the sections
// inside the "$[...]" comment tags alone or they will be overwritten!
//=============================================================================
#ifndef __SILICON_LABS_USBCONFIG_H
#define __SILICON_LABS_USBCONFIG_H

// -----------------------------------------------------------------------------
// Specify bus- or self-powered
// -----------------------------------------------------------------------------
// $[Device Power]
#define SLAB_USB_BUS_POWERED                   1
// [Device Power]$

// -----------------------------------------------------------------------------
// Specify USB speed
// -----------------------------------------------------------------------------
// $[USB Speed]
#define SLAB_USB_FULL_SPEED                    1
// [USB Speed]$

// -----------------------------------------------------------------------------
// Enable or disable the clock recovery
// -----------------------------------------------------------------------------
// $[Clock Recovery]
#define SLAB_USB_CLOCK_RECOVERY_ENABLED        1
// [Clock Recovery]$

// -----------------------------------------------------------------------------
// Enable or disable remote wakeup
// -----------------------------------------------------------------------------
// $[Remote Wake-up]
#define SLAB_USB_REMOTE_WAKEUP_ENABLED         0
// [Remote Wake-up]$

// -----------------------------------------------------------------------------
// Specify number of interfaces and whether any interfaces support alternate
// settings
// -----------------------------------------------------------------------------
// $[Number of Interfaces]
#define SLAB_USB_NUM_INTERFACES                1
#define SLAB_USB_SUPPORT_ALT_INTERFACES        0
// [Number of Interfaces]$

// -----------------------------------------------------------------------------
// Enable or disable each endpoint
// -----------------------------------------------------------------------------
// $[Endpoints Used]
#define SLAB_USB_EP1IN_USED                    1
#define SLAB_USB_EP1OUT_USED                   1
#define SLAB_USB_EP2IN_USED                    0
#define SLAB_USB_EP2OUT_USED                   0
#define SLAB_USB_EP3IN_USED                    0
#define SLAB_USB_EP3OUT_USED                   0
// [Endpoints Used]$

// -----------------------------------------------------------------------------
// Specify maximum packet size for each endpoint
// -----------------------------------------------------------------------------
// $[Endpoint Max Packet Size]
#define SLAB_USB_EP1IN_MAX_PACKET_SIZE         64
#define SLAB_USB_EP1OUT_MAX_PACKET_SIZE        64
#define SLAB_USB_EP2IN_MAX_PACKET_SIZE         64
#define SLAB_USB_EP2OUT_MAX_PACKET_SIZE        64
#define SLAB_USB_EP3IN_MAX_PACKET_SIZE         64
#define SLAB_USB_EP3OUT_MAX_PACKET_SIZE        64
// [Endpoint Max Packet Size]$

// -----------------------------------------------------------------------------
// Specify transfer type of each endpoint
// -----------------------------------------------------------------------------
// $[Endpoint Transfer Type]
#define SLAB_USB_EP1IN_TRANSFER_TYPE           USB_EPTYPE_INTR
#define SLAB_USB_EP1OUT_TRANSFER_TYPE          USB_EPTYPE_INTR
#define SLAB_USB_EP2IN_TRANSFER_TYPE           USB_EPTYPE_BULK
#define SLAB_USB_EP2OUT_TRANSFER_TYPE          USB_EPTYPE_BULK
#define SLAB_USB_EP3IN_TRANSFER_TYPE           USB_EPTYPE_ISOC
#define SLAB_USB_EP3OUT_TRANSFER_TYPE          USB_EPTYPE_ISOC
// [Endpoint Transfer Type]$

// -----------------------------------------------------------------------------
// Enable or disable callback functions
// -----------------------------------------------------------------------------
// $[Callback Functions]
#define SLAB_USB_HANDLER_CB                    0
#define SLAB_USB_IS_SELF_POWERED_CB            1
#define SLAB_USB_RESET_CB                      1
#define SLAB_USB_SETUP_CMD_CB                  1
#define SLAB_USB_SOF_CB                        0
#define SLAB_USB_STATE_CHANGE_CB               1
// [Callback Functions]$

// -----------------------------------------------------------------------------
// Specify number of languages supported by string descriptors.
// -----------------------------------------------------------------------------
// $[Number of Languages]
#define SLAB_USB_NUM_LANGUAGES                 1
// [Number of Languages]$

// -----------------------------------------------------------------------------
// If only one descriptor language is supported, specify that language here.
// If multiple descriptor languages are supported, this value is ignored and
// the supported languages must listed in the
// myUsbStringTableLanguageIDsDescriptor structure.
// -----------------------------------------------------------------------------
// $[USB Language]
#define SLAB_USB_LANGUAGE                      USB_LANGID_ENUS
// [USB Language]$

// -----------------------------------------------------------------------------
// 
// Set the power saving mode
// 
// SLAB_USB_PWRSAVE_MODE configures when the device will automatically enter
// the USB power-save mode. It is a bitmask constant with bit values:
// USB_PWRSAVE_MODE_OFF       - No energy saving mode selected
// USB_PWRSAVE_MODE_ONSUSPEND - Enter USB power-save mode on USB suspend
// USB_PWRSAVE_MODE_ONVBUSOFF - Enter USB power-save mode when not attached
//                              to the USB host.
// USB_PWRSAVE_MODE_FASTWAKE  - Exit USB power-save mode more quickly.
//                              This is useful for some applications that
//                              support remote wakeup.    
// 
// -----------------------------------------------------------------------------
// $[Power Save Mode]
#define SLAB_USB_PWRSAVE_MODE                  ( USB_PWRSAVE_MODE_OFF )
// [Power Save Mode]$

// -----------------------------------------------------------------------------
// Enable or disable polled mode
//     
// When enabled, the application must call USBD_Run() periodically to process
// USB events.
// When disabled, USB events will be handled automatically by an interrupt
// handler.
// -----------------------------------------------------------------------------
// $[Polled Mode]
#define SLAB_USB_POLLED_MODE                   0
// [Polled Mode]$

#endif // __SILICON_LABS_USBCONFIG_H
