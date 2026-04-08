#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usbd_custom_hid_if.h"
#include "usb_device.h"


void StartStatusMonitorTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartStatusMonitorTask */
  uint32_t loop_count = 0;
  uint8_t usbRecBuf[64] = {0};
  /* Infinite loop */
  for(;;)
  {
    loop_count++;
    // 接收数据
    if (usbRecFlag) {
      usbRecFlag = 0;
      // usbRecBuf = hUsbDeviceFS.pClassData->Report_buf;
      memcpy(usbRecBuf, ((USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData)->Report_buf, 64); 
      int8_t result = USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usbRecBuf, 64);
      if (result != USBD_OK) {
        printf("Status Monitor: USB send failed with code %d\n", result);
      }
    }
    
    osDelay(10);
  }
  /* USER CODE END StartStatusMonitorTask */
}