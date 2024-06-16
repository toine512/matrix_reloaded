/*** License Notice ***
*
* Matrix Reloaded Display: Wi-Fi LED panel display
* <https://github.com/toine512/matrix_reloaded>
* Copyright © 2024 toine512 <os@toine512.fr>
*
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use,
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* <http://www.cecill.info>.
*
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability.
*
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or
* data to be ensured and,  more generally, to use and operate it in the
* same conditions as regards security.
*
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*
*** ***************************** ***/

// Cstdlib
#include <stdint.h>
// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
// ESP-IDF
#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <driver/gpio.h>
// ESP-IDF Components
#include <tinyusb.h>
#include <mdns.h>


// App
#include "wifi.h"
#include "stor.h"
#include "web.h"
#include "disp.h"
#include "config.h"



//StaticSemaphore_t web_server_img_semphr;
SemaphoreHandle_t web_img_mem_use;
//StaticQueue_t web_server_img_q;
QueueHandle_t web_img_q;
//uint8_t web_server_img_q_mem[web_server_image_slots * sizeof(web_server_image_slot_t *)];


TaskHandle_t tsk_carousel;
MatrixPanel_I2S_DMA *dma_display;
VirtualMatrixPanel  *virtualDisp;

TaskHandle_t disp_task = NULL;
Display matrix_display(disp_task);

global_config_t config;

