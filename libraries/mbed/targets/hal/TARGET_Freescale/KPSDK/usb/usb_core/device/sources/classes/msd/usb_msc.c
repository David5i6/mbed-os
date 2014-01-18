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
* $FileName: usb_msc.c$
* $Version : 
* $Date    : 
*
* Comments:
*
* @brief The file contains USB stack MSC layer implimentation.
*
*****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device_stack_interface.h"

 
#if USBCFG_DEV_MSC
	#include "usb_class_internal.h"
    #include "usb_msc.h"
	#include "usb_opt.h"
/*****************************************************************************
 * Constant and Macro's
 *****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/*****************************************************************************
 * Local Types - None
 *****************************************************************************/

/*****************************************************************************
 * Local Functions Prototypes
 *****************************************************************************/
uint8_t process_mass_storage_command (MSC_DEVICE_STRUCT_PTR mscDevicePtr,
                                     PTR_CBW cbw_ptr, 
                                     uint32_t* csw_residue_ptr, 
                                     uint8_t* csw_status_ptr);                           
                          
/*****************************************************************************
 * Local Variables - None
 *****************************************************************************/
 uint8_t * msc_lba_send_ptr;
 uint8_t * msc_lba_recv_ptr;
 /*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*************************************************************************//*!
 *
 * @name  USB_Msd_Get_Device_Ptr
 *
 * @brief The funtion gets the device pointer from device array .
 *
 * @param handle  index in device array.
 * @return returns returns pointer to MSD device structure..      
 *
 *****************************************************************************/
static MSC_DEVICE_STRUCT_PTR USB_Msd_Get_Device_Ptr(MSD_HANDLE handle)
{
     return (MSC_DEVICE_STRUCT *)handle; 
}

  /*************************************************************************//*!
 *
 * @name  USB_Msd_Get_Desc_Info
 *
 * @brief The funtion gets the info of the descriptors. .
 *
 * @param handle  index in device array.
 * @param type     descriptor type.
 * @param object   store the returned value.
 * @return returns USB_OK if successful.      
 *
 *****************************************************************************/
 static uint8_t USB_Msd_Get_Desc_Info(MSC_DEVICE_STRUCT_PTR msc_dev_ptr,USB_MSD_DESC_INFO_T type, uint32_t * object)
   {
	   
	   /* Get class info */
	   
	   switch(type)
	   {
		   case USB_MSD_INTERFACE_COUNT:
#if USBCFG_DEV_COMPOSITE
			*object = MAX_MSC_SUPPORTED_INTERFACES + 2;
#else
			*object = MAX_MSC_SUPPORTED_INTERFACES;
#endif
                   break;
#if USBCFG_DEV_COMPOSITE
		   case USB_MSD_CLASS_INFO:
		   {
			   uint32_t class_i;
			   USB_CLASS_STRUCT_PTR usbclassPtr;
			   USB_COMPOSITE_INFO_STRUCT_PTR usbcompinfoPtr;
			   /* Get class info */
			   msc_dev_ptr->desc_callback_ptr->GET_DESC_ENTITY((uint32_t)msc_dev_ptr->controller_handle,
				   USB_COMPOSITE_INFO,
				   (uint32_t *)&usbcompinfoPtr);
			   usbclassPtr = usbcompinfoPtr->class;
			   for(class_i = 0; usbclassPtr[class_i].type != USB_CLASS_INVALID; class_i++)
			   {
					   if(USB_CLASS_MSC == usbclassPtr[class_i].type)
					   {
							   *object = (uint32_t)&usbclassPtr[class_i];
							   break;
					   }
			   }
		   }
		   break;
#endif
		   default :
			   break;
	   } 
	   return USB_OK;
  }


  
/**************************************************************************//*!
 *
 * @name  process_mass_storage_command
 *
 * @brief Process a Mass storage class command 
 *        This function is added here to add more sub class specific commands)
 *
 * @param MSC_DEVICE_STRUCT_PTR 
 * @param cbw_ptr        : pointer to Command Block Wrapper sent by host
 * @param csw_residue_ptr: pointer to dCSWDataResidue of Command Status Wrapper
 * @param csw_status_ptr : pointer to bCSWStatus  of Command Status Wrapper
 *
 * @return error
 *
 *****************************************************************************/
uint8_t process_mass_storage_command 
(
    MSC_DEVICE_STRUCT_PTR msc_obj_ptr,
    PTR_CBW cbw_ptr, 
    uint32_t* csw_residue_ptr, 
    uint8_t* csw_status_ptr
)
{   
    uint8_t error = USBERR_ERROR;/* initializing to error value */
    switch (cbw_ptr->command_block[0]) 
    {
        /* commands to be supported by all devices */
        case INQUIRY_COMMAND : /*opcode : 0x12*/
               error = msc_inquiry_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case READ_10_COMMAND : /*opcode : 0x28 */
        case READ_12_COMMAND : /*opcode : 0xA8 */
            error = msc_read_command(msc_obj_ptr, 
            cbw_ptr,csw_residue_ptr,csw_status_ptr);
            break;      
        case REQUEST_SENSE_COMMAND : /*opcode : 0x03*/
            error = msc_request_sense_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
           break;           
        case TEST_UNIT_READY_COMMAND : /*opcode : 0x00 */ 
           error = msc_test_unit_ready_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break; 
        case WRITE_10_COMMAND : /*opcode : 0x2A */
        case WRITE_12_COMMAND : /*opcode : 0xAA */
           error = msc_write_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
           csw_status_ptr);
            break; 
        case PREVENT_ALLOW_MEDIUM_REM_COMMAND : /*opcode :0x1E */
           error = msc_prevent_allow_medium_removal(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;                   
        case FORMAT_UNIT_COMMAND : /*opcode : 0x04*/
           error = msc_format_unit_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case READ_CAPACITY_10_COMMAND : /*opcode : 0x25*/
           error = msc_read_capacity_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;       
        case MODE_SENSE_10_COMMAND : /* opcode :0x5A*/
        case MODE_SENSE_6_COMMAND : /* opcode : 0x1A */
           error = msc_mode_sense_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case MODE_SELECT_10_COMMAND : /*opcode : 0x55 */
        case MODE_SELECT_6_COMMAND : /*opcode : 0x15 */
           error = msc_mode_select_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case READ_FORMAT_CAPACITIES_COMMAND : /*opcode : 0x23 */
           error = msc_read_format_capacity_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case SEND_DIAGNOSTIC_COMMAND : /*opcode : 0x1D*/
            error = msc_send_diagnostic_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break; 
        case VERIFY_COMMAND : /*opcode : 0x2F*/
            error = msc_verify_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;
        case START_STOP_UNIT_COMMAND : /*opcode : 0x1B*/
            error = msc_start_stop_unit_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            break;                                  
        default: /* for all unsupported commands */
            error = msc_unsupported_command(msc_obj_ptr, cbw_ptr,csw_residue_ptr,
                csw_status_ptr);
            msc_obj_ptr->out_flag = FALSE;
            msc_obj_ptr->in_flag = FALSE;
            msc_obj_ptr->out_stall_flag = FALSE;
            msc_obj_ptr->in_stall_flag = FALSE;
            break;
    }
    return error;
}

/**************************************************************************//*!
 *
 * @name  USB_Service_Bulk_In
 *
 * @brief The funtion ic callback function of DIC Bulk In Endpoint 
 *
 * @param event
 * @param arg
 * @return None       
 *
 *****************************************************************************/
void USB_Service_Bulk_In(PTR_USB_EVENT_STRUCT event, void* arg)
{
    uint8_t event_type; 
    PTR_CSW csw_ptr;
    MSC_DEVICE_STRUCT_PTR msc_obj_ptr;
    
    #if IMPLEMENT_QUEUING 
        uint8_t index;
        uint8_t producer, consumer;
        USB_ENDPOINTS *usb_ep_data ;
            
        USB_CLASS_MSC_QUEUE queue;
    #endif
    
    msc_obj_ptr = (MSC_DEVICE_STRUCT_PTR)arg;  
    
    if (NULL == msc_obj_ptr)
    {
        #if _DEBUG
            printf("USB_Service_Bulk_In: msc_obj_ptr is NULL\n");
        #endif  
        return;
    }
    
    #if IMPLEMENT_QUEUING    
        usb_ep_data = msc_obj_ptr->ep_desc_data; 
        
        /* map the endpoint num to the index of the endpoint structure */
        for(index = 0; index < usb_ep_data->count; index++) 
        {
            if(usb_ep_data->ep[index].ep_num == event->ep_num)
            break;
        }
                                                   
        producer = msc_obj_ptr->ep[index].bin_producer;         
        /* if there are no errors de-queue the queue and decrement the no. of 
           transfers left, else send the same data again */
        msc_obj_ptr->ep[index].bin_consumer++;              
        consumer = msc_obj_ptr->ep[index].bin_consumer;         
        
        if(consumer != producer) 
        {/*if bin is not empty */
                            
            queue = msc_obj_ptr->ep[index].queue[consumer%MSD_MAX_QUEUE_ELEMS];                         
            (void)USB_Class_Send_Data(msc_obj_ptr->class_handle, queue.channel, 
                queue.app_data.data_ptr, queue.app_data.data_size);
            return;
        }          
    #endif

    if(msc_obj_ptr->transfer_remaining >= event->len)
    {   /* decrement the global count */
        msc_obj_ptr->transfer_remaining -= event->len;          
    }
        
    /* check if there is need to stall BULK IN ENDPOINT And
       there isn't any transfer in progress*/
    if( (msc_obj_ptr->in_stall_flag == TRUE)&&
        (!msc_obj_ptr->transfer_remaining))
    {
        //uint8_t component = (uint8_t)(event->ep_num | 
        //    (USB_SEND<<COMPONENT_PREPARE_SHIFT));
        msc_obj_ptr->in_stall_flag = FALSE; /* clear the flag */
        msc_obj_ptr->in_flag = FALSE; /* clear send flag */
        usb_device_stall_endpoint(msc_obj_ptr->controller_handle,event->ep_num,USB_SEND);
        return;                 
    }

    /* If its not a data phase on bulk endpoint */
    if ((!msc_obj_ptr->in_flag) && (event->len == MSC_CSW_LENGTH)) 
    {
        csw_ptr = (PTR_CSW)(event->buffer_ptr);
    } 
   
    if(msc_obj_ptr->in_flag) /* bulk in transaction has occured before CSW */
    {           
        if(msc_obj_ptr->class_specific_callback.callback != NULL) 
        {
            event_type = (uint8_t)(msc_obj_ptr->transfer_remaining?
                USB_MSC_DEVICE_READ_REQUEST : USB_DEV_EVENT_SEND_COMPLETE);
            
            if(event_type == USB_DEV_EVENT_SEND_COMPLETE)
            {       
                MSC_APP_DATA_STRUCT bulk_in_recv;
                bulk_in_recv.data_ptr = event->buffer_ptr;
                bulk_in_recv.data_size = event->len; 
                msc_obj_ptr->class_specific_callback.callback(event_type,
                    USB_REQ_VAL_INVALID,NULL,(uint32_t *)&bulk_in_recv, msc_obj_ptr->class_specific_callback.arg);
            }
            else
            {
                LBA_APP_STRUCT lba_data;        
                msc_obj_ptr->current_offset += event->len;
                lba_data.offset = msc_obj_ptr->current_offset;
                lba_data.size = (msc_obj_ptr->transfer_remaining > 
                    MSD_SEND_BUFFER_SIZE ? MSD_SEND_BUFFER_SIZE:
                    msc_obj_ptr->transfer_remaining); 
                /* whichever is smaller */
                lba_data.buff_ptr = msc_lba_send_ptr;
                /* fetch data from MSD App(e.g)ram disk or Sd card, etc*/
                msc_obj_ptr->class_specific_callback.callback(event_type,
                    USB_REQ_VAL_INVALID,NULL,(uint32_t *)&lba_data, msc_obj_ptr->class_specific_callback.arg);
                /* Send data on USB Bus */
                (void)USB_MSC_Bulk_Send_Data(msc_obj_ptr->msc_handle, 
                    (uint8_t *)lba_data.buff_ptr,lba_data.size);    
            }
        }        
        
        if(!msc_obj_ptr->transfer_remaining)
        {   /* marks the end of data phase */
            msc_obj_ptr->in_flag = FALSE; /* clear the flag for next CBW */         
            /* Send the command status information */
            (void)USB_MSC_Bulk_Send_Data(msc_obj_ptr->msc_handle, 
                (uint8_t *)msc_obj_ptr->csw_ptr, MSC_CSW_LENGTH);                  
        }
    } 
    else if ((event->len == MSC_CSW_LENGTH) /* CSW is 13 bytes in length */
        && (csw_ptr->signature == USB_DCSWSIGNATURE)) /*valid CSW signature*/                 
    {
        /* this flag will now be set on reset or after CSW being sent */
        msc_obj_ptr->cbw_valid_flag = TRUE;        
        /* prepare for next CBW */  
        (void)USB_MSC_Bulk_Recv_Data(msc_obj_ptr->controller_handle,
            (uint8_t*)msc_obj_ptr->cbw_ptr, MSC_CBW_LENGTH);
    }     
}

/**************************************************************************//*!
 *
 * @name  USB_Service_Bulk_Out
 *
 * @brief The funtion ic callback function of DIC Bulk Out Endpoint 
 *
 * @param event
 * @param arg
 * @return None       
 *
 *****************************************************************************/
