/*
----------------------------------------------------------------------
File    : logger.c
Purpose : Source code for the data logger task. Data acquisition from
          multiple sensors will be done in this task.

          The log file is a binary file with the following format:
          [timestamp][sensor_id][data_length][data][EOL]
          where
            timestamp is a 32-bit unsigned integer,
            sensor_id is a 8-bit unsigned integer,
            data_length is a 8-bit unsigned integer,
            and followed by data_length bytes of data.
            EOL is a 16-bit unsigned integer 0x0D0A.

          Sensors IDs and data format are as follows:
            0x01 - ADC, each ADC data is 16-bit unsigned integer
              [N][ADC_1][ADC_2]...[ADC_N]
            0x02 - Accelerometer, 3 axes raw data, each axis is 16-bit
              signed integer [X][Y][Z]
            0x03 - Gyroscope, 3 axes raw data, each axis is 16-bit
              signed integer [X][Y][Z]

Revision: $Rev: 2023.49$
----------------------------------------------------------------------
*/

#include "logger.h"

#include "SEGGER_RTT.h"
#include "adc.h"
#include "config.h"
#include "events.h"
#include "fx_api.h"
#include "imu.h"
#include "main.h"
#include "stddef.h"
#include "usbd_cdc_if.h"

TX_THREAD logger_thread;

extern TX_EVENT_FLAGS_GROUP event_flags;

// Logger file objects
extern FX_MEDIA sdio_disk;
static FX_FILE logger_file = {.fx_file_name = NULL};

static inline void logger_output(char *buf, size_t len) {
#ifdef LOGGER_SD
  fx_file_write(&logger_file, buf, len);
#endif
#ifdef LOGGER_USB_SERIAL
  CDC_Transmit_FS((uint8_t *)buf, len);
#endif
}

void logger_thread_entry(ULONG thread_input) {
  UINT status = FX_SUCCESS;

  // Wait for the filesystem and config to be loaded
  ULONG recv_events_flags = 0;
  status = tx_event_flags_get(&event_flags,
                              EVENT_BIT(EVENT_FS_INIT) |
                                  EVENT_BIT(EVENT_CONFIG_LOADED) |
                                  EVENT_BIT(EVENT_LOGGING),
                              TX_AND, &recv_events_flags, TX_WAIT_FOREVER);

  while (1) {
    recv_events_flags = 0;
    tx_event_flags_get(&event_flags, EVENT_BIT(EVENT_LOGGING), TX_OR,
                       &recv_events_flags, TX_NO_WAIT);
    if (recv_events_flags & EVENT_BIT(EVENT_LOGGING)) {
      if (unlikely(!logger_file.fx_file_name)) {
        int fid = 0;
        char fn[32];
        do {
          sprintf(fn, LOGGER_FN_PATTERN, fid++);
          status = fx_file_create(&sdio_disk, fn);
        } while (status != FX_SUCCESS);

        fx_file_open(&sdio_disk, &logger_file, fn, FX_OPEN_FOR_WRITE);
        fx_file_seek(&logger_file, 0);
        SEGGER_RTT_printf(0, "Logger file %s opened\n", fn);
        HAL_GPIO_WritePin(REC_OUTPUT_GPIO_Port, REC_OUTPUT_Pin, GPIO_PIN_SET);
      }
    } else {
      fx_media_flush(&sdio_disk);
      fx_file_close(&logger_file);
      logger_file.fx_file_name = NULL;
      HAL_GPIO_WritePin(REC_OUTPUT_GPIO_Port, REC_OUTPUT_Pin, GPIO_PIN_RESET);
      // Wait for the logging event to be set again
      tx_event_flags_get(&event_flags, EVENT_BIT(EVENT_LOGGING), TX_AND,
                         &recv_events_flags, TX_WAIT_FOREVER);
    }

    static char buf[1024];
    uint32_t timestamp = tx_time_get();
    memcpy(buf, &timestamp, sizeof(timestamp));

#if ADC_ENABLE
    static uint32_t last_adc_timestamp = 0;
    adc_t *adc[] = {open_adc_instance(0), open_adc_instance(1),
                    open_adc_instance(2), open_adc_instance(3)};
    const size_t ADC_N = sizeof(adc) / sizeof(adc[0]);
    const size_t ADC_VALUE_SIZE = sizeof(typeof(adc[0]->value));
    if (timestamp - last_adc_timestamp > TX_TIMER_TICKS_PER_SECOND / 1000) {
      buf[4] = 0x01;
      buf[5] = ADC_N * ADC_VALUE_SIZE;

      for (size_t i = 0; i < ADC_N; i++)
        memcpy(buf + 6 + i * ADC_VALUE_SIZE, &adc[i]->value, ADC_VALUE_SIZE);

      buf[6 + ADC_N * 2] = 0x0D;
      buf[7 + ADC_N * 2] = 0x0A;
      logger_output(buf, 8 + ADC_N * ADC_VALUE_SIZE);
      last_adc_timestamp = timestamp;
    }
#endif

#if IMU_ENABLE
    static imu_t *imu = NULL;
    if (!imu) imu = open_imu_instance(0);

    static uint32_t last_acc_timestamp = 0;
    static uint32_t last_gyro_timestamp = 0;
    if (imu->acc.timestamp != last_acc_timestamp) {
      buf[4] = 0x02;
      buf[5] = 0x06;
      memcpy(buf + 6, &imu->acc_raw.x, 2);
      memcpy(buf + 8, &imu->acc_raw.y, 2);
      memcpy(buf + 10, &imu->acc_raw.z, 2);
      buf[12] = 0x0D;
      buf[13] = 0x0A;
      logger_output(buf, 14);
      last_acc_timestamp = imu->acc.timestamp;
    }

    if (imu->gyro.timestamp != last_gyro_timestamp) {
      buf[4] = 0x03;
      buf[5] = 0x06;
      memcpy(buf + 6, &imu->gyro_raw.x, 2);
      memcpy(buf + 8, &imu->gyro_raw.y, 2);
      memcpy(buf + 10, &imu->gyro_raw.z, 2);
      buf[12] = 0x0D;
      buf[13] = 0x0A;
      logger_output(buf, 14);
      last_gyro_timestamp = imu->gyro.timestamp;
    }
#endif

#ifdef LOGGER_SD
    static uint32_t last_sd_timestamp = 0;
    if (timestamp - last_sd_timestamp > TX_TIMER_TICKS_PER_SECOND) {
      fx_media_flush(&sdio_disk);
      last_sd_timestamp = timestamp;
    }
#endif
  }
}
