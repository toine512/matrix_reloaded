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

#pragma once

// Global configuration constants
#define web_server_image_slots 11U
#define web_server_image_q_length web_server_image_slots
#define web_server_image_buffer_size (512U*1024U)

// C Standard Library
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
// ESP-IDF
#include <esp_err.h>
#include <esp_log.h>
#include <esp_check.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <sys/socket.h>
// App
#include "helpers.h"
#include "formats.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

// Types
typedef struct web_server_image_slot
{
	bool b_isfree;
	enum ImageFormat e_format;
	uint8_t *p_data;
} web_server_image_slot_t;

// Web server core
void web_server_custom_response(httpd_req_t *hidf_req, const char *sz_response, const char *sz_content);

// Image server functions
esp_err_t web_server_alloc_memory();
esp_err_t web_server_free_memory();
web_server_image_slot_t * web_server_find_free_slot();

// HTTP handlers
esp_err_t web_server_handler_get_root(httpd_req_t *hidf_req);
esp_err_t web_server_handler_clear_display(httpd_req_t *hidf_req);
esp_err_t web_server_handler_post_image(httpd_req_t *hidf_req);

// Web server operations
esp_err_t web_server_start(TaskHandle_t *p_disp_task, SemaphoreHandle_t in_use_semaphore, QueueHandle_t image_queue);
esp_err_t web_server_stop();

#ifdef __cplusplus
}
#endif