void USB_Service_Bulk_Out(PTR_USB_EVENT_STRUCT event,void* arg)
{       
    uint8_t event_type; 
    PTR_CBW cbw_ptr = NULL;
    uint8_t error;
    MSC_DEVICE_STRUCT_PTR msc_obj_ptr;
	//uint32_t signature = USB_DCBWSIGNATURE;
	//printf("\nsignautre is :%x",signature);
	
    
    msc_obj_ptr = (MSC_DEVICE_STRUCT_PTR)arg;  
    
    if (NULL == msc_obj_ptr)
    {
        #if _DEBUG
            printf("USB_Service_Bulk_Out: msc_dev_ptr is NULL\n");
        #endif  
        return;
    }
         
     /* check if there is need to stall BULK IN ENDPOINT */
    if( (msc_obj_ptr->out_stall_flag == TRUE)&&
        (!msc_obj_ptr->transfer_remaining))
    {
        //uint8_t component = (uint8_t)(event->ep_num | 
        //    (USB_RECV<<COMPONENT_PREPARE_SHIFT));
        msc_obj_ptr->out_stall_flag = FALSE; /* clear the flag */
        msc_obj_ptr->out_flag = FALSE; /* clear send flag */
        usb_device_stall_endpoint(msc_obj_ptr->controller_handle,event->ep_num,USB_SEND);
        return;                 
    }
    
    /* If its not a data phase on bulk endpoint */
    if ((!msc_obj_ptr->out_flag) && (event->len == MSC_CBW_LENGTH)) 
    {
        cbw_ptr = (PTR_CBW)(event->buffer_ptr);
    } 

    if(msc_obj_ptr->out_flag) /* bulk out transaction has occured after CBW */
    {           
        if(msc_obj_ptr->class_specific_callback.callback != NULL) 
        {
            event_type = (uint8_t)(msc_obj_ptr->transfer_remaining?
                USB_MSC_DEVICE_WRITE_REQUEST : USB_DEV_EVENT_DATA_RECEIVED);
                                        
            if(event_type == USB_DEV_EVENT_DATA_RECEIVED)
            {
                MSC_APP_DATA_STRUCT bulk_out_recv;                  
                bulk_out_recv.data_ptr = event->buffer_ptr;
                bulk_out_recv.data_size = event->len; 
                
                msc_obj_ptr->class_specific_callback.callback(event_type,
                    USB_REQ_VAL_INVALID,NULL,(uint32_t *)&bulk_out_recv, msc_obj_ptr->class_specific_callback.arg);
            }
            else
            {
                LBA_APP_STRUCT lba_data1;
                lba_data1.buff_ptr = msc_lba_recv_ptr;
                lba_data1.offset = msc_obj_ptr->current_offset;
                lba_data1.size = event->len;                                
                /* copy the data received on USB Bus, to MSD Disk 
                   (e.g.) CD/DVD or SD Card, etc */
                msc_obj_ptr->class_specific_callback.callback(event_type,
                    USB_REQ_VAL_INVALID,NULL,(uint32_t *)&lba_data1, msc_obj_ptr->class_specific_callback.arg);
            }            
        }
        
        if(msc_obj_ptr->transfer_remaining >= event->len)
        {   /* decrement the global count */
            msc_obj_ptr->transfer_remaining -= event->len;
                                  
            if((msc_obj_ptr->out_stall_flag == TRUE)&&
               (!msc_obj_ptr->transfer_remaining))
            {
                //uint8_t component = (uint8_t)(event->ep_num | 
                //    (event->direction<<COMPONENT_PREPARE_SHIFT));
                msc_obj_ptr->out_stall_flag = FALSE; /* clear the flag */
                msc_obj_ptr->out_flag = FALSE; /* clear send flag */
                usb_device_stall_endpoint(msc_obj_ptr->controller_handle,event->ep_num,USB_SEND);
                return;                 
            }
        }
        
        if(msc_obj_ptr->transfer_remaining)
        {   /* Transfers Remaining -- Again invoke recv_data */         
            LBA_APP_STRUCT lba_data0;           
            msc_obj_ptr->current_offset += event->len;
            lba_data0.size = (msc_obj_ptr->transfer_remaining > MSD_RECV_BUFFER_SIZE) ? 
                MSD_RECV_BUFFER_SIZE : msc_obj_ptr->transfer_remaining; /* whichever is smaller */
            lba_data0.buff_ptr = msc_lba_recv_ptr;                
            (void)USB_MSC_Bulk_Recv_Data(msc_obj_ptr->controller_handle,
                lba_data0.buff_ptr,lba_data0.size);
            return;
        }
        else
        {   /* marks the end of data phase */
            msc_obj_ptr->out_flag = FALSE; /* clear the flag for next CBW */
            /* Send the command status information */
            (void)USB_MSC_Bulk_Send_Data(msc_obj_ptr->msc_handle, 
                (uint8_t *)msc_obj_ptr->csw_ptr, MSC_CSW_LENGTH); 
        }
    } 
    else if(/* check for valid and meaningful CBW */
        /* CBW received after device had sent a CSW or after a reset */
        (msc_obj_ptr->cbw_valid_flag)         
        /* CBW is 31 bytes in length */
        && (event->len == MSC_CBW_LENGTH) 
        /* valid CBW signature*/
        && (cbw_ptr != NULL) && (cbw_ptr->signature == USB_DCBWSIGNATURE) 
        /* all reserved bits should be zero*/
        && (!((cbw_ptr->lun & 0xF0) || (cbw_ptr->cb_length & 0xE0)))         
        /* host should send command to valid LUN */
        && (cbw_ptr->lun<msc_obj_ptr->device_info.num_lun_supported)                                    
        /* valid cbwcb length*/
        && ((cbw_ptr->cb_length >= 0x01)&&(cbw_ptr->cb_length <= 0x10))                                                     
          )
    {
        msc_obj_ptr->re_stall_flag = TRUE;
        /* A valid CBW was received */
        msc_obj_ptr->csw_ptr->signature = USB_DCSWSIGNATURE;
        msc_obj_ptr->csw_ptr->tag = cbw_ptr->tag;
        /*this flag will now be set on reset or after CSW being sent */
        msc_obj_ptr->cbw_valid_flag = FALSE;
        
        cbw_ptr->data_length = USB_LONG_LE_TO_HOST(cbw_ptr->data_length);
        
        /* set flag if device is going to recv data in coming transaction */
        msc_obj_ptr->out_flag = (bool)(( (!(cbw_ptr->flag & USB_CBW_DIRECTION_BIT))
            && (cbw_ptr->data_length))?TRUE:FALSE);
        /* set flag if send is going to send data in coming transaction */
        msc_obj_ptr->in_flag = (bool)(( (cbw_ptr->flag & USB_CBW_DIRECTION_BIT)
            && (cbw_ptr->data_length))?TRUE:FALSE);                
        /* Process the command */
        error = process_mass_storage_command(msc_obj_ptr, cbw_ptr, 
            &(msc_obj_ptr->csw_ptr->residue), &(msc_obj_ptr->csw_ptr->csw_status));
        
        if(error == USBERR_ENDPOINT_STALLED)
        {
            if(msc_obj_ptr->out_flag==TRUE)
            {
                msc_obj_ptr->out_stall_flag = TRUE; 
                msc_obj_ptr->out_flag = FALSE; /* so as to send status phase */
            }
            else if(msc_obj_ptr->in_flag==TRUE)
            {
                msc_obj_ptr->in_stall_flag = TRUE;  
            }           
        }
        
        /* if there is no data phase on bulk endpoints*/
        if(!(msc_obj_ptr->out_flag || msc_obj_ptr->in_flag)) 
        {   /* Send the command status information */
            (void)USB_MSC_Bulk_Send_Data(msc_obj_ptr->msc_handle, 
                (uint8_t *)msc_obj_ptr->csw_ptr, MSC_CSW_LENGTH);         
        }
    } 
    else /* Invalid/NMreceived */
    {        
        //uint8_t direction;
        //uint8_t ep_num;
        /* prepare the component to be sent in lower layer with 
           endpoint number and direction*/
        //uint8_t component;
        
        //direction  = USB_RECV; 
        //ep_num = msc_obj_ptr->bulk_out_endpoint;
        //component = (uint8_t)(ep_num | (direction<<COMPONENT_PREPARE_SHIFT));
        usb_device_stall_endpoint(msc_obj_ptr->controller_handle,msc_obj_ptr->bulk_out_endpoint,USB_RECV);
        usb_device_stall_endpoint(msc_obj_ptr->controller_handle,msc_obj_ptr->bulk_in_endpoint,USB_SEND);
        msc_obj_ptr->cbw_valid_flag = FALSE;
    }    
}


