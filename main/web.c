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

#include "web.h"

#define LOG_TAG "HTTP Server"

// Working variables
struct web_server_ctx
{
	httpd_handle_t hidf_httpd;
	void *p_psram_block;
	TaskHandle_t *p_hfrt_disp_tsk;
	SemaphoreHandle_t hfrt_mem_semphr;
	QueueHandle_t hfrt_img_q;
	web_server_image_slot_t p_mem_slots[web_server_image_slots];
};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
struct web_server_ctx web_server = {NULL, NULL, NULL, NULL, NULL, /*uninitialised*/};
#pragma GCC diagnostic pop


// Helper functions
inline void web_server_custom_response(httpd_req_t *hidf_req, const char *sz_response, const char *sz_content)
{
	httpd_resp_set_status(hidf_req, sz_response);
	httpd_resp_set_type(hidf_req, "text/plain");

	// Implement the NODELAY option from ESP-IDF
	// Code from httpd_resp_send_custom_err()
#ifdef CONFIG_HTTPD_ERR_RESP_NO_DELAY
	/* Use TCP_NODELAY option to force socket to send data in buffer
	 * This ensures that the error message is sent before the socket
	 * is closed */
	int fd = *(int *)(hidf_req->aux); // Dangerous: equivalent to hidf_req->aux->sd->fd (firstest element) without knowing the structures, see components/esp_http_server/src/esp_httpd_priv.h from ESP-IDF, can break at any moment
	int nodelay = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
	{
		/* If failed to turn on TCP_NODELAY, throw warning and continue */
		ESP_LOGW(LOG_TAG, "error calling setsockopt : %d", errno);
		nodelay = 0;
	}
#endif

	httpd_resp_sendstr(hidf_req, sz_content);

#ifdef CONFIG_HTTPD_ERR_RESP_NO_DELAY
	/* If TCP_NODELAY was set successfully above, time to disable it */
	if (nodelay == 1)
	{
		nodelay = 0;
		if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
		{
			/* If failed to turn off TCP_NODELAY, throw error and
			 * return failure to signal for socket closure */
			ESP_LOGE(LOG_TAG, "error calling setsockopt : %d", errno);
		}
	}
#endif
}

