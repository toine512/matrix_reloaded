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
#define PNG_MAX_LINE_LENGTH 640 // must match disp.h

// C Standard Library
#include <cstdint>
// ESP-IDF
#include <esp_log.h>
// Larry Bank's PNGdec library
#include <PNGdec.h>
// App
#include "cpp_string_literal.h"

class PNGRenderer
{
	struct PNGRenderContext
	{
		uint8_t *p_destination;
		PNG &decoder;
	};

	public:
		PNG decoder;

		PNGRenderer();

		template<unsigned int decode_size, StringLiteral log_tag>
		inline bool read(uint8_t *p_data, int i_data_size, uint8_t *p_dest_buffer)
		{
			// Open image
			if (PNG_SUCCESS != decoder.openRAM(p_data, i_data_size, render))
			{
				ESP_LOGE(log_tag.chars, "PNG image open error: %d", decoder.getLastError());
				return false;
			}

			// Check size
			if (decoder.getWidth() > decode_size || decoder.getHeight() > decode_size)
			{
				ESP_LOGW(log_tag.chars, "Image resolution too large. Maximum is %ux%u.", decode_size, decode_size);
				decoder.close();
				return false;
			}

			PNGRenderContext renderer_ctx = {
				.p_destination = p_dest_buffer,
				.decoder = decoder
			};

			// Read, decode, render image
			if (PNG_SUCCESS != decoder.decode(static_cast<void *>(&renderer_ctx), 0))
			{
				ESP_LOGE(log_tag.chars, "PNG image decode error: %d", decoder.getLastError());
				decoder.close();
				return false;
			}

			return true;
		}

		private:
			static void render(PNGDRAW *p_draw);
};