/**************************************************************************//*!
 *
 * @name  USB_Class_MSC_Event
 *
 * @brief The funtion initializes MSC endpoints 
 *
 * @param arg             handle to Identify class object.
 * @param event           pointer to event structure
 * @param val             gives the configuration value 
 *
 * @return None       
 *
 *****************************************************************************/
 void USB_Class_MSC_Event(uint8_t event, void* val,void* arg) 
{   
	USB_CLASS_STRUCT_PTR usbclassPtr;
    MSC_DEVICE_STRUCT_PTR msc_dev_ptr;
    
    #if IMPLEMENT_QUEUING    
        uint8_t index;
    #endif    
    USB_ENDPOINTS *usb_ep_data;
	//uint8_t error;
    
    //printf("\n USB_Class_MSD_Event: event: 0x%x", event);
    msc_dev_ptr = (MSC_DEVICE_STRUCT_PTR)arg;
    
    if (NULL == msc_dev_ptr)
    {
        #if _DEBUG
            printf("USB_Class_MSC_Event: msc_dev_ptr is NULL\n");
        #endif  
        return;
    }
    
    
        
    if(event == USB_DEV_EVENT_CONFIG_CHANGED)
    {
        uint8_t count = 0;
        
        //printf("\n 000, count: 0x%x", count);
        //printf("\nUSB_DEV_EVENT_CONFIG_CHANGED");
		

#if USBCFG_DEV_COMPOSITE
        /* Set configuration according to config number*/
        msc_dev_ptr->desc_callback_ptr->SET_CONFIGURATION((uint32_t)msc_dev_ptr->controller_handle, (uint8_t)(*(uint16_t *)val));
        /* Get class info */
        USB_Msd_Get_Desc_Info(msc_dev_ptr, USB_MSD_CLASS_INFO, (uint32_t *)&usbclassPtr);
        USB_Msd_Get_Desc_Info(msc_dev_ptr, USB_MSD_INTERFACE_COUNT, &msc_dev_ptr->usb_max_suported_interfaces);
#else
		msc_dev_ptr->desc_callback_ptr->GET_DESC_ENTITY((uint32_t)msc_dev_ptr->controller_handle,
								USB_CLASS_INFO, (uint32_t*)&usbclassPtr);
		USB_Msd_Get_Desc_Info(msc_dev_ptr, USB_MSD_INTERFACE_COUNT, &msc_dev_ptr->usb_max_suported_interfaces);
#endif
		#if 0
        
        msc_dev_ptr->ep_desc_data = (struct _USB_ENDPOINTS *) &usbclassPtr->interfaces.interface->endpoints;
        msc_dev_ptr->msc_endpoint_data.count = usbclassPtr->interfaces.interface->endpoints.count;
       
        msc_dev_ptr->msc_endpoint_data.ep = (USB_CLASS_MSC_ENDPOINT *)OS_Mem_alloc_zero(usbclassPtr->interfaces.interface->endpoints.count* sizeof(USB_CLASS_MSC_ENDPOINT));

        /* get the endpoints from the descriptor module */            
        usb_ep_data = msc_dev_ptr->ep_desc_data; 
         
		#endif

		usb_ep_data = &(usbclassPtr[*(uint16_t*)val - 1].interfaces.interface[0].endpoints);
		msc_dev_ptr->ep_desc_data = usb_ep_data;
		msc_dev_ptr->msc_endpoint_data.count = usbclassPtr->interfaces.interface->endpoints.count;
		       
		msc_dev_ptr->msc_endpoint_data.ep = (USB_CLASS_MSC_ENDPOINT *)OS_Mem_alloc_zero(usbclassPtr->interfaces.interface->endpoints.count* sizeof(USB_CLASS_MSC_ENDPOINT));
		msc_dev_ptr->bulk_in_endpoint = usbclassPtr->interfaces.interface->endpoints.ep[0].ep_num;
		msc_dev_ptr->bulk_in_endpoint_packet_size = usbclassPtr->interfaces.interface->endpoints.ep[0].size;
		msc_dev_ptr->bulk_out_endpoint = usbclassPtr->interfaces.interface->endpoints.ep[1].ep_num;
		
		
        #if IMPLEMENT_QUEUING
                 #if _DEBUG
        		     printf("\n 111, count: 0x%x", count);
                 #endif
        		 for(index = 0; index < usb_ep_data->count; index++) 
        		 {			  
        			 msc_dev_ptr->msc_endpoint_data.ep[index].endpoint = usbclassPtr->interfaces.interface->endpoints.ep[index].ep_num;
        			 msc_dev_ptr->msc_endpoint_data.ep[index].type = usbclassPtr->interfaces.interface->endpoints.ep[index].type;		
        			 msc_dev_ptr->msc_endpoint_data.ep[index].bin_consumer = 0x00;		  
        			 msc_dev_ptr->msc_endpoint_data.ep[index].bin_producer = 0x00;		 
        		  }
        #endif
        		     
        /* intialize all non control endpoints */            
        while(count < usb_ep_data->count) 
        {
            #if _DEBUG
        	    printf("\n 222, count: 0x%x", count);
            #endif
            USB_EP_STRUCT_PTR ep_struct_ptr= 
                (USB_EP_STRUCT_PTR) (&usb_ep_data->ep[count]);

            (void)usb_device_init_endpoint(msc_dev_ptr->controller_handle,
                ep_struct_ptr, TRUE);
            
            /* register callback service for Non Control EndPoints */
            if(ep_struct_ptr->type == USB_BULK_PIPE) 
            {             
                if(ep_struct_ptr->direction == USB_RECV) 
                {
                    (void)usb_device_register_service(msc_dev_ptr->controller_handle,
                        (uint8_t)(USB_SERVICE_EP0+ep_struct_ptr->ep_num),
                        USB_Service_Bulk_Out,(void *)msc_dev_ptr);
                } 
                else
                {
                    (void)usb_device_register_service(msc_dev_ptr->controller_handle,
                        (uint8_t)(USB_SERVICE_EP0+ep_struct_ptr->ep_num),
                        USB_Service_Bulk_In,(void *)msc_dev_ptr);
                }            
            }
            count++;                           

        }
        msc_dev_ptr->re_stall_flag = FALSE;
        msc_dev_ptr->out_flag = FALSE;
        msc_dev_ptr->in_flag = FALSE;
        msc_dev_ptr->out_stall_flag = FALSE;
        msc_dev_ptr->in_stall_flag = FALSE;
        msc_dev_ptr->cbw_valid_flag = TRUE; /*making the first CBW valid */ 
        msc_dev_ptr->transfer_remaining = 0; 
                
    }
	else if(event == USB_DEV_EVENT_ENUM_COMPLETE) 
    {
    	/* To Do */
        if(NULL != msc_dev_ptr->cbw_ptr)
        {
            OS_Mem_free(msc_dev_ptr->cbw_ptr);
            msc_dev_ptr->cbw_ptr = NULL;
        }
		msc_dev_ptr->cbw_ptr = (PTR_CBW)OS_Mem_alloc_uncached(MSC_CBW_LENGTH);
		(void)usb_device_recv_data(msc_dev_ptr->controller_handle,2,(uint8_t*)msc_dev_ptr->cbw_ptr,MSC_CBW_LENGTH);
		#if _DEBUG
            printf("\nusb_device_recv_data");
        #endif
		//msc_dev_ptr->csw_ptr = (PTR_CSW)OS_mem_alloc_uncached(MSC_CSW_LENGTH);
		//USB_Class_MSC_Send_Data(msc_dev_ptr->controller_handle,1,(uint8_t *)msc_dev_ptr->csw_ptr,MSC_CSW_LENGTH);
    }
    else if(event == USB_DEV_EVENT_BUS_RESET)
    {
        #if IMPLEMENT_QUEUING
            for(index = 0; index < usb_ep_data->count; index++) 
            {            
                msc_dev_ptr->ep[index].bin_consumer = 0x00;         
                msc_dev_ptr->ep[index].bin_producer = 0x00;        
            }
        #endif 
    }
    else if(event == USB_DEV_EVENT_EP_UNSTALLED)
    {
        uint8_t value;
        value = *((uint8_t *)val);
        
        if( (msc_dev_ptr->re_stall_flag == TRUE)
            &&
            (((value & 0x0F) == msc_dev_ptr->bulk_in_endpoint) || 
            ((value & 0x0F) == msc_dev_ptr->bulk_out_endpoint)))
        {   /* For MASS Storage Class BULK ENDPOINTS have to be unstalled 
               only on receiving Bulk Only Reset. 
               Therefore, if Host sends clear feature to unstall these 
               endpoints, re-stall them
             */     
             usb_device_stall_endpoint(msc_dev_ptr->controller_handle,value & 0x0F, (value & 0x80) >> 7);
        }       
    }
    else if(event == USB_DEV_EVENT_EP_STALLED)
    {
        /* Code to be added here, 
           if there is some action needed at app level */           
    }
            
    if(msc_dev_ptr->msc_application_callback.callback != NULL) 
    {
        msc_dev_ptr->msc_application_callback.callback(event,val,
            msc_dev_ptr->msc_application_callback.arg);
    } 
}