esp_err_t web_server_alloc_memory()
{
	if (web_server.hfrt_mem_semphr == NULL || web_server.hfrt_img_q == NULL) {
		return ESP_ERR_INVALID_STATE;
	}
	assert(uxQueueMessagesWaiting(web_server.hfrt_img_q) == 0); // The image queue should be empty

	// Allocate data buffers in the PSRAM
	web_server.p_psram_block = heap_caps_malloc(web_server_image_slots * web_server_image_buffer_size * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
	if (web_server.p_psram_block == NULL) {
		ESP_LOGD(LOG_TAG, "Not enough free PSRAM for image ingress buffers!");
		return ESP_ERR_NO_MEM;
	}

	// Initialise slot structures
	web_server_image_slot_t *p_slot = web_server.p_mem_slots;
	uint8_t *p_psram_buffer = (uint8_t *)web_server.p_psram_block;
	for (unsigned int i = 0 ; i < web_server_image_slots; i++, p_slot++, p_psram_buffer += web_server_image_buffer_size)
	{
		p_slot->b_isfree = true;
		p_slot->e_format = IMG_FMT_UNSUPPORTED;
		p_slot->p_data = p_psram_buffer;
	}

	// Signal memory is ready
	BaseType_t res = xSemaphoreGive(web_server.hfrt_mem_semphr);
	if (res != pdTRUE)
	{
		void *p_mem = web_server.p_psram_block;
		web_server.p_psram_block = NULL;
		free(p_mem);

		ESP_LOGD(LOG_TAG, "Invalid semaphore!");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t web_server_free_memory()
{
	if (web_server.hfrt_mem_semphr == NULL || web_server.hfrt_img_q == NULL) {
		return ESP_ERR_INVALID_STATE;
	}

	// Purge the image queue
	xQueueReset(web_server.hfrt_img_q);

	// Make sure memory is not in use while it is deallocated
	if (web_server.p_psram_block != NULL) // checked to prevent deadlock on semaphore
	{
		BaseType_t res = xSemaphoreTake(web_server.hfrt_mem_semphr, portMAX_DELAY); // wait forever
		assert(res == pdTRUE); // pdFALSE means the semaphore acquisition timed out which indicates INCLUDE_vTaskSuspend is misconfigured
		(void)res;

		// Neutralise slot structures
		web_server_image_slot_t *p_slot = web_server.p_mem_slots;
		for (unsigned int i = 0 ; i < web_server_image_slots ; i++, p_slot++)
		{
			p_slot->b_isfree = false;
			p_slot->p_data = NULL;
		}

		// Free data buffers in the PSRAM
		void *p_mem = web_server.p_psram_block;
		web_server.p_psram_block = NULL;
		free(p_mem);
	}

	return ESP_OK;
}

/* NOT thread-safe!
   Only one request handler runs concurrently so it's ok. */
web_server_image_slot_t * web_server_find_free_slot()
{
	if (web_server.p_psram_block != NULL)
	{
		web_server_image_slot_t *p_slot = web_server.p_mem_slots;
		for (unsigned int i = 0; i < web_server_image_slots; i++, p_slot++)
		{
			if (p_slot->b_isfree)
			{
				return p_slot;
			}
		}
	}

	return NULL;
}

// Root handler
esp_err_t web_server_handler_get_root(httpd_req_t *hidf_req)
{
	const char resp[] = "Matrix Reloaded Display v." MTX_VERSION;
	httpd_resp_set_type(hidf_req, "text/plain");
	httpd_resp_send(hidf_req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

// Clear display handler
esp_err_t web_server_handler_clear_display(httpd_req_t *hidf_req)
{
	ESP_LOGV(LOG_TAG, "GET clear screen handler");

	if (pdTRUE == xSemaphoreTake(web_server.hfrt_mem_semphr, pdMS_TO_TICKS(100))) // Lock memory
	{
		// Empty the queue
		xQueueReset(web_server.hfrt_img_q);

		// Release memory
		xSemaphoreGive(web_server.hfrt_mem_semphr);

		// Make all slots available
		// No race condition because only one request handler runs
		web_server_image_slot_t *p_slot = web_server.p_mem_slots;
		for (unsigned int i = 0; i < web_server_image_slots; i++, p_slot++) {
			p_slot->b_isfree = true;
		}

		// Request clear via DtTN: set bit 0 of notification slot 0 from display task
		if (web_server.p_hfrt_disp_tsk != NULL) {
			TaskHandle_t hfrt_display_tsk = *web_server.p_hfrt_disp_tsk;
			if (hfrt_display_tsk != NULL) {
				xTaskNotifyIndexed(hfrt_display_tsk, 0, 0x00000001, eSetBits);
			}
		}

		httpd_resp_set_type(hidf_req, "text/plain");
		httpd_resp_sendstr(hidf_req, "Cleared display.");
	}
	else
	{
		web_server_custom_response(hidf_req, "500 Internal Server Error", "Can't take semaphore!");
		ESP_LOGE(LOG_TAG, "Cannot take memory semaphore to push to front of queue! (100 ms)");
	}

	return ESP_OK;
}

// Image upload handler
esp_err_t web_server_handler_post_image(httpd_req_t *hidf_req)
{
	ESP_LOGV(LOG_TAG, "POST image handler");

	// Limit file size
	ESP_LOGV(LOG_TAG, "Content-Length: %u", hidf_req->content_len);

	if (hidf_req->content_len > web_server_image_buffer_size) // in bytes
	{
		web_server_custom_response(hidf_req, "413 Content Too Large", "File too large! Max is 512 KiB.");
		ESP_LOGV(LOG_TAG, "Content too large!");
		return ESP_OK;
	}

	// Get an image slot, no race condition as only one request handler runs
	web_server_image_slot_t *p_slot = web_server_find_free_slot();
	if (p_slot == NULL)
	{
		web_server_custom_response(hidf_req, "503 Service Unavailable", "No memory available! Please retry later.");
		ESP_LOGV(LOG_TAG, "No memory slot available for storing content at the moment.");
		return ESP_OK;
	}

	// Receive
	int i_received = 0;
	for (int i_chunk = 0 ; i_received < hidf_req->content_len ; i_received += i_chunk)
	{
		i_chunk = httpd_req_recv(hidf_req, (char *)(p_slot->p_data + i_received), web_server_image_buffer_size * sizeof(uint8_t) / sizeof(char)); // in bytes or error code
		if (i_chunk <= 0) // failure
		{
			ESP_LOGD(LOG_TAG, "Fatal socket error %d !", i_chunk);
			if (i_chunk == HTTPD_SOCK_ERR_TIMEOUT) {
				httpd_resp_send_408(hidf_req);
				ESP_LOGI(LOG_TAG, "Transfer timeout!");
			}
			return ESP_FAIL;
		}
		//else
		ESP_LOGV(LOG_TAG, "Received chunk: %dB", i_chunk);
	}
	ESP_LOGD(LOG_TAG, "Received %dB file.", i_received);

	// Check there is at least 32 B of content
	if (i_received < 32)
	{
		web_server_custom_response(hidf_req, "422 Unprocessable Content", "File too small!");
		ESP_LOGV(LOG_TAG, "Content rejected: too small!");
		return ESP_OK;
	}

	// Find image format
	uint8_t p_copy[8];
	memcpy((void *)p_copy, (void *)p_slot->p_data, 8); // data is in PSRAM
	p_slot->e_format = detect_image_format(p_copy); // needs 8 B of data

	// Abort if format not supported
	if (p_slot->e_format == IMG_FMT_UNSUPPORTED)
	{
		web_server_custom_response(hidf_req, "422 Unprocessable Content", "Unsupported image format.");
		ESP_LOGV(LOG_TAG, "Content rejected: unsupported.");
		return ESP_OK;
	}

	// Configure slot
	p_slot->b_isfree = false;
	memset((void *)(p_slot->p_data + i_received), 0, web_server_image_buffer_size - i_received); // fill unused buffer space with zeros

	// Push to image queue
	BaseType_t res;
	if (0 == strcmp("/image-prio", hidf_req->uri)) // prio or not?
	{
		if (pdTRUE == xSemaphoreTake(web_server.hfrt_mem_semphr, pdMS_TO_TICKS(100)))
		{
			res = xQueueSendToFront(web_server.hfrt_img_q, &p_slot, pdMS_TO_TICKS(100));
			xSemaphoreGive(web_server.hfrt_mem_semphr);
		}
		else
		{
			web_server_custom_response(hidf_req, "500 Internal Server Error", "Can't take semaphore!");
			ESP_LOGE(LOG_TAG, "Cannot take memory semaphore to push to front of queue! (100 ms)");
			return ESP_OK;
		}
	}
	else
	{
		res = xQueueSendToBack(web_server.hfrt_img_q, &p_slot, pdMS_TO_TICKS(100));
	}
	assert(res == pdTRUE); // Queue size is supposed to be at least the number of slots thus never full.
	(void)res;

	httpd_resp_set_type(hidf_req, "text/plain");
	httpd_resp_sendstr(hidf_req, "Loaded.");

	return ESP_OK;
}

// URI definitions
httpd_uri_t web_server_uri_get_root = {
	.uri      = "/",
	.method   = HTTP_GET,
	.handler  = web_server_handler_get_root,
	.user_ctx = NULL
};
httpd_uri_t web_server_uri_get_clear = {
	.uri      = "/clear",
	.method   = HTTP_GET,
	.handler  = web_server_handler_clear_display,
	.user_ctx = NULL
};
httpd_uri_t web_server_uri_post_image = {
	.uri      = "/image",
	.method   = HTTP_POST,
	.handler  = web_server_handler_post_image,
	.user_ctx = NULL
};
httpd_uri_t web_server_uri_post_image_prio = {
	.uri      = "/image-prio",
	.method   = HTTP_POST,
	.handler  = web_server_handler_post_image,
	.user_ctx = NULL
};

// Actions
/* The server will own control of the queue. */
esp_err_t web_server_start(TaskHandle_t *p_disp_task, SemaphoreHandle_t in_use_semaphore, QueueHandle_t image_queue)
{
	if (in_use_semaphore == NULL || image_queue == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	web_server.hfrt_mem_semphr = in_use_semaphore;
	web_server.hfrt_img_q = image_queue;
	web_server.p_hfrt_disp_tsk = p_disp_task;

	httpd_config_t server_cfg = HTTPD_DEFAULT_CONFIG();
	server_cfg.task_priority = 17;
	server_cfg.max_uri_handlers = 4; // Adjust to the number of following httpd_register_uri_handler() calls
	server_cfg.max_resp_headers = 4;

	ESP_RETURN_ON_ERROR( httpd_start(&web_server.hidf_httpd, &server_cfg) , LOG_TAG, "Starting httpd failed!");

	ESP_RETURN_ON_ERROR( httpd_register_uri_handler(web_server.hidf_httpd, &web_server_uri_get_root) , LOG_TAG, "Can't register GET root handler!");

	RETURN_FAILED( web_server_alloc_memory() );

	esp_err_t ret = 0;
	ESP_GOTO_ON_ERROR( httpd_register_uri_handler(web_server.hidf_httpd, &web_server_uri_get_clear) , cleanup, LOG_TAG, "Can't register GET clear handler!");
	ESP_GOTO_ON_ERROR( httpd_register_uri_handler(web_server.hidf_httpd, &web_server_uri_post_image) , cleanup, LOG_TAG, "Can't register POST image handler!");
	ESP_GOTO_ON_ERROR( httpd_register_uri_handler(web_server.hidf_httpd, &web_server_uri_post_image_prio) , cleanup, LOG_TAG, "Can't register POST priority image handler!");

	return ESP_OK;

	cleanup:
	web_server_free_memory();
	return ret;
}

esp_err_t web_server_stop()
{
	if (web_server.hidf_httpd == NULL) {
		return ESP_ERR_INVALID_STATE;
	}
	ESP_ERROR_CHECK( httpd_stop(web_server.hidf_httpd) );
	web_server.hidf_httpd = NULL;
	return ESP_OK;
}


#undef LOG_TAG
