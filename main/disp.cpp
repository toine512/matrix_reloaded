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

#include "disp.h"

#define STBIR_NO_SIMD
#define STBIR_MALLOC(size,user_data) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#define STBIR_FREE(ptr,user_data) heap_caps_free(ptr)
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image_resize2.h"
#pragma GCC diagnostic pop

//#define LOG_TAG "Matrix Display"
constexpr char LOG_TAG[] = "Matrix Display";

Display::Display(TaskHandle_t &hfrt_display_task) :
	hfrt_display_task(hfrt_display_task), hfrt_img_mem_sem(NULL), hfrt_img_q(NULL), i_min_img_dur(0), i_max_total_gif_dur(0), i_min_gif_loops(0), i_gif_aggressive_transparency_lvl(0), p_hub75_panel(nullptr), p_virtualmatrix(nullptr)
{ }

Display::~Display()
{ }

esp_err_t Display::start(SemaphoreHandle_t in_use_semaphore, QueueHandle_t image_queue, uint8_t configured_brightness, unsigned int min_image_duration, unsigned int max_total_gif_duration, unsigned int min_gif_loops, int gif_aggressive_transp_level, uint8_t clkdiv_num, uint8_t clkdiv_a, uint8_t clkdiv_b)
{
	// Set handles to data sources
	if (in_use_semaphore == NULL || image_queue == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	hfrt_img_mem_sem = in_use_semaphore;
	hfrt_img_q = image_queue;

	i_min_img_dur = min_image_duration;
	i_max_total_gif_dur = max_total_gif_duration;
	i_min_gif_loops = min_gif_loops;
	i_gif_aggressive_transparency_lvl = gif_aggressive_transp_level;

	// Allocate and start the HUB75 matrix
	HUB75_I2S_CFG mxconfig;
	/*mxconfig.mx_width = 64;
	mxconfig.mx_height = 64;
	mxconfig.chain_length = 4;*/
	mxconfig.mx_width = 128;
	mxconfig.mx_height = 64;
	mxconfig.chain_length = 2;
	mxconfig.min_refresh_rate = 57; // will select the right bitdepth for both 60 Hz and 90 Hz at 128x128
	//mxconfig.min_refresh_rate = 87;
	mxconfig.clkdiv_num = clkdiv_num; //14
	mxconfig.clkdiv_a = clkdiv_a; //21
	mxconfig.clkdiv_b = clkdiv_b; //13
	mxconfig.latch_blanking = 1;
	mxconfig.clkphase = false;
	mxconfig.gpio.r1 = MTX_HUB75_R1;
	mxconfig.gpio.g1 = MTX_HUB75_G1;
	mxconfig.gpio.b1 = MTX_HUB75_B1;
	mxconfig.gpio.r2 = MTX_HUB75_R2;
	mxconfig.gpio.g2 = MTX_HUB75_G2;
	mxconfig.gpio.b2 = MTX_HUB75_B2;
	mxconfig.gpio.a = MTX_HUB75_A;
	mxconfig.gpio.b = MTX_HUB75_B;
	mxconfig.gpio.c = MTX_HUB75_C;
	mxconfig.gpio.d = MTX_HUB75_D;
	mxconfig.gpio.e = MTX_HUB75_E;
	mxconfig.gpio.lat = MTX_HUB75_LATCH;
	mxconfig.gpio.oe = MTX_HUB75_BLANK;
	mxconfig.gpio.clk = MTX_HUB75_CLK;
	mxconfig.double_buff = false;
	p_hub75_panel = new MatrixPanel_I2S_DMA(mxconfig);
	ESP_RETURN_ON_FALSE( p_hub75_panel->begin() , ESP_ERR_NO_MEM, LOG_TAG, "HUB75 library startup failed!");
	//p_hub75_panel->setBrightness(59); // 724 mA/panel
	//p_hub75_panel->setBrightness(50); //normal
	//p_hub75_panel->setBrightness(70);
	p_hub75_panel->setBrightness(configured_brightness);
	//p_virtualmatrix = new VirtualMatrixPanel(*p_hub75_panel, 2, 2, 64, 64, CHAIN_BOTTOM_LEFT_UP);
	p_virtualmatrix = new VirtualMatrixPanel(*p_hub75_panel, 2, 1, 128, 64, CHAIN_TOP_RIGHT_DOWN_ZZ);

	// Enable level shifters output
	gpio_set_level(MTX_HUB75_nOE, 0);

	// Launch renderer task with stack in PSRAM
	BaseType_t res = xTaskCreatePinnedToCoreWithCaps(task_static_fn, "renderer", 53*1024 + DISPLAY_RENDERER_DECODE_SIZE*DISPLAY_RENDERER_DECODE_SIZE*3 + 128*128*3, static_cast<void *>(this), DISPLAY_TASK_PRIORITY, &hfrt_display_task, 1, MALLOC_CAP_SPIRAM);
	if (res != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "Cannot create task with error %d !", res);
		destroy();
		return ESP_FAIL;
	}

	return ESP_OK;
}