/**************************************************************************//*!
 *
 * @name  USB_MSC_Other_Requests
 *
 * @brief The funtion provides flexibilty to add class and vendor specific
 *        requests 
 *
 * @param arg
 * @param setup_packet:     setup packet recieved      
 * @param data:             data to be send back
 * @param size:             size to be returned    
 *
 * @return status:       
 *                        USB_OK : When Successfull       
 *                        Others : When Error
 *
 *****************************************************************************/
uint8_t USB_MSC_Requests
(
    USB_SETUP_STRUCT * setup_packet, 
    uint8_t * *data, 
    uint32_t *size,
    void* arg
) 
{
    MSC_DEVICE_STRUCT_PTR msc_dev_ptr;
    uint8_t error = USBERR_INVALID_REQ_TYPE;  
     
    msc_dev_ptr = (MSC_DEVICE_STRUCT_PTR)arg;
    //printf("\nsetup_packet->request: 0x%x", setup_packet->request);
    
    if (NULL == msc_dev_ptr)
    {
        #if _DEBUG
            printf("USB_MSC_Other_Requests: msc_dev_ptr is NULL\n");
        #endif  
        return USBERR_ERROR;
    }
    
    *size=0;   
 
     if((setup_packet->request_type & USB_DEV_REQ_STD_REQUEST_TYPE_TYPE_POS) == 
        USB_DEV_REQ_STD_REQUEST_TYPE_TYPE_CLASS) 
     {  /* class request so handle it here */
     
        error = USB_OK;
        
        /* call for class/subclass specific requests */
        switch(setup_packet->request) 
        {
            case GET_MAX_LUN :
                if((setup_packet->index < msc_dev_ptr->usb_max_suported_interfaces) &&
                    (!setup_packet->value)&&(setup_packet->length == 0x0001) &&
                    ((setup_packet->request_type & USB_DEV_REQ_STD_REQUEST_TYPE_DIR_POS) == USB_DEV_REQ_STD_REQUEST_TYPE_DIR_IN)
                  ) 
                {
                    /* return the maximum number of logical units supported */
                    *data = &msc_dev_ptr->lun;
                    *size=1;
                    msc_dev_ptr->re_stall_flag = TRUE;
                }
                else 
                {   /* for Get Max LUN request with invalid wIndex parameter, 
                       host expects stall */
                    error = USBERR_INVALID_REQ_TYPE; 
                }              
              break;
            case BULK_ONLY_MASS_STORAGE_RESET :
                /* Steps to be taken in this command :
                1) ready the device for the next CBW from the host
                2) preserve the value of its bulk data toggle bits 
                3) preserve the value of its bulk endpoint STALL conditions
                4) device shall NAK the status stage of device request until 
                   command is complete*/

                if( (setup_packet->index < msc_dev_ptr->usb_max_suported_interfaces) &&
                    (!setup_packet->value)&&(!setup_packet->length) &&
                    ((setup_packet->request_type & USB_DEV_REQ_STD_REQUEST_TYPE_DIR_POS) == USB_DEV_REQ_STD_REQUEST_TYPE_DIR_OUT)
                   )
                {
                    /* get the endpoints from the descriptor module */            
                    USB_ENDPOINTS *usb_ep_data = msc_dev_ptr->ep_desc_data;
                    uint8_t count = 0;

                    /* deinitialize and intialize bulk endpoints */            
                    while(count < usb_ep_data->count) 
                    {
                        USB_EP_STRUCT_PTR ep_struct_ptr= 
                            (USB_EP_STRUCT_PTR) (&usb_ep_data->ep[count]);
                        
                        if(ep_struct_ptr->type == USB_BULK_PIPE)
                        {
                            (void)usb_device_deinit_endpoint(msc_dev_ptr->controller_handle,
                                ep_struct_ptr->ep_num, ep_struct_ptr->direction);
                            (void)usb_device_init_endpoint(msc_dev_ptr->controller_handle,
                                ep_struct_ptr, TRUE);
                        }
                        count++;
                    }
                    msc_dev_ptr->out_flag = FALSE;
                    msc_dev_ptr->in_flag = FALSE;
                    msc_dev_ptr->out_stall_flag = FALSE;
                    msc_dev_ptr->in_stall_flag = FALSE;
                    /*making the first CBW valid */
                    msc_dev_ptr->cbw_valid_flag = TRUE;                
                    msc_dev_ptr->re_stall_flag = FALSE;
                    msc_dev_ptr->transfer_remaining = 0;
                }
                else 
                {   /* for Get Max LUN request with invalid wIndex parameter, 
                       host expects stall */
                    error = USBERR_INVALID_REQ_TYPE; 
                }    
                break;           
            default :break;        
        }
    } 
    else if((setup_packet->request_type & USB_DEV_REQ_STD_REQUEST_TYPE_TYPE_POS) == 
        USB_DEV_REQ_STD_REQUEST_TYPE_TYPE_VENDOR) 
    {   /* vendor specific request  */    
        if(msc_dev_ptr->vendor_req_callback.callback != NULL) 
        {
            error = msc_dev_ptr->vendor_req_callback.callback(setup_packet,
            data,size,msc_dev_ptr->vendor_req_callback.arg);
        }
    }
     
    return error;
}


