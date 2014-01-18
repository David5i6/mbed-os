/**HEADER********************************************************************
* 
* Copyright (c) 2004 -2010, 2013 Freescale Semiconductor;
* All Rights Reserved
*
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
 * @file usb_composite.c
 *
 * @author
 *
 * @version
 *
 * @date 
 *
 * @brief The file contains USB composite layer implementation.
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "usb_device_config.h"
#include "usb.h"

#include "usb_device_stack_interface.h"
#include "usb_class_composite.h"
#include "usb_composite.h"
#include "usb_class_internal.h"

#if USBCFG_DEV_COMPOSITE
#if USBCFG_DEV_HID
#include "usb_class_hid.h"
#endif

#if USBCFG_DEV_MSC
#include "usb_class_msc.h"
#endif

#if USBCFG_DEV_AUDIO
#include "usb_class_audio.h"
#endif

#if USBCFG_DEV_VIDEO
    #include "usb_video.h"
#endif

#if USBCFG_DEV_PHDC
    #include "usb_phdc.h"
#endif

#if USBCFG_DEV_CDC
#include "usb_class_cdc.h"
#endif

#if USBCFG_DEV_DFU
    #include "usb_dfu.h"
#endif
/*****************************************************************************
 * Local functions prototypes
 *****************************************************************************/ 
void USB_Composite_Event (uint8_t event, void* val,void * arg) ;

uint8_t USB_Composite_Requests (
    USB_SETUP_STRUCT * setup_packet,    /* [IN] Setup packet received */
	uint8_t * *data, 
	uint32_t *size,
	void* arg );


/*****************************************************************************
 * Global functions
 *****************************************************************************/ 

 /**************************************************************************//*!
 *
 * @name  USB_Composite_Init
 *
 * @brief   The funtion initializes the Device and Controller layer
 *
 * @param   controller_id               : Controller ID
 * @param   composite_callback_ptr      : Pointer to app callback  
 *
 * @return status:
 *                        USB_OK        : When Successfull
 *                        Others        : When Error
 *
 ******************************************************************************
 *This function initializes the Composite layer
 *****************************************************************************/ 
uint8_t USB_Composite_Init(
    uint8_t    controller_id,                /* [IN] Controller ID */
    COMPOSITE_CALLBACK_STRUCT *composite_callback_ptr,  /* [IN] Poiter to class info */
    COMPOSITE_HANDLE *  compositeHandle   
)
{    
    uint8_t count,status = USB_OK;
    COMPOSITE_DEVICE_STRUCT *devicePtr;
    //USB_CLASS_HANDLE class_handle;
    USB_COMPOSITE_INFO_STRUCT_PTR usb_composite_info;
	 
    if (NULL == composite_callback_ptr)
    {
        return USBERR_ERROR;    
    }	
    devicePtr = (COMPOSITE_DEVICE_STRUCT_PTR)OS_Mem_alloc_zero(sizeof(COMPOSITE_DEVICE_STRUCT));
    if (NULL == devicePtr)
    {
        #if _DEBUG
            printf("USB_Class_COMPOSITE_Init: Memalloc devicePtr failed\n");
        #endif  
        return USBERR_ALLOC;
    }
    devicePtr->cl_count = composite_callback_ptr->count;
	devicePtr->class_app_callback = (COMPOSITE_CONFIG_STRUCT_PTR)OS_Mem_alloc_zero(devicePtr->cl_count*sizeof(COMPOSITE_CONFIG_STRUCT));
    if (NULL == devicePtr->class_app_callback)
    {
        #if _DEBUG
            printf("USB_Class_COMPOSITE_Init: Memalloc class_app_callback failed\n");
        #endif  
		OS_Mem_free(devicePtr);  
        return USBERR_ALLOC;
    }
  
	OS_Mem_copy(composite_callback_ptr->class_app_callback,
    devicePtr->class_app_callback,devicePtr->cl_count*sizeof(COMPOSITE_CONFIG_STRUCT));

	devicePtr->class_composite_info = (USB_COMPOSITE_INFO_STRUCT_PTR)OS_Mem_alloc_zero(sizeof(USB_COMPOSITE_INFO_STRUCT));
    if (NULL == devicePtr->class_composite_info)
    {
        #if _DEBUG
            printf("USB_Class_COMPOSITE_Init: Memalloc class_composite_info failed\n");
        #endif  
		OS_Mem_free(devicePtr->class_app_callback);
		OS_Mem_free(devicePtr);
        return USBERR_ALLOC;
    }
	 
	//status = usb_device_preinit(controller_id,&devicePtr->handle);
 	
   	status = usb_device_init(controller_id,&devicePtr->handle);
   	devicePtr->class_app_callback->desc_callback_ptr->GET_DESC_ENTITY((uint32_t)devicePtr->handle,
		USB_COMPOSITE_INFO,
			(uint32_t *)&usb_composite_info);
	OS_Mem_copy(usb_composite_info,
    			devicePtr->class_composite_info , sizeof(USB_COMPOSITE_INFO_STRUCT));
    if(status == USB_OK)
    {
        /* Initialize the generic class functions */
        //class_handle = 
        devicePtr->class_handle = USB_Class_Init(devicePtr->handle,USB_Composite_Event, USB_Composite_Requests,(void *)devicePtr,
                            composite_callback_ptr->class_app_callback[0].desc_callback_ptr);

        if(status == USB_OK)
        {   
            for(count = 0; count < devicePtr->cl_count; count++)
            {
                /* Initializes sub_classes */
                switch(composite_callback_ptr->class_app_callback[count].type)  
                {
#if USBCFG_DEV_HID 
                    case USB_CLASS_HID:
                        (void)USB_Class_HID_Init(
                            controller_id,(struct hid_config_struct *)&devicePtr->class_app_callback[count],
							&devicePtr->hid_handle);
                        break;
#endif
#if USBCFG_DEV_AUDIO
                    case USB_CLASS_AUDIO:
                        (void)USB_Class_Audio_Init(
                            controller_id,(struct audio_config_struct *)&devicePtr->class_app_callback[count],
							&devicePtr->audio_handle);
                        break;
#endif

#if USBCFG_DEV_CDC
                    case USB_CLASS_CDC:                            
                        (void)USB_Class_CDC_Init(
                            controller_id, (CDC_CONFIG_STRUCT_PTR)&devicePtr->class_app_callback[count], &devicePtr->cdc_handle);      
                        break;
#endif
#if USBCFG_DEV_MSC
                    case USB_CLASS_MSC:
                        (void)USB_Class_MSC_Init(
                            controller_id, (USB_MSD_CONFIG_STRUCT_PTR)&devicePtr->class_app_callback[count], &devicePtr->msc_handle);      
                        break;
#endif
#if USBCFG_DEV_PHDC
                    case USB_CLASS_PHDC:
                        (void)USB_Class_PHDC_Init(
                            controller_id,(struct phdc_config_struct *)&devicePtr->class_app_callback[count],
    						&devicePtr->phdc_handle);
                        break;
#endif
#if USBCFG_DEV_DFU
                    case DFU_COMP_CC:
                        (void)USB_Class_Dfu_Init(
                            controller_id,composite_callback_ptr->class_app_callback[count]->composite_application_callback,
                            composite_callback_ptr->class_app_callback[count]->vendor_req_callback,
                            composite_callback_ptr->class_app_callback[count]->param_callback); 
                        break;
#endif
                    default:
                        break;
                }
            }
        }
    }
	
	/* Initialize the device layer*/
	*compositeHandle = (COMPOSITE_HANDLE)devicePtr;

    return status;   
}

 /**************************************************************************//*!
 *
 * @name  USB_Composite_DeInit
 *
 * @brief   The funtion De-initializes the Device and Controller layer
 *
 * @param   controller_id               : Controller ID
 * @param   composite_callback_ptr      : Pointer to app callback  
 *
 * @return status:
 *                        USB_OK        : When Successfull
 *                        Others        : When Error
 *
 ******************************************************************************
 *This function De-initializes the Composite layer
 *****************************************************************************/ 
uint8_t USB_Composite_DeInit(
   	COMPOSITE_HANDLE   handle                           /* [IN] Controller ID */
)
{    
	COMPOSITE_DEVICE_STRUCT	*devicePtr;
	uint8_t count;
	uint8_t status = USB_OK;
	 if (handle == 0)
		 return USBERR_ERROR;
	 
	 devicePtr = (COMPOSITE_DEVICE_STRUCT *)handle;
	 
	 if (NULL == devicePtr)
	 {
		 return USBERR_NO_DEVICE_CLASS;
	 }
	 
    for(count = 0; count < devicePtr->cl_count; count++)
    {

       switch(devicePtr->class_app_callback[count].type)  
        {
#if USBCFG_DEV_HID
            case USB_CLASS_HID:
                //status = 
                USB_Class_HID_Deinit(devicePtr->hid_handle);
            break;
#endif
#if USBCFG_DEV_AUDIO   
            case USB_CLASS_AUDIO:
               //status = 
               USB_Class_Audio_Deinit(devicePtr->audio_handle);
            break;
#endif
#if USBCFG_DEV_CDC
            case USB_CLASS_CDC:
                //status = 
                USB_Class_CDC_Deinit(devicePtr->cdc_handle);
                break;
#endif
#if USBCFG_DEV_MSC
           case USB_CLASS_MSC:
                //status = 
                USB_Class_MSC_Deinit(devicePtr->msc_handle);
                break;
#endif
#if USBCFG_DEV_PHDC
            case USB_CLASS_PHDC:
                //status = 
                USB_Class_PHDC_Deinit(devicePtr->phdc_handle);
                break;
#endif

        }
    }
	
    if(status == USB_OK)
        /* Deinitialize the generic class functions */
        status = USB_Class_Deinit(devicePtr->handle,devicePtr->class_handle);
    if(status == USB_OK)
        /* Deinitialize the device layer*/
        status = usb_device_deinit(devicePtr->handle);
    
	OS_Mem_free(devicePtr->class_app_callback);
	OS_Mem_free(devicePtr->class_composite_info);
	OS_Mem_free(devicePtr);
    devicePtr = NULL;
	
    return USB_OK;    
}