void Display::destroy()
{
	if (hfrt_display_task != NULL) {
		TaskHandle_t hfrt_task = hfrt_display_task;
		hfrt_display_task = NULL;
		vTaskDelete(hfrt_task);
	}
	p_virtualmatrix->clearScreen();

	// Disable level shifters output
	gpio_set_level(MTX_HUB75_nOE, 1);

	p_hub75_panel->stopDMAoutput();
	delete p_virtualmatrix;
	p_virtualmatrix = nullptr;
	//TODO FUCKING BROKEN
	//delete p_hub75_panel;
	p_hub75_panel = nullptr;
}

void Display::suspend_task()
{
	if (hfrt_display_task != NULL) {
		vTaskSuspend(hfrt_display_task);
	}
}

inline uint8_t * Display::scale(int i_width, int i_height, uint8_t *p_input_buffer, uint8_t *p_intermediate_buffer)
{
	// Reset output offsets
	i_offset_x = 0;
	i_bound_x = 128;
	i_offset_y = 0;
	i_bound_y = 128;

	// If no scaling needed, p_intermediate_buffer is not used
	if (i_width == 128 && i_height == 128) {
		return p_input_buffer;
	}

	// Else
	// Select filter
	stbir_filter use_filter = STBIR_FILTER_CATMULLROM; // for lowest resolution
	if (i_width > 128 || i_height > 128) { // down-sample
		use_filter = STBIR_FILTER_POINT_SAMPLE;
	}
	else if (i_width > 72 || i_height > 72) {  // 72-128 px
		use_filter = STBIR_FILTER_TRIANGLE;
	}

	// Compute scaled dimensions keeping aspect ratio, and offsets for center alignment
	int i_scaled_width = 128;
	int i_scaled_height = 128;
	if (i_width > i_height)
	{
		i_scaled_height = static_cast<int>(128.0 * static_cast<float>(i_height) / static_cast<float>(i_width) + 0.5);
		i_offset_y = (128 - i_scaled_height + 1) >> 1; // (divide by 2 rounding up)
		i_bound_y = i_scaled_height + i_offset_y;
		// full width occupied, nothing to skip
	}
	else if (i_width < i_height)
	{
		i_scaled_width = static_cast<int>(128.0 * static_cast<float>(i_width) / static_cast<float>(i_height) + 0.5);
		i_offset_x = (128 - i_scaled_width + 1) >> 1; // (divide by 2 rounding up)
		i_bound_x = i_scaled_width + i_offset_x;
	}

	// Warning: will fail silently if not enough RAM! (dynamic allocation)
	stbir_resize(p_input_buffer, i_width, i_height, 0, p_intermediate_buffer, i_scaled_width, i_scaled_height, 0, STBIR_RGB, STBIR_TYPE_UINT8_SRGB, STBIR_EDGE_CLAMP, use_filter);

	return p_intermediate_buffer;
}

		/* static inline uint8_t * scale(int i_width, int i_height, uint8_t *p_input_buffer, uint8_t *p_intermediate_buffer)
		{
			// If no scaling needed, p_intermediate_buffer is not used
			if (i_width == 128 && i_height == 128) {
				return p_input_buffer;
			}

			// Else
			// Select filter
			stbir_filter use_filter = STBIR_FILTER_CATMULLROM; // for lowest resolution
			if (i_width > 128 || i_height > 128) { // down-sample
				use_filter = STBIR_FILTER_POINT_SAMPLE;
			}
			else if (i_width > 72 || i_height > 72) {  // 72-128 px
				use_filter = STBIR_FILTER_TRIANGLE;
			}

			// Compute scaled dimensions keeping aspect ratio, and offsets for center alignment
			int i_scaled_width = 128;
			int i_scaled_height = 128;
			int i_offset_bytes = 0;
			int i_skip_bytes = 0;
			if (i_width > i_height)
			{
				i_scaled_height = static_cast<int>(128.0 * static_cast<float>(i_height) / static_cast<float>(i_width) + 0.5);
				i_offset_bytes = ((128 - i_scaled_height) >> 1) * 128 * 3;
				// full width occupied, nothing to skip
			}
			else if (i_width < i_height)
			{
				i_scaled_width = static_cast<int>(128.0 * static_cast<float>(i_width) / static_cast<float>(i_height) + 0.5);
				i_skip_bytes = (128 - i_scaled_width) * 3;
				i_offset_bytes = ((128 - i_scaled_width + 1) >> 1) * 3; // (divide by 2 rounding up, then x3) full height occupied, but padding at the begining of the copy goes here
			}

			// Warning: will fail silently if not enough RAM! (dynamic allocation)
			stbir_resize(p_input_buffer, i_width, i_height, 0, p_intermediate_buffer, i_scaled_width, i_scaled_height, 0, STBIR_RGB, STBIR_TYPE_UINT8_SRGB, STBIR_EDGE_CLAMP, use_filter);

			// Center scaled frame on black canvas (= display)
			if (i_offset_bytes > 0 || i_skip_bytes > 0)
			{
				uint8_t *p_canvas = p_intermediate_buffer + (128 * 128 * 3 - 1);
				uint8_t *p_frame = p_intermediate_buffer + (i_scaled_width * i_scaled_height * 3 - 1);

				// Clear from end of canvas to (end - offset)
				for (int i = 0 ; i < i_offset_bytes ; i++)
				{
					*p_canvas-- = 0;
				}

				// Copy frame with skip cleared pixels between each line
				for (int y = 1 ; y < i_scaled_height ; y++) // don't do the last line here
				{
					for (int x = 0 ; x < i_scaled_width ; x++)
					{
						*p_canvas-- = *p_frame--; // B
						*p_canvas-- = *p_frame--; // G
						*p_canvas-- = *p_frame--; // R
					}
					for (int x = 0 ; x < i_skip_bytes ; x++)
					{
						*p_canvas-- = 0;
					}
				}

				// Copy last line without skip
				for (int x = 0 ; x < i_scaled_width ; x++)
				{
					*p_canvas-- = *p_frame--; // B
					*p_canvas-- = *p_frame--; // G
					*p_canvas-- = *p_frame--; // R
				}

				// Clear from end of drawing to begining of canvas
				while (p_canvas >= p_intermediate_buffer)
				{
					*p_canvas-- = 0;
				}
			}

			return p_intermediate_buffer;
		} */


