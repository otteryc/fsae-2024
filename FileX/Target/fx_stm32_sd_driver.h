/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FX_STM32_SD_DRIVER_H
#define FX_STM32_SD_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "fx_api.h"
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

extern TX_SEMAPHORE transfer_semaphore;

/* Exported constants --------------------------------------------------------*/

/* Default timeout used to wait for fx operations */
#define FX_STM32_SD_DEFAULT_TIMEOUT                      (10 * TX_TIMER_TICKS_PER_SECOND)

/* Let the filex low-level driver initialize the SD driver */
#define FX_STM32_SD_INIT                                 1

/* Use the SD DMA API */
#define FX_STM32_SD_DMA_API                              1

/* SDIO instance to be used by FileX */
#define FX_STM32_SD_INSTANCE                             0

/* Default sector size, used by the driver */
#define FX_STM32_SD_DEFAULT_SECTOR_SIZE                  512

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Get the current time in ticks */
/* USER CODE BEGIN FX_STM32_SD_CURRENT_TIME_TX */

#define FX_STM32_SD_CURRENT_TIME()                                   tx_time_get()

/* USER CODE END FX_STM32_SD_CURRENT_TIME_TX */

/* Macro called before initializing the SD driver
 * e.g. create a semaphore used for transfer notification */
/* USER CODE BEGIN FX_STM32_SD_PRE_INIT_TX */

#define FX_STM32_SD_PRE_INIT(_media_ptr)                do { \
                                                          if (tx_semaphore_create(&transfer_semaphore, "sd transfer semaphore", 1) != TX_SUCCESS) \
                                                          { \
                                                            _media_ptr->fx_media_driver_status = FX_IO_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_PRE_INIT_TX */

/* Macro called after initializing the SD driver */
/* USER CODE BEGIN FX_STM32_SD_POST_INIT */

#define FX_STM32_SD_POST_INIT(_media_ptr)

/* USER CODE END FX_STM32_SD_POST_INIT */

/* Macro called after the SD deinit */
/* USER CODE BEGIN FX_STM32_SD_POST_DEINIT_TX */

#define FX_STM32_SD_POST_DEINIT(_media_ptr)             do { \
                                                          tx_semaphore_delete(&transfer_semaphore); \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_POST_DEINIT_TX */

/* Macro called after the abort request */
/* USER CODE BEGIN FX_STM32_SD_POST_ABORT */

#define FX_STM32_SD_POST_ABORT(_media_ptr)

/* USER CODE END FX_STM32_SD_POST_ABORT */

/* Macro called before performing read operation */
/* USER CODE BEGIN FX_STM32_SD_PRE_READ_TRANSFER_DMA */

#define FX_STM32_SD_PRE_READ_TRANSFER(_media_ptr)       do { \
                                                          if(tx_semaphore_get(&transfer_semaphore, FX_STM32_SD_DEFAULT_TIMEOUT) != TX_SUCCESS) \
                                                          { \
                                                            return FX_IO_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_PRE_READ_TRANSFER_DMA */

/* Macro called after performing read operation */
/* USER CODE BEGIN FX_STM32_SD_POST_READ_TRANSFER_TX */

#define FX_STM32_SD_POST_READ_TRANSFER(_media_ptr)      do { \
                                                          if(tx_semaphore_put(&transfer_semaphore) != TX_SUCCESS) \
                                                          { \
                                                            return FX_IO_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_POST_READ_TRANSFER_TX */

/* Macro for read error handling */
/* USER CODE BEGIN FX_STM32_SD_READ_TRANSFER_ERROR_TX */

#define FX_STM32_SD_READ_TRANSFER_ERROR(_status_)       do { \
                                                          tx_semaphore_put(&transfer_semaphore); \
                                                          _status_ = FX_IO_ERROR; \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_READ_TRANSFER_ERROR_TX */

/* Define how to notify about Read completion operation */
/* USER CODE BEGIN FX_STM32_SD_READ_CPLT_NOTIFY_TX */

#define FX_STM32_SD_READ_CPLT_NOTIFY()                  do { \
                                                          if(tx_semaphore_get(&transfer_semaphore, FX_STM32_SD_DEFAULT_TIMEOUT) != TX_SUCCESS) \
                                                          { \
                                                            return FX_IO_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END FX_STM32_SD_READ_CPLT_NOTIFY_TX */

/* Define how to notify about write completion operation */
/* USER CODE BEGIN FX_STM32_SD_WRITE_CPLT_NOTIFY_TX */

#define FX_STM32_SD_WRITE_CPLT_NOTIFY                   FX_STM32_SD_READ_CPLT_NOTIFY

/* USER CODE END FX_STM32_SD_WRITE_CPLT_NOTIFY_TX */

/* Macro called before performing write operation */
/* USER CODE BEGIN FX_STM32_SD_PRE_WRITE_TRANSFER_TX */

#define FX_STM32_SD_PRE_WRITE_TRANSFER                  FX_STM32_SD_PRE_READ_TRANSFER

/* USER CODE END FX_STM32_SD_PRE_WRITE_TRANSFER_TX */

/* Macro called after performing write operation */
/* USER CODE BEGIN FX_STM32_SD_POST_WRITE_TRANSFER */

#define FX_STM32_SD_POST_WRITE_TRANSFER                 FX_STM32_SD_POST_READ_TRANSFER

/* USER CODE END FX_STM32_SD_POST_WRITE_TRANSFER */

/* Macro for write error handling */
/* USER CODE BEGIN FX_STM32_SD_WRITE_TRANSFER_ERROR */

#define FX_STM32_SD_WRITE_TRANSFER_ERROR                FX_STM32_SD_READ_TRANSFER_ERROR

/* USER CODE END FX_STM32_SD_WRITE_TRANFSER_ERROR */

/* Exported functions prototypes ---------------------------------------------*/

INT fx_stm32_sd_init(UINT instance);
INT fx_stm32_sd_deinit(UINT instance);

INT fx_stm32_sd_get_status(UINT instance);

INT fx_stm32_sd_read_blocks(UINT instance, UINT *buffer, UINT start_block, UINT total_blocks);
INT fx_stm32_sd_write_blocks(UINT instance, UINT *buffer, UINT start_block, UINT total_blocks);

VOID fx_stm32_sd_driver(FX_MEDIA *media_ptr);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif

#endif /* FX_STM32_SD_DRIVER_H */