/**************************************************************************//*!
 *
 * @name  USB_Composite_Event
 *
 * @brief The funtion initializes composite endpoint
 *
 * @param controller_id     : Controller ID
 * @param event             : Event Type
 * @param val               : Pointer to configuration Value
 *
 * @return None
 *
 ******************************************************************************
 * 
 *****************************************************************************/
void USB_Composite_Event (uint8_t event, void* val,void * arg) 
{   
    uint8_t count;
	COMPOSITE_DEVICE_STRUCT	*devicePtr;

	devicePtr = (COMPOSITE_DEVICE_STRUCT *)arg;
	 
	 if (NULL == devicePtr)
	 {
		 return ;
	 }

     for(count = 0; count < devicePtr->cl_count; count++)
     {
       switch(devicePtr->class_app_callback[count].type)
       {
#if USBCFG_DEV_HID
            case USB_CLASS_HID:
                USB_Class_Hid_Event(event,val,(void *)devicePtr->hid_handle);
            break;
#endif
#if USBCFG_DEV_AUDIO    
            case USB_CLASS_AUDIO:
                USB_Class_Audio_Event(event,val,(void *)devicePtr->audio_handle);
            break;
#endif
#if USBCFG_DEV_CDC
            case USB_CLASS_CDC:
                USB_Class_CDC_Event(event,val,(void *)devicePtr->cdc_handle);
                break;
#endif
#if USBCFG_DEV_MSC
            case USB_CLASS_MSC:
                USB_Class_MSC_Event(event,val,(void *)devicePtr->msc_handle);
                break;
#endif
#if USBCFG_DEV_PHDC
            case USB_CLASS_PHDC:
                USB_Class_PHDC_Event(event,val,(void *)devicePtr->phdc_handle);
                break;
#endif
        }
    }
}

/**************************************************************************//*!
 *
 * @name  USB_Composite_Other_Requests
 *
 * @brief   The funtion provides flexibilty to add class and vendor specific
 *              requests
 *
 * @param controller_id     : Controller ID
 * @param setup_packet      : Setup packet received
 * @param data              : Data to be send back
 * @param size              : Size to be returned
 *
 * @return status:
 *                        USB_OK        : When Successfull
 *                        Others        : When Error
 *
 ******************************************************************************
 * Handles Class requests and forwards vendor specific request to the
 * application
 *****************************************************************************/