inline bool Display::test_clear()
{
	uint32_t notifications = 0;
	BaseType_t res = xTaskNotifyWaitIndexed(0, 0x00000000, 0x00000001, &notifications, 1); // Slot 0 is dedicated so value can be read and reset anywhere.
	return res == pdTRUE && (notifications | 0x00000001);
}

inline void Display::show_frame(uint8_t *p_rgb_image, unsigned int i_frame_period)
{
	// Consume previous frame duration
	TickType_t delay = pdMS_TO_TICKS(i_current_frame_period);
	if (delay > 1) {
		vTaskDelayUntil(&i_last_frame_time, delay-1); // test_clear() adds 1 tick delay
	}

	// Clear the display and do nothing or show next frame?
	if (test_clear())
	{
		// Clear screen
		p_virtualmatrix->clearScreen();
		// New image can be displayed immediately
		i_current_frame_period = 0;
	}
	else
	{
		// Set timing parameters of the new frame
		i_last_frame_time = xTaskGetTickCount(); // vTaskDelayUntil actually never sets i_last_frame_time to current tick count, it is just adding the delay to the value so when the deadline is in the past it stays that way forever
		i_current_frame_period = i_frame_period;

		// Transfer frame
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
// The compiler can't guess bounds of p_rgb_image incrementation.
		for (int16_t y = 0 ; y < 128 ; y++)
		{
			if (y >= i_offset_y && y < i_bound_y)
			{
				for (int16_t x = 0 ; x < 128 ; x++)
				{
					if (x >= i_offset_x && x < i_bound_x)
					{
						p_virtualmatrix->drawPixelRGB888(x, y, *p_rgb_image++, *p_rgb_image++, *p_rgb_image++);
					}
					else
					{
						p_virtualmatrix->drawPixelRGB888(x, y, 0, 0, 0);
					}
				}
			}
			else
			{
				for (int16_t x = 0 ; x < 128 ; x++)
				{
					p_virtualmatrix->drawPixelRGB888(x, y, 0, 0, 0);
				}
			}
		}
#pragma GCC diagnostic pop
	}
/*
	uint32_t now = xTaskGetTickCount();
	ESP_LOGE("DEBUG", "Frame time %lu,%lu", i_last_frame_time, now);*/
}

		/* inline void transfer(uint8_t *p_rgb_image)
		{
			for (int16_t y = 0 ; y < 128 ; y++)
			{
				if (y >= i_offset_y && y < i_bound_y)
				{
					for (int16_t x = 0 ; x < 128 ; x++)
					{
						if (x >= i_offset_x && x < i_bound_x)
						{
							p_virtualmatrix->drawPixelRGB888(x, y, *p_rgb_image++, *p_rgb_image++, *p_rgb_image++);
						}
						else
						{
							p_virtualmatrix->drawPixelRGB888(x, y, 0, 0, 0);
						}
					}
				}
				else
				{
					for (int16_t x = 0 ; x < 128 ; x++)
					{
						p_virtualmatrix->drawPixelRGB888(x, y, 0, 0, 0);
					}
				}
			}
		} */

		/* inline void transfer(uint8_t *p_rgb_image)
		{
			for (int16_t y = 0 ; y < 128 ; y++)
			{
				for (int16_t x = 0 ; x < 128 ; x++, p_rgb_image += 3)
				{
					p_virtualmatrix->drawPixelRGB888(x, y, *p_rgb_image, *(p_rgb_image + 1), *(p_rgb_image + 2));
				}
			}
		} */

