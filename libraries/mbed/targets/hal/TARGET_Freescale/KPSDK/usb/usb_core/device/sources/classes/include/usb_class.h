/**HEADER********************************************************************
* 
* Copyright (c) 2008, 2013 Freescale Semiconductor;
* All Rights Reserved
*
* Copyright (c) 1989-2008 ARC International;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: usb_class.h$
* $Version : 
* $Date    : 
*
* Comments:
*
* @brief The file contains USB stack class layer api header function.
*
*****************************************************************************/

#ifndef _USB_CLASS_H
#define _USB_CLASS_H 1

/******************************************************************************
 * Includes
 *****************************************************************************/



/******************************************************************************
 * Global Variables
 *****************************************************************************/

/******************************************************************************
 * Macro's
 *****************************************************************************/
 /*#define DELAYED_PROCESSING  1 This define is used to delay the control 
                                processing and not have it executed as part
                                of the interrupt routine */

//#define UNINITIALISED_VAL        (0xffffffff)  
/* Events to the Application */
/*
#define USB_APP_BUS_RESET           (0)
#define USB_APP_CONFIG_CHANGED      (1)
#define USB_APP_ENUM_COMPLETE       (2)
#define USB_APP_SEND_COMPLETE       (3)
#define USB_APP_DATA_RECEIVED       (4) 
#define USB_APP_ERROR               (5)
#define USB_APP_GET_DATA_BUFF       (6)
#define USB_APP_EP_STALLED          (7)
#define USB_APP_EP_UNSTALLED        (8) 
#define USB_APP_GET_TRANSFER_SIZE   (9)
*/

/******************************************************************************
 * Types
 *****************************************************************************/
typedef uint32_t USB_CLASS_HANDLE;

/* callback function pointer structure for Application to handle events */
//typedef void(_CODE_PTR_ USB_CLASS_CALLBACK)(uint8_t event, void* val, pointer arg);

/* callback function pointer structure to handle USB framework request */
//typedef uint8_t (_CODE_PTR_ USB_REQ_FUNC)(USB_SETUP_STRUCT *,
//                                          uint8_t **,
//                                          uint32_t*,pointer arg); 

/*callback function pointer structure for application to provide class params*/
typedef uint8_t (_CODE_PTR_ USB_CLASS_SPECIFIC_HANDLER_FUNC)(
                           uint8_t,
                           uint16_t, 
                           uint8_t **,
                           uint32_t*,
                           void* arg);


typedef struct usb_class_specific_handler_callback_struct
{
    USB_CLASS_SPECIFIC_HANDLER_FUNC  callback;
    void*                            arg;
}USB_CLASS_SPECIFIC_HANDLER_CALLBACK_STRUCT,
* USB_CLASS_SPECIFIC_HANDLER_CALLBACK_STRUCT_PTR;

#endif

/* EOF */