uint8_t USB_Composite_Requests (
    USB_SETUP_STRUCT * setup_packet,    /* [IN] Setup packet received */
	uint8_t * *data, 
	uint32_t *size,
	void* arg)
{
     uint8_t count;
     uint8_t status = USB_OK;
	COMPOSITE_DEVICE_STRUCT	*devicePtr;
	uint8_t itf_num = 0xFF;
    //uint16_t ep_num = 0;
	uint8_t type_sel;
    USB_CLASS_STRUCT_PTR usbclassPtr;
	USB_IF_STRUCT *if_ptr;
	devicePtr = (COMPOSITE_DEVICE_STRUCT *)arg;
	 
	 if (NULL == devicePtr)
	 {
		 return USBERR_NO_DEVICE_CLASS;
	 }

     devicePtr->class_app_callback->desc_callback_ptr->GET_DESC_ENTITY((uint32_t)devicePtr->handle,
         USB_CLASS_INFO, (uint32_t *)&usbclassPtr);

	if(setup_packet->request_type & 0x01)
		itf_num = (setup_packet->index);
#if 0
	/* request is for an Endpoint */
	else if(setup_packet->request_type & 0x02)
		ep_num = (setup_packet->index);
#endif
     for(count = 0; count < devicePtr->cl_count; count++)
     {
        switch(devicePtr->class_app_callback[count].type)
        {
#if USBCFG_DEV_HID
            /* Call Hid other request */
            case USB_CLASS_HID:
			for(type_sel = 0;type_sel < devicePtr->class_composite_info->count;type_sel++)
			{
				if(devicePtr->class_composite_info->class[type_sel].type == USB_CLASS_HID)
					break;
			}
            if(itf_num == devicePtr->class_composite_info->class[type_sel].interfaces.interface->index)    
            		USB_HID_Requests(setup_packet,data,size,(void *)devicePtr->hid_handle); 
            break;
#endif
#if USBCFG_DEV_AUDIO  
            /* Call Audio other request */
            case USB_CLASS_AUDIO:
			for(type_sel = 0;type_sel < devicePtr->class_composite_info->count;type_sel++)
			{
				if(devicePtr->class_composite_info->class[type_sel].type == USB_CLASS_AUDIO)
					break;
			}
            if(itf_num == devicePtr->class_composite_info->class[type_sel].interfaces.interface->index)    
            	USB_Audio_Requests(setup_packet,data,size,(void *)devicePtr->audio_handle); 
            break;
#endif
#if USBCFG_DEV_CDC  
            /* Call Cdc other request */ 
            case USB_CLASS_CDC:
                for_each_if_in_class(if_ptr, usbclassPtr, USB_CLASS_CDC)
                {
                    if(if_ptr->index == itf_num)
                    {
                        status = USB_CDC_Other_Requests(setup_packet,data,size, (void *)devicePtr->cdc_handle);
                    }
                }
                break;
#endif
#if USBCFG_DEV_MSC
            /* Call Msd other request */
            case USB_CLASS_MSC:
                for_each_if_in_class(if_ptr, usbclassPtr, USB_CLASS_MSC)
                {
                    if(if_ptr->index == itf_num)
                    {
                        status = USB_MSC_Requests(setup_packet,data,size, (void *)devicePtr->msc_handle);
                    }
                }
                break;
#endif
#if USBCFG_DEV_PHDC
            /* Call Phdc other request */
            case USB_CLASS_PHDC:
    			for(type_sel = 0;type_sel < devicePtr->class_composite_info->count;type_sel++)
    			{
    				if(devicePtr->class_composite_info->class[type_sel].type == USB_CLASS_PHDC)
    					break;
    			}
                if(itf_num == devicePtr->class_composite_info->class[type_sel].interfaces.interface->index)    
                	USB_PHDC_Requests(setup_packet,data,size,(void *)devicePtr->phdc_handle); 
                break;
#endif
            default:
                break;
        }
      }
    return status;  
}

 /**************************************************************************//*!
 *
 * @name  USB_Composite_Get_Class_Handle
 *
 * @brief   The funtion returns the specific class handle to the content referred by class_handle_ptr.
 *
 * @param   handle               : composite handle
 * @param   type                  : class type  
 * @param   class_handle_ptr      : pointer to class handle
 *
 * @return status:
 *                        USB_OK        : When Successfull
 *                        Others        : When Error
 *
 ******************************************************************************
 *This function gets the specific class handle
 *****************************************************************************/ 
uint8_t USB_Composite_Get_Class_Handle(
    COMPOSITE_HANDLE handle,                /* [IN] composite handle */
    class_type type,                        /* [IN] class type */
    void *  class_handle_ptr                /* [IN] pointer to class handle */   
)
{
	 uint8_t status = USB_OK;
	 COMPOSITE_DEVICE_STRUCT_PTR dev_ptr = (COMPOSITE_DEVICE_STRUCT_PTR)handle;
	 if(USB_CLASS_INVALID < type)
		 status = USBERR_ERROR;
     switch(type)
     {
		case USB_CLASS_HID:
		*((HID_HANDLE *)class_handle_ptr) = dev_ptr->hid_handle;
		break;
		case USB_CLASS_CDC:
		*((CDC_HANDLE *)class_handle_ptr) = dev_ptr->cdc_handle;
		break;
		case USB_CLASS_MSC:
		*((MSD_HANDLE *)class_handle_ptr) = dev_ptr->msc_handle;
		break;
		case USB_CLASS_AUDIO:
		*((AUDIO_HANDLE *)class_handle_ptr) = dev_ptr->audio_handle;
		break;
		case USB_CLASS_PHDC:
		*((PHDC_HANDLE *)class_handle_ptr) = dev_ptr->phdc_handle;
		break;
		default:
			status = USBERR_ERROR;
		break;
     }
	return status;
}
#endif
/* EOF */