void carousel(void *parameter)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = (TickType_t)(2000 / portTICK_PERIOD_MS);

	while(true) {
		//virtualDisp->fillScreenRGB888(255, 0, 0);
		int16_t x = 0;
		int16_t y = 0;
		for ( ; x < 64 ; x++, y++) {
			virtualDisp->drawPixelRGB888(x, y, 255, 0, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		virtualDisp->fillScreenRGB888(0, 255, 0);
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		virtualDisp->fillScreenRGB888(0, 0, 255);
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		virtualDisp->fillScreenRGB888(255, 255, 255);
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

/*void draw_image(PNGDRAW *p_draw)
{
	//ESP_LOGI("draw", "draw_image called   iWidth: %d, iBpp: %d", p_draw->iWidth, p_draw->iBpp);
	int16_t y = static_cast<int16_t>(p_draw->y);
	uint8_t *p_pixel_base = p_draw->pPixels;
	for (int16_t x = 0 ; x < p_draw->iWidth ; x++, p_pixel_base += 4) //p_draw->iWidth
	{
		virtualDisp->drawPixelRGB888(x, y, *p_pixel_base, *(p_pixel_base+1), *(p_pixel_base+2));
	}
}

void display(void *parameters)
{
	void *p_receive = NULL;

	while (true)
	{
		// Wait forever for a new item in the queue
		BaseType_t res = xQueuePeek(web_img_q, &p_receive, portMAX_DELAY);
		assert(res == pdTRUE); // pdFALSE means the semaphore acquisition timed out which indicates INCLUDE_vTaskSuspend is misconfigured
		(void)res;

		// Lock memory
		xSemaphoreTake(web_img_mem_use, portMAX_DELAY);

		// Get item from the queue
		if (xQueuePeek(web_img_q, &p_receive, portTICK_PERIOD_MS) == pdTRUE)
		{
			web_server_image_slot_t *p_slot = static_cast<web_server_image_slot_t *>(p_receive);

			PNG decoder;
			int err = 0;
			err = decoder.openRAM(p_slot->p_data, web_server_image_buffer_size, draw_image);
			if(err != 0) {
				ESP_LOGE("disp", "erreur d'ouverture %d", decoder.getLastError());
			}
			err = decoder.decode(NULL, 0);
			if(err != 0) {
				ESP_LOGE("disp", "erreur de decodage %d", decoder.getLastError());
			}
			decoder.close();

			//virtualDisp->flipDMABuffer();

			// Release slot and pop from queue
			p_slot->b_isfree = true;
			xQueueReceive(web_img_q, &p_receive, portTICK_PERIOD_MS);
		}
		// else queue was reset in the meantime

		// Release memory
		xSemaphoreGive(web_img_mem_use);
	}
}*/

#ifndef NDEBUG

void stats(void *parameter)
{
  while(true) {
    vTaskDelay(60000 / portTICK_PERIOD_MS);
    ESP_LOGW("Stats","-----------------");

    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;

   /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
   uxArraySize = uxTaskGetNumberOfTasks();

   /* Allocate a TaskStatus_t structure for each task.  An array could be
   allocated statically at compile time. */
   pxTaskStatusArray = (TaskStatus_t*)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

   if( pxTaskStatusArray != NULL )
   {
      /* Generate raw status information about each task. */
      uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                 uxArraySize,
                                 &ulTotalRunTime );

      /* For percentage calculations. */
      ulTotalRunTime /= 100UL;

      /* Avoid divide by zero errors. */
      if( ulTotalRunTime > 0 )
      {
         /* For each populated position in the pxTaskStatusArray array,
         format the raw data as human readable ASCII data. */
         for( x = 0; x < uxArraySize; x++ )
         {
            /* What percentage of the total run time has the task used?
            This will always be rounded down to the nearest integer.
            ulTotalRunTimeDiv100 has already been divided by 100. */
            ulStatsAsPercentage =
            pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

            ESP_LOGW("Stats", "%u %s %lu %lu%% %u %lu",
                                 pxTaskStatusArray[ x ].xCoreID,
                                 pxTaskStatusArray[ x ].pcTaskName,
                                 pxTaskStatusArray[ x ].ulRunTimeCounter,
                                 ulStatsAsPercentage,
                                 pxTaskStatusArray[ x ].uxCurrentPriority,
                                 pxTaskStatusArray[ x ].usStackHighWaterMark );
         }
      }

      /* The array is no longer needed, free the memory it consumes. */
      vPortFree( pxTaskStatusArray );
   }
  }
}

#endif

extern "C" {

// Hack for when VBUS detection doesn't work
void tud_suspend_cb(bool) {
    tud_umount_cb();
}

void usb_change_operation(tinyusb_msc_event_t *msc_event)
{
	static bool first_mount = true;

	// USB plugged, pre-unmount flash: stop PSRAM write ASAP
	if (msc_event->type == TINYUSB_MSC_EVENT_PREMOUNT_CHANGED && msc_event->mount_changed_data.is_mounted == true)
	{
		ESP_LOGW("USB", "preunmount");
		ESP_ERROR_CHECK_WITHOUT_ABORT( web_server_free_memory() );
		matrix_display.suspend_task(); // AFTER freeing memory, otherwise the semaphore may deadlock
	}

	// Post-unmount flash: shutdown the show
	else if (msc_event->type == TINYUSB_MSC_EVENT_MOUNT_CHANGED && msc_event->mount_changed_data.is_mounted == false)
	{
		ESP_LOGW("USB", "postunmount");
		ESP_ERROR_CHECK_WITHOUT_ABORT( web_server_stop() );
		matrix_display.destroy(); // web server has a pointer to display
		mdns_free();
		ESP_ERROR_CHECK_WITHOUT_ABORT( wifi_sta_stop() );
		esp_netif_deinit();
	}

	// Post-mount flash: reboot on USB unplugged or do nothing on startup (the first time)
	else if (msc_event->type == TINYUSB_MSC_EVENT_MOUNT_CHANGED && msc_event->mount_changed_data.is_mounted == true)
	{
		if (first_mount) {
			first_mount = false;
		}
		else
		{
			ESP_LOGW("USB", "postmount");
			// unmount
			ESP_ERROR_CHECK_WITHOUT_ABORT( tinyusb_msc_unregister_callback(TINYUSB_MSC_EVENT_PREMOUNT_CHANGED) );
			ESP_ERROR_CHECK_WITHOUT_ABORT( tinyusb_msc_unregister_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED) );
			ESP_ERROR_CHECK( tinyusb_msc_storage_unmount() );
			tinyusb_msc_storage_deinit();
			// reboot
			esp_restart();
		}
	}
}

void app_main()
{
	// GPIO default
	#define GMSK GPIO_TO_MASK

	gpio_set_drive_capability(MTX_HUB75_nOE, GPIO_DRIVE_CAP_2);
	gpio_set_drive_capability(MTX_SPARE0, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability(MTX_SPARE1, GPIO_DRIVE_CAP_0);
	gpio_set_level(MTX_HUB75_nOE, 1);
	gpio_set_level(MTX_SPARE0, 0);
	gpio_set_level(MTX_SPARE1, 0);

	gpio_config_t gpio_cfg;
	gpio_cfg.pin_bit_mask = GMSK(MTX_SPARE0) | GMSK(MTX_SPARE1) | GMSK(MTX_HUB75_nOE) | GMSK(MTX_HUB75_R1) | GMSK(MTX_HUB75_G1) | GMSK(MTX_HUB75_B1) | GMSK(MTX_HUB75_R2) | GMSK(MTX_HUB75_G2) | GMSK(MTX_HUB75_B2) | GMSK(MTX_HUB75_A) | GMSK(MTX_HUB75_B) | GMSK(MTX_HUB75_C) | GMSK(MTX_HUB75_D) | GMSK(MTX_HUB75_E) | GMSK(MTX_HUB75_LATCH) | GMSK(MTX_HUB75_BLANK) | GMSK(MTX_HUB75_CLK);
	gpio_cfg.mode = GPIO_MODE_OUTPUT;
	gpio_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_cfg.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&gpio_cfg);

	#undef GMSK

	// Shared ressources
	//web_img_mem_use = xSemaphoreCreateBinaryStatic(&web_server_img_semphr);
	web_img_mem_use = xSemaphoreCreateBinary();
	//web_img_q = xQueueCreateStatic(web_server_image_slots, sizeof(web_server_image_slot_t *), web_server_img_q_mem, &web_server_img_q);
	web_img_q = xQueueCreate(web_server_image_q_length, sizeof(web_server_image_slot_t *));

	// Event loop
	ESP_ERROR_CHECK( esp_event_loop_create_default() ); // system drivers are handled by the default, always running, loop

	// tinyUSB
	const tinyusb_config_t tinyusb_init_cfg = {};
	ESP_ERROR_CHECK( tinyusb_driver_install(&tinyusb_init_cfg) );

	// Flash FAT Storage (dep: tinyUSB)
	esp_err_t res = storage_init_spiflash("config", usb_change_operation, usb_change_operation);
	if (res != ESP_OK)
	{
		ESP_LOGE("Startup", "Can't initialise storage partition, aborting. Code %d", res);
		vTaskSuspend(NULL);
	}
	res = tinyusb_msc_storage_mount("/flash");
	if (res != ESP_OK)
	{
		ESP_LOGE("Startup", "Can't mount FATFS, aborting. Code %d", res);
		vTaskSuspend(NULL);
	}

	ESP_LOGW("MEM", "free heap %lu", esp_get_free_internal_heap_size());

	// Read configuration from flash (filesystem must be mounted)
	if ( ESP_OK == config_load(&config, "/flash/configuration.ini") )
	{
		ESP_LOGI("Startup", "Configuration loaded, starting the show.");
	}
	else
	{
		ESP_LOGE("Startup", "Configuration unsuccessful, aborting.");
		vTaskSuspend(NULL);
	}

	// LED Matrix
	ESP_ERROR_CHECK( matrix_display.start(web_img_mem_use, web_img_q, config.i_disp_brightness, static_cast<unsigned int>(config.i_disp_img_min_dur), static_cast<unsigned int>(config.i_disp_gif_max_total_dur), static_cast<unsigned int>(config.i_disp_gif_min_loops), config.i_disp_gif_aggressive_transp_lvl, config.i_lcdcam_clkdiv_num, config.i_lcdcam_clkdiv_a, config.i_lcdcam_clkdiv_b) );

	// Networking
	ESP_ERROR_CHECK( esp_netif_init() );

	// Wi-Fi (dep: default event loop & netif)
	ESP_ERROR_CHECK( wifi_sta_init() );
	ESP_ERROR_CHECK( wifi_sta_config(config.s_wifi_cc, config.s_wifi_ssid, config.s_wifi_key) );
	ESP_ERROR_CHECK( wifi_sta_start() );

	// mDNS service (dep: default event loop & netif)
	ESP_ERROR_CHECK( mdns_init() );
	ESP_ERROR_CHECK( mdns_hostname_set(config.sz_mdns_hostname) );
	ESP_ERROR_CHECK( mdns_instance_name_set("Matrix Reloaded") );

	// HTTP srever (dep: default event loop & netif)
	ESP_ERROR_CHECK( web_server_start(&disp_task, web_img_mem_use, web_img_q) );

	ESP_LOGW("MEM", "free heap %lu", esp_get_free_internal_heap_size());
	ESP_LOGW("MEM", "free PSRAM %u", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

	//xTaskCreate(carousel, "couleurs", 1024, nullptr, 19, &tsk_carousel);
	//xTaskCreate(display, "images", 50*1024, nullptr, 19, &tsk_carousel);
	#ifndef NDEBUG
	//xTaskCreate(stats, "stats", 4096, nullptr, 0, nullptr);
	#endif
}
}
