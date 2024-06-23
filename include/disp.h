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

// Configuration
#define DISPLAY_TASK_PRIORITY 20
#define DISPLAY_RENDERER_DECODE_SIZE 640 // pixels square, must match png.h

// C Standard Library
#include <cstdint>
#include <cstring>
// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/idf_additions.h>
// ESP-IDF
#include <esp_err.h>
#include <esp_log.h>
#include <esp_check.h>
#include <esp_heap_caps.h>
// Dependencies
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
// App
#include "web.h"
#include "gif.h"
#include "png.h"

class Display
{
	struct PNGRenderContext
	{
		uint8_t *p_destination;
		PNG &decoder;
	};

	public:
		static constexpr uint16_t i_disp_width = MTX_HUB75_PANEL_W * MTX_HUB75_COLS;
		static constexpr float f_disp_width = static_cast<float>(i_disp_width);
		static constexpr uint16_t i_disp_height = MTX_HUB75_PANEL_H * MTX_HUB75_ROWS;
		static constexpr float f_disp_height = static_cast<float>(i_disp_height);
		static constexpr size_t i_disp_bufsize = i_disp_width * i_disp_height * 3;

		Display(TaskHandle_t &hfrt_display_task);
		~Display();
		esp_err_t start(SemaphoreHandle_t in_use_semaphore, QueueHandle_t image_queue, uint8_t configured_brightness, unsigned int min_image_duration, unsigned int max_total_gif_duration, unsigned int min_gif_loops, int gif_aggressive_transp_level, uint8_t clkdiv_num, uint8_t clkdiv_a, uint8_t clkdiv_b);
		void destroy();
		void suspend_task();

	protected:
		TaskHandle_t &hfrt_display_task;
		SemaphoreHandle_t hfrt_img_mem_sem;
		QueueHandle_t hfrt_img_q;
		unsigned int i_min_img_dur;
		unsigned int i_max_total_gif_dur;
		unsigned int i_min_gif_loops;
		int i_gif_aggressive_transparency_lvl;

		uint16_t i_offset_x = 0;
		uint16_t i_bound_x = 0;
		uint16_t i_offset_y = 0;
		uint16_t i_bound_y = 0;

		inline uint8_t * scale(int i_width, int i_height, uint8_t *p_input_buffer, uint8_t *p_intermediate_buffer);

	private:
		static constexpr size_t i_decode_bufsize = DISPLAY_RENDERER_DECODE_SIZE * DISPLAY_RENDERER_DECODE_SIZE * 3;

		MatrixPanel_I2S_DMA *p_hub75_panel;
		VirtualMatrixPanel *p_virtualmatrix;

		TickType_t i_last_frame_time;
		unsigned int i_current_frame_period;

		inline bool test_clear();
		inline void show_frame(uint8_t *p_rgb_image, unsigned int i_frame_period);
		inline void release_slot_pop_from_q_release_mem(web_server_image_slot_t *p_slot);
		static void task_static_fn(void *parameters);
		void task();
};