/*****************************************************************************
 * Global Functions
 *****************************************************************************/


/**************************************************************************//*!
 *
 * @name  USB_Class_MSC_Init
 *
 * @brief The funtion initializes the Device and Controller layer 
 *
 * @param msd_config_ptr    : Configuration paramemter strucutre pointer
 *                            passed by APP.
 * @return status       
 *         MSD Handle           : When Successfull 
 *         Others           : Errors
 ******************************************************************************
 *
 *This function initializes the MSC Class layer and layers it is dependent on 
 ******************************************************************************/
USB_STATUS USB_Class_MSC_Init
(
	uint8_t controller_id,
	USB_MSD_CONFIG_STRUCT_PTR msd_config_ptr,
	MSD_HANDLE *  msd_handle

) 
{
    #if IMPLEMENT_QUEUING    
        uint8_t index;
    #endif
        
    #if _DEBUG
        printf("Eneter USB_Class_MSC_Init\n");
    #endif
    uint8_t error = USBERR_ERROR;
	uint32_t implementing_disk_drive = IMPLEMENTING_DISK_DRIVE;
    MSC_DEVICE_STRUCT_PTR devicePtr = NULL;
    USB_CLASS_SPECIFIC_HANDLER_CALLBACK_STRUCT_PTR scsi_cb_ptr;
    //USB_ENDPOINTS *usb_ep_data;
    PTR_DEVICE_LBA_INFO_STRUCT usb_msd_lba_info_ptr = NULL;
	
  
    if (msd_config_ptr == NULL)
    {
        return USBERR_ERROR;    
    }
    
    devicePtr = (MSC_DEVICE_STRUCT_PTR)OS_Mem_alloc_zero(sizeof(MSC_DEVICE_STRUCT));
    if (NULL == devicePtr)
    {
        #if _DEBUG
            printf("1: USB_Class_MSC_Init: Memalloc failed\n");
        #endif  
        return USBERR_ALLOC;
    }
    
//    devicePtr->desc_callback_ptr = (DESC_REQUEST_NOFIFY_STRUCT_PTR)OS_Mem_alloc_zero(sizeof(DESC_REQUEST_NOFIFY_STRUCT));
//	if (NULL == devicePtr->desc_callback_ptr)
//	 {
//		#ifdef _DEV_DEBUG
//			printf("USB_Class_Msd_Init: desc_callback_ptr Memalloc failed\n");
//		#endif
//		OS_Mem_free(devicePtr);
//		return USBERR_ALLOC;
//	 }

//	usb_msd_lba_info_ptr = (PTR_DEVICE_LBA_INFO_STRUCT)OS_Mem_alloc_zero(sizeof(DEVICE_LBA_INFO_STRUCT));
//	if(NULL == usb_msd_lba_info_ptr)
//		{
//			return USBERR_ALLOC;
//		}
	//devicePtr->hid_endpoint_data.ep = NULL;
#if 0
        /* initialize the Global Variable Structure */
    OS_Mem_zero(devicePtr, sizeof(MSC_DEVICE_STRUCT));
     /* Initialize number of Logical Units */
    devicePtr->ep = msd_config_ptr->ep;
    usb_ep_data = msd_config_ptr->ep_desc_data;
    OS_Mem_copy(&msd_config_ptr->device_info, &devicePtr->device_info, 
    
	sizeof(DEVICE_LBA_INFO_STRUCT));
    devicePtr->lun = (uint8_t)(msd_config_ptr->device_info.num_lun_supported - 1);

	
#endif
	//OS_Mem_copy(&msd_config_ptr->msd_buff, &devicePtr->msd_buff, 
   //     sizeof(MSD_BUFF_INFO));
   
   msc_lba_send_ptr = (uint8_t *)OS_Mem_alloc_uncached(MSD_SEND_BUFFER_SIZE);
   if(NULL == msc_lba_send_ptr)
   {
        #if _DEBUG
            printf("2: USB_Class_MSC_Init: Memalloc failed\n");
		#endif
	    error = USBERR_ALLOC;
	    goto error_alloc;
   }

   OS_Mem_zero(msc_lba_send_ptr, MSD_SEND_BUFFER_SIZE);
   
   msc_lba_recv_ptr = (uint8_t *)OS_Mem_alloc_uncached(MSD_RECV_BUFFER_SIZE);
   if(NULL == msc_lba_recv_ptr)
   {
       #if _DEBUG
           printf("3: USB_Class_MSC_Init: Memalloc failed\n");
       #endif
		   error = USBERR_ALLOC;
		   goto error_alloc;
	 }

   OS_Mem_zero(msc_lba_recv_ptr, MSD_RECV_BUFFER_SIZE);
   
#if USBCFG_DEV_COMPOSITE
    devicePtr->class_handle = USB_Class_Get_Class_Handle();
		
    devicePtr->controller_handle = (_usb_device_handle)USB_Class_Get_Ctrler_Handle(devicePtr->class_handle);
    if(NULL == devicePtr->controller_handle)
    {
        #if _DEBUG
            printf("4: USB_Class_MSC_Init: call USB_Class_Get_Ctrler_Handle failed\n");
        #endif
        goto error1;
    }
#else
	/* Initialize the device layer*/
	  error = usb_device_init(controller_id,(&devicePtr->controller_handle));
	  /* +1 is for Control Endpoint */
	  if(error != USB_OK)
	  {
        #if _DEBUG
            printf("4: USB_Class_MSC_Init: call usb_device_init failed\n");
        #endif
		    goto error1;	
	  }
	  /* Initialize the generic class functions */
	  devicePtr->class_handle = USB_Class_Init(devicePtr->controller_handle,USB_Class_MSC_Event,
		  USB_MSC_Requests,(void *)devicePtr,msd_config_ptr->desc_callback_ptr);
#endif


    
    devicePtr->cbw_ptr = NULL; /* Initializing */
    devicePtr->csw_ptr = NULL;
    
    devicePtr->cbw_ptr = (PTR_CBW)OS_Mem_alloc_uncached(MSC_CBW_LENGTH); 
    if (NULL == devicePtr->cbw_ptr)
    {
        #if _DEBUG
            printf("6: USB_Class_MSC_Init: Memalloc failed\n");
        #endif
			  error = USBERR_ALLOC;
		    goto error_alloc;
    }
    OS_Mem_zero(devicePtr->cbw_ptr, MSC_CBW_LENGTH);
    
    devicePtr->csw_ptr = (PTR_CSW)OS_Mem_alloc_uncached(MSC_CSW_LENGTH); 
    if (NULL == devicePtr->csw_ptr)
    {
        #if _DEBUG
            printf("7: USB_Class_MSC_Init: Memalloc failed\n");
        #endif  
			  error = USBERR_ALLOC;
		    goto error_alloc;
    }
    OS_Mem_zero(devicePtr->csw_ptr, MSC_CSW_LENGTH);
    #if 0
    devicePtr->ep_desc_data = usb_ep_data;
    devicePtr->bulk_in_endpoint =   msd_config_ptr->bulk_in_endpoint;
    devicePtr->bulk_in_endpoint_packet_size =   
        msd_config_ptr->bulk_in_endpoint_packet_size;
    devicePtr->bulk_out_endpoint = 
        msd_config_ptr->bulk_out_endpoint;
    devicePtr->usb_max_suported_interfaces = 
        msd_config_ptr->usb_max_suported_interfaces;
	#endif
                                                           
    #if 0
        for(index = 0; index < usb_ep_data->count; index++) 
        {            
            devicePtr->ep[index].endpoint = usb_ep_data->ep[index].ep_num;
            devicePtr->ep[index].type = usb_ep_data->ep[index].type;        
            devicePtr->ep[index].bin_consumer = 0x00;         
            devicePtr->ep[index].bin_producer = 0x00;        
        }
    #endif
    
    /* Save the dec  callback to ask application for class specific params*/
    devicePtr->desc_callback_ptr = (DESC_REQUEST_NOFIFY_STRUCT_PTR)OS_Mem_alloc_uncached(sizeof(DESC_REQUEST_NOFIFY_STRUCT)); 
    if (NULL == devicePtr->desc_callback_ptr)
    {
        #if _DEBUG
            printf("8: USB_Class_MSC_Init: Memalloc failed\n");
        #endif
			  error = USBERR_ALLOC;
		    goto error_alloc;
    }
		OS_Mem_copy(msd_config_ptr->desc_callback_ptr,
    devicePtr->desc_callback_ptr ,sizeof(DESC_REQUEST_NOFIFY_STRUCT)); 
        	
    /* save the callback pointer */
    devicePtr->msc_application_callback.callback = msd_config_ptr->msc_application_callback.callback;
    devicePtr->msc_application_callback.arg = msd_config_ptr->msc_application_callback.arg;
           
    /* save the callback pointer */
    devicePtr->vendor_req_callback.callback =
        msd_config_ptr->vendor_req_callback.callback;
    devicePtr->vendor_req_callback.arg = msd_config_ptr->vendor_req_callback.arg;
           
    /* save the callback pointer */
    devicePtr->class_specific_callback.callback = msd_config_ptr->class_specific_callback.callback;           
    devicePtr->class_specific_callback.arg = msd_config_ptr->class_specific_callback.arg;


    devicePtr->desc_callback_ptr->GET_DESC_ENTITY((uint32_t)devicePtr->controller_handle,USB_MSC_LBA_INFO, (uint32_t *)&usb_msd_lba_info_ptr);
    if(NULL == usb_msd_lba_info_ptr)
    {
	    #if _DEBUG
            printf("9: USB_Class_MSC_Init: get msd lba info failed\n");
        #endif
        goto error2;
    }
    devicePtr->device_info.length_of_each_lab_of_device = usb_msd_lba_info_ptr->length_of_each_lab_of_device;
    devicePtr->device_info.num_lun_supported = usb_msd_lba_info_ptr->num_lun_supported;
    devicePtr->device_info.total_lba_device_supports = usb_msd_lba_info_ptr->total_lba_device_supports;

	/* Initialize the scsi subclass functions */
    //scsi_cb.callback = USB_Class_MSC_Event;
    scsi_cb_ptr = &msd_config_ptr->class_specific_callback;
    //scsi_cb.arg   = devicePtr; 
    error = USB_MSC_SCSI_Init(devicePtr,scsi_cb_ptr, &devicePtr->device_info,
    		implementing_disk_drive); 
    
    if(error != USB_OK)
    {
        #if _DEBUG
            printf("10: USB_Class_MSC_Init: call USB_MSC_SCSI_Init failed\n");
        #endif
        goto error2;  
    }

#if 0
	devicePtr->device_info.total_lba_device_supports = msd_config_ptr->device_info.total_lba_device_supports;
	devicePtr->device_info.length_of_each_lab_of_device = msd_config_ptr->device_info.length_of_each_lab_of_device;
	devicePtr->device_info.num_lun_supported = msd_config_ptr->device_info.num_lun_supported;
#endif
    
    		
    *msd_handle = (unsigned long)devicePtr;
    devicePtr->msc_handle = *msd_handle;

    return USB_OK;
   
    error2:
        /* Implement Denit class and invoke here*/    
    error1:

    error_alloc:
        if(NULL != devicePtr->msc_endpoint_data.ep)
        {
            OS_Mem_free(devicePtr->msc_endpoint_data.ep);
            devicePtr->msc_endpoint_data.ep = NULL;
        }
        if(NULL != devicePtr->csw_ptr)
        {
            OS_Mem_free(devicePtr->csw_ptr);
            devicePtr->csw_ptr = NULL;
        }        
        if(NULL != devicePtr->cbw_ptr)
        {
            OS_Mem_free(devicePtr->cbw_ptr);
            devicePtr->cbw_ptr = NULL;
        }
        if(NULL != devicePtr->desc_callback_ptr)
        {
            OS_Mem_free(devicePtr->desc_callback_ptr);
            devicePtr->desc_callback_ptr = NULL;
        }
        if(NULL != devicePtr)
        {
            OS_Mem_free(devicePtr);
            devicePtr = NULL;
        }
        
        if(NULL != msc_lba_send_ptr)
        {
            OS_Mem_free(msc_lba_send_ptr);
            msc_lba_send_ptr = NULL;
        }
        if(NULL != msc_lba_recv_ptr)
        {
            OS_Mem_free(msc_lba_recv_ptr);
            msc_lba_recv_ptr = NULL;
        }

    return error;     
}