inline void Display::release_slot_pop_from_q_release_mem(web_server_image_slot_t *p_slot)
{
	// Scratch pointer
	void *p_receive;

	// Release slot and pop from queue
	p_slot->b_isfree = true;
	xQueueReceive(hfrt_img_q, &p_receive, pdMS_TO_TICKS(10));

	// Release memory
	xSemaphoreGive(hfrt_img_mem_sem);
}

void Display::task_static_fn(void *parameters)
{
	static_cast<Display *>(parameters)->task();
}

void Display::task()
{
	void *p_receive = NULL;
	web_server_image_slot_t *p_slot;
	uint8_t *p_frame = NULL;
	uint8_t p_img_source[i_decode_buffer_size]; // 640x640 RGB buffer for rendered images
	uint8_t p_img_output[128*128*3]; // 128x128 RGB buffer (display canvas) for copying to HUB75 library
	/*
	TickType_t i_last_frame_time = xTaskGetTickCount();
	unsigned int i_frame_period = MIN_IMG_TIME; // time to keep what's currently displayed
	*/
	i_last_frame_time = xTaskGetTickCount();
	i_current_frame_period = i_min_img_dur;

	while (true)
	{
		// Handle clear notification
		if (test_clear())
		{
			// Clear screen
			p_virtualmatrix->clearScreen();
			// New image can be displayed immediately
			i_current_frame_period = 0;
		}

		// Wait for a new item in the queue
		BaseType_t res = xQueuePeek(hfrt_img_q, &p_receive, pdMS_TO_TICKS(10));
		if (res != pdTRUE) {
			continue;
		}

		// Lock memory
		xSemaphoreTake(hfrt_img_mem_sem, portMAX_DELAY);

		// Get item from the queue
		if (xQueuePeek(hfrt_img_q, &p_receive, pdMS_TO_TICKS(10)) == pdTRUE)
		{
			p_slot = static_cast<web_server_image_slot_t *>(p_receive);

			switch (p_slot->e_format)
			{
			/*case IMG_CLEAR:
				release_slot_pop_from_q_release_mem(p_slot);
				// Clear screen
				p_virtualmatrix->clearScreen();
				// New image can be displayed immediately
				i_frame_period = 10;
				break;*/

			case IMG_FMT_PNG:
				{
					p_frame = NULL; // will redirect to one of the buffers depending on scaling being done or not
					PNGRenderer renderer;

					if (renderer.read<DISPLAY_RENDERER_DECODE_SIZE, LOG_TAG>(p_slot->p_data, web_server_image_buffer_size, p_img_source))
					{
						p_frame = scale(renderer.decoder.getWidth(), renderer.decoder.getHeight(), p_img_source, p_img_output);
					}

					release_slot_pop_from_q_release_mem(p_slot);

					// Show
					if (p_frame != NULL)
					{
						/*vTaskDelayUntil(&i_last_frame_time, pdMS_TO_TICKS(i_frame_period));
						i_last_frame_time = xTaskGetTickCount(); // vTaskDelayUntil actually never sets i_last_frame_time to current tick count, il is just adding the delay to the value so when the deadline is in the past it stays that way forever
						transfer(p_frame);
						i_frame_period = MIN_IMG_TIME;*/
						show_frame(p_frame, i_min_img_dur);
					}
				}
				break;

			case IMG_FMT_GIF:
				{
					p_frame = NULL; // will redirect to one of the buffers depending on scaling being done or not

					GIFRenderer renderer(p_img_source, i_gif_aggressive_transparency_lvl);

					if (renderer.load<LOG_TAG>(p_slot->p_data, web_server_image_buffer_size))
					{
						unsigned int i_total_play_duration = 0;
						unsigned int i_loop = 0;
						bool b_loop;

						do
						{
							b_loop = false;
							unsigned int i_play_duration = 0;
							int i_next_frame_period;
							int i_animation_state;

							// Set canvas to black for first frame with transparency
							memset(p_img_source, 0, 3 * renderer.decoder.getCanvasHeight() * renderer.decoder.getCanvasWidth());

							// Try to decode first frame
							//i_animation_state = decoder.playFrame(false, &i_next_frame_period, static_cast<void *>(&renderer_ctx));
							renderer.play(i_animation_state, i_next_frame_period);

							if (i_animation_state != 1) // Will stop (1 frame or error)
							{
								renderer.decoder.close();
								release_slot_pop_from_q_release_mem(p_slot);
							}

							if ((i_animation_state == 0) && (renderer.decoder.getLastError() == GIF_SUCCESS)) // One frame, treat as static image
							{
								p_frame = scale(renderer.decoder.getCanvasWidth(), renderer.decoder.getCanvasHeight(), p_img_source, p_img_output);
								/*vTaskDelayUntil(&i_last_frame_time, pdMS_TO_TICKS(i_frame_period));
								i_last_frame_time = xTaskGetTickCount(); // vTaskDelayUntil actually never sets i_last_frame_time to current tick count, il is just adding the delay to the value so when the deadline is in the past it stays that way forever
								transfer(p_frame);
								i_frame_period = MIN_IMG_TIME;*/
								show_frame(p_frame, i_min_img_dur);
							}
							else if (i_animation_state == 1) // Animate
							{
								while (true)
								{
									// Disallow 0 ms frame period
									if (i_next_frame_period < 10) {
										i_next_frame_period = 10;
									}

									// Accumulate play duration then check if we continue
									i_play_duration += i_next_frame_period;
									if (i_play_duration > i_max_total_gif_dur && uxQueueMessagesWaiting(hfrt_img_q) > 1) // Stop
									{
										renderer.decoder.close();
										release_slot_pop_from_q_release_mem(p_slot);
										break;
									}

									// Release memory but keep slot in the queue
									xSemaphoreGive(hfrt_img_mem_sem);

									// Show frame
									p_frame = scale(renderer.decoder.getCanvasWidth(), renderer.decoder.getCanvasHeight(), p_img_source, p_img_output);
									/*vTaskDelayUntil(&i_last_frame_time, pdMS_TO_TICKS(i_frame_period));
									i_last_frame_time = xTaskGetTickCount(); // vTaskDelayUntil actually never sets i_last_frame_time to current tick count, il is just adding the delay to the value so when the deadline is in the past it stays that way forever
									transfer(p_frame); // next frame becomes current frame
									i_frame_period = i_next_frame_period;*/
									show_frame(p_frame, i_next_frame_period);

									// Reacquire slot
									// Lock memory
									xSemaphoreTake(hfrt_img_mem_sem, portMAX_DELAY);
									// Get item from the queue
									if (xQueuePeek(hfrt_img_q, &p_receive, pdMS_TO_TICKS(10)) != pdTRUE)
									{
										// Queue was reset in the meantime
										// No more data
										renderer.decoder.close();
										// Release memory
										xSemaphoreGive(hfrt_img_mem_sem);
										break;
									}
									// Is it the same slot?
									// /!\ Assumption: the slot cannot reappear with different data because this function is the only place where a slot is invalidated
									if (p_slot != p_receive)
									{
										// Data isn't coherent anymore
										renderer.decoder.close();
										// Release memory
										xSemaphoreGive(hfrt_img_mem_sem);
										break;
									}

									// Decode next frame and decide what happens after
									//i_animation_state = decoder.playFrame(false, &i_next_frame_period, static_cast<void *>(&renderer_ctx));
									renderer.play(i_animation_state, i_next_frame_period);
									if (i_animation_state == 1) { // continue
										continue;
									}
									else if (i_animation_state == 0) // animation is done
									{
										if (GIF_SUCCESS == renderer.decoder.getLastError()) // Decoder may produce garbage as last frame
										{
											// Show last frame
											if (i_next_frame_period < 10) {
												i_next_frame_period = 10;
											}
											i_play_duration += i_next_frame_period;
											if (i_play_duration > i_max_total_gif_dur && uxQueueMessagesWaiting(hfrt_img_q) > 1) {
												goto cleanup;
											}
											p_frame = scale(renderer.decoder.getCanvasWidth(), renderer.decoder.getCanvasHeight(), p_img_source, p_img_output);
											/*vTaskDelayUntil(&i_last_frame_time, pdMS_TO_TICKS(i_frame_period));
											i_last_frame_time = xTaskGetTickCount(); // vTaskDelayUntil actually never sets i_last_frame_time to current tick count, it is just adding the delay to the value so when the deadline is in the past it stays that way forever
											transfer(p_frame);
											i_frame_period = i_next_frame_period;*/
											show_frame(p_frame, i_next_frame_period);
										}

										// Decide what's next
										i_total_play_duration += i_play_duration;
										i_loop++;

										/* Rules for a GIF to play again:
												- there is nothing else in the queue
												or
												- the maximum play duration will not be exceeded and it has looped less than set times already */
										if (   (uxQueueMessagesWaiting(hfrt_img_q) < 2)
												|| (((i_total_play_duration + i_play_duration) < i_max_total_gif_dur) && (i_loop < i_min_gif_loops)) )
										{
											renderer.reset();
											b_loop = true;
											break;
										}
										// else
										cleanup:
											renderer.decoder.close();
											release_slot_pop_from_q_release_mem(p_slot);
									}
									else // Failure, abort
									{
										renderer.decoder.close();
										release_slot_pop_from_q_release_mem(p_slot);
										ESP_LOGE(LOG_TAG, "GIF file decode error: %d", renderer.decoder.getLastError());
									}

									break;
								}
							}
							else { // Empty frame or failure, abort
								ESP_LOGE(LOG_TAG, "GIF file decode error: %d", renderer.decoder.getLastError());
							}

						} while (b_loop);

					}
				}
				break;

			default:
				// Should never happen!
				// Release slot and pop from queue
				p_slot->b_isfree = true;
				xQueueReceive(hfrt_img_q, &p_receive, pdMS_TO_TICKS(10));
				// Release memory
				xSemaphoreGive(hfrt_img_mem_sem);
				break;
			}

		}
		else // queue was reset in the meantime
		{
			// Release memory
			xSemaphoreGive(hfrt_img_mem_sem);
		}

	}
}

//#undef LOG_TAG
