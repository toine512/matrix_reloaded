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

// C Standard Library
#include <cstdint>
// ESP-IDF
#include <esp_log.h>
// Larry Bank's AnimatedGIF library
#include <AnimatedGIF.h>
// App
#include "cpp_string_literal.h"

class GIFRenderer
{
	struct GIFRenderContext
	{
		uint8_t *p_destination;
		AnimatedGIF &decoder;
		int i_transparency_mode;
		bool b_first_frame;
		bool b_background;
		bool b_background_transparent;
		uint8_t background_color[3];
	};

	public:
		AnimatedGIF decoder;

		GIFRenderer(uint8_t *p_destination, int i_transparency_mode);

		template<StringLiteral log_tag>
		inline bool load(uint8_t *p_data, int i_data_size)
		{
			decoder.begin(GIF_PALETTE_RGB888);

			// Open file
			if (1 != decoder.open(p_data, i_data_size, render))
			{
				ESP_LOGE(log_tag.chars, "GIF file open error: %d", decoder.getLastError());
				return false;
			}

			ctx.b_first_frame = true;

			return true;
		}

		inline void reset()
		{
			ctx.b_first_frame = true;
			decoder.reset();
		}

		inline void play(int &i_animation_state, int &i_next_frame_period)
		{
			i_animation_state = decoder.playFrame(false, &i_next_frame_period, static_cast<void *>(&ctx));
		}

	protected:
		GIFRenderContext ctx;

	private:
		static void render(GIFDRAW *p_draw);
};