/**************************************************************************//*!
 *
 * @name  USB_Class_MSC_Deinit
 *
 * @brief The funtion initializes the Device and Controller layer 
 *
 * @param cdc_handle
 *
 * @return status       
 *         USB_OK           : When Successfull 
 *         Others           : Errors
 ******************************************************************************
 *
 *This function initializes the MSC Class layer and layers it is dependednt on 
 *
 *****************************************************************************/
USB_STATUS USB_Class_MSC_Deinit
(
  MSD_HANDLE msd_handle /*[IN]*/
)
{
    uint8_t error = USB_OK;
    MSC_DEVICE_STRUCT_PTR devicePtr = NULL;
    
    if (msd_handle == 0)
		return USBERR_ERROR;
        
    devicePtr = USB_Msd_Get_Device_Ptr(msd_handle);
    
    if(error == USB_OK)
        /* Deinitialize the scsi subclass functions */
        error = USB_MSC_SCSI_Deinit(devicePtr);

#if !USBCFG_DEV_COMPOSITE
    if(error == USB_OK)
        /* Deinitialize the generic class functions */
        error = USB_Class_Deinit(devicePtr->controller_handle,devicePtr->class_handle);
    if(error == USB_OK)
        /* Deinitialize the device layer*/
        error = usb_device_deinit(devicePtr->controller_handle);
#endif
    if(error == USB_OK)
    {
        if(NULL != devicePtr->msc_endpoint_data.ep)
        {
            OS_Mem_free(devicePtr->msc_endpoint_data.ep);
            devicePtr->msc_endpoint_data.ep = NULL;
        }
        if(NULL != devicePtr->csw_ptr)
        {
            OS_Mem_free(devicePtr->csw_ptr);
            devicePtr->csw_ptr = NULL;
        }        
        if(NULL != devicePtr->cbw_ptr)
        {
            OS_Mem_free(devicePtr->cbw_ptr);
            devicePtr->cbw_ptr = NULL;
        }
        if(NULL != devicePtr->desc_callback_ptr)
        {
            OS_Mem_free(devicePtr->desc_callback_ptr);
            devicePtr->desc_callback_ptr = NULL;
        }
        if(NULL != devicePtr)
        {
            OS_Mem_free(devicePtr);
            devicePtr = NULL;
        }
        
        if(NULL != msc_lba_send_ptr)
        {
            OS_Mem_free(msc_lba_send_ptr);
            msc_lba_send_ptr = NULL;
        }
        if(NULL != msc_lba_recv_ptr)
        {
            OS_Mem_free(msc_lba_recv_ptr);
            msc_lba_recv_ptr = NULL;
        }
    }
    return error;    
}

/**************************************************************************//*!
 *
 * @name  USB_Class_MSC_Send_Data
 *
 * @brief 
 *
 * @param msc_handle   :   handle returned from USB_Class_MSC_Init
 * @param ep_num          :   endpoint num 
 * @param app_buff        :   buffer to send
 * @param size            :   length of the transfer   
 *
 * @return status       
 *         USB_OK           : When Successfull 
 *         Others           : Errors
 *****************************************************************************/
uint8_t USB_Class_MSC_Send_Data
(
    MSD_HANDLE msc_handle, /*[IN]*/
    uint8_t ep_num, /*[IN]*/
    uint8_t * app_buff, /*[IN]*/
    uint32_t size /*[IN]*/
) 
{
    MSC_DEVICE_STRUCT_PTR devicePtr;
    #if IMPLEMENT_QUEUING    
        uint8_t index;
        uint8_t producer, consumer; 
        USB_ENDPOINTS *usb_ep_data;
    #endif
    uint8_t error = USB_OK;


    //if (/*(msc_handle < 0) || */ (msc_handle > MAX_MSC_DEVICE))
    //   return USBERR_ERROR;
      
    devicePtr = USB_Msd_Get_Device_Ptr(msc_handle);
    if (devicePtr == NULL)
        return USBERR_ERROR;

    #if IMPLEMENT_QUEUING    
 
        usb_ep_data = devicePtr->ep_desc_data; 
      
        /* map the endpoint num to the index of the endpoint structure */
        for(index = 0; index < usb_ep_data->count; index++) 
        {
            if(usb_ep_data->ep[index].ep_num == ep_num)
            break;
        }
        
        producer = devicePtr->ep[index].bin_producer;
        consumer = devicePtr->ep[index].bin_consumer;
                                
        if(((uint8_t)(producer - consumer)) != (uint8_t)(MSD_MAX_QUEUE_ELEMS))  
        {
            /* the bin is not full*/        
            uint8_t queue_num = (uint8_t)(producer % MSD_MAX_QUEUE_ELEMS);
            
            /* put all send request parameters in the endpoint data structure*/
            devicePtr->ep[index].queue[queue_num].channel = ep_num;
            devicePtr->ep[index].queue[queue_num].app_data.data_ptr = app_buff;
            devicePtr->ep[index].queue[queue_num].app_data.data_size = size;
            devicePtr->ep[index].queue[queue_num].handle = 
            devicePtr->controller_handle; 
            
            /* increment producer bin by 1*/       
            devicePtr->ep[index].bin_producer = ++producer;
                    
            if((uint8_t)(producer - consumer) == (uint8_t)1)         
            {          
    #endif            
                error = USB_Class_Send_Data(devicePtr->class_handle, 
                    ep_num, app_buff,size);
    #if IMPLEMENT_QUEUING
            }
        }
        else /* bin is full */
        {
            error = USBERR_DEVICE_BUSY; 
        }    
    #endif 
    
    return error;
}

/**************************************************************************//*!
 *
 * @name  USB_MSC_LBA_Transfer
 *
 * @brief 
 *
 * @param MSC_DEVICE_STRUCT_PTR   
 * @param direction       :   transfer direction
 * @param lba_info_ptr    :   buffer to send
 *
 * @return status       
 *         USB_OK           : When Successfull 
 *         Others           : Errors
 *****************************************************************************/
uint8_t USB_MSC_LBA_Transfer
(
    MSC_DEVICE_STRUCT_PTR msc_obj_ptr,
    bool direction,
    PTR_LBA_INFO_STRUCT lba_info_ptr
)
{
    uint8_t error;
    LBA_APP_STRUCT lba_data;    
    
    if((!((lba_info_ptr->starting_lba<msc_obj_ptr->device_info.total_lba_device_supports)&&
        (lba_info_ptr->lba_transfer_num <= (msc_obj_ptr->device_info.total_lba_device_supports - 
        lba_info_ptr->starting_lba)))) || (msc_obj_ptr->class_specific_callback.callback == NULL))
    {
        /* host trying to access region beyond MASS STORAGE SPACE 
           Or no class_specific_callback is registered */
        return(uint8_t)((direction?USBERR_TX_FAILED:USBERR_RX_FAILED));
    }

    msc_obj_ptr->transfer_remaining = lba_info_ptr->lba_transfer_num * 
        msc_obj_ptr->device_info.length_of_each_lab_of_device;              
    msc_obj_ptr->current_offset = lba_info_ptr->starting_lba * 
        msc_obj_ptr->device_info.length_of_each_lab_of_device;
    
    lba_data.offset = msc_obj_ptr->current_offset;

    if(direction == USB_SEND)
    {
        lba_data.size = (msc_obj_ptr->transfer_remaining > MSD_SEND_BUFFER_SIZE) ? 
            MSD_SEND_BUFFER_SIZE : msc_obj_ptr->transfer_remaining; /* whichever is smaller */
        lba_data.buff_ptr = msc_lba_send_ptr;
        
        msc_obj_ptr->class_specific_callback.callback(USB_MSC_DEVICE_READ_REQUEST,
            USB_REQ_VAL_INVALID,NULL,(uint32_t *)&lba_data, msc_obj_ptr->class_specific_callback.arg);

        error = USB_MSC_Bulk_Send_Data(msc_obj_ptr->msc_handle,lba_data.buff_ptr,lba_data.size);
    }
    else
    {
        lba_data.size = (msc_obj_ptr->transfer_remaining > MSD_RECV_BUFFER_SIZE) ? 
            MSD_RECV_BUFFER_SIZE : msc_obj_ptr->transfer_remaining; /* whichever is smaller */
        lba_data.buff_ptr = msc_lba_recv_ptr;
        
        error = USB_MSC_Bulk_Recv_Data(msc_obj_ptr->controller_handle,lba_data.buff_ptr,lba_data.size);
    }               
    return error;
}

#endif /*MSD_CONFIG*/
/* EOF */
