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

#include "png.h"
#include "helpers.h"

PNGRenderer::PNGRenderer() :
	decoder()
{}

void PNGRenderer::render(PNGDRAW *p_draw)
{
	// The context was passed to PNG::decode()
	PNGRenderContext *ctx(static_cast<PNGRenderContext *>(p_draw->pUser));

	// Parameters
	bool b_transparency = ctx->decoder.hasAlpha();
	int32_t transparent_color = ctx->decoder.getTransparentColor();

	// Output
	uint8_t *p_dest_buffer = ctx->p_destination;

	// Line length in pixels
	int i_width = p_draw->iWidth;

	// Compute offset: we write line by line therefore the offset is based on line index (y), both drawing and output are referenced to top left = (0, 0) positive coordinates
	p_dest_buffer += p_draw->y * 3 * i_width; // Destination buffer is 3x8bit RGB

	switch (p_draw->iPixelType)
	{
	case PNG_PIXEL_TRUECOLOR_ALPHA:
		{
			uint32_t *p_decoded = reinterpret_cast<uint32_t *>(p_draw->pPixels);

			for (int x = 0 ; x < i_width ; x++)
			{
				uint32_t px = *p_decoded++; // ABGR because PNG order is RGBA while this platform is big endian, thus the 4-bytes are reverse-read
				uint32_t a = (px & 0xff000000) >> 24;
				FbByteMul(px, a);
				*p_dest_buffer++ = static_cast<uint8_t>(px);       // R
				*p_dest_buffer++ = static_cast<uint8_t>(px >> 8);  // G
				*p_dest_buffer++ = static_cast<uint8_t>(px >> 16); // B
			}
		}
		break;

	case PNG_PIXEL_TRUECOLOR:
		if (b_transparency) // transparent color has to be replaced
		{
			uint8_t *p_decoded = p_draw->pPixels;

			for (int x = 0 ; x < i_width ; x++)
			{
				// 0RGB
				uint32_t px  = static_cast<uint32_t>(*p_decoded++) << 16; // R
									px |= static_cast<uint32_t>(*p_decoded++) << 8;  // G
									px |= static_cast<uint32_t>(*p_decoded++);       // B

				if (px == transparent_color)
				{
					*p_dest_buffer++ = 0;
					*p_dest_buffer++ = 0;
					*p_dest_buffer++ = 0;
				}
				else
				{
					*p_dest_buffer++ = static_cast<uint8_t>(px >> 16); // R
					*p_dest_buffer++ = static_cast<uint8_t>(px >> 8);  // G
					*p_dest_buffer++ = static_cast<uint8_t>(px);       // B
				}
			}
		}
		else { // no transparent color
			memcpy(static_cast<void *>(p_dest_buffer), static_cast<void *>(p_draw->pPixels), 3 * i_width); // direct RGB copy
		}
		break;

	case PNG_PIXEL_INDEXED:
		{
			uint8_t *p_decoded = p_draw->pPixels;
			uint8_t *p_palette = p_draw->pPalette;
			int i_bpp = p_draw->iBpp;
			int i_pixels_per_byte = 8 / i_bpp;
			int bitmask_selector = (1 << i_bpp) - 1;

			// Premultiply palette if RGBA
			if (b_transparency)
			{
				uint8_t *p_alpha = p_draw->pPalette;

				for (int i = 768 ; i < 1024 ; i++)
				{
					// 0RGB
					uint32_t color  = static_cast<uint32_t>(p_palette[0]) << 16; // R
										color |= static_cast<uint32_t>(p_palette[1]) << 8;  // G
										color |= static_cast<uint32_t>(p_palette[2]);       // B
					uint32_t a = static_cast<uint32_t>(p_alpha[i]);
					FbByteMul(color, a);

					/*uint8_t *p_ch = reinterpret_cast<uint8_t *>(&color); // 4 bytes
					*p_palette++ = p_ch[1]; // R
					*p_palette++ = p_ch[2]; // G
					*p_palette++ = p_ch[3]; // B*/
					*p_palette++ = static_cast<uint8_t>(color >> 16); // R
					*p_palette++ = static_cast<uint8_t>(color >> 8);  // G
					*p_palette++ = static_cast<uint8_t>(color);       // B
				}

				// Reset palette pointer
				p_palette = p_draw->pPalette;
			}

			for (int x = 0 ; x < i_width ; x += i_pixels_per_byte)
			{
				uint8_t i_index = *p_decoded++;

				for (int slice = 8 - i_bpp ; slice >= 0 ; slice -= i_bpp)
				{
					uint8_t *p_color = p_palette + 3 * ((i_index & (bitmask_selector << slice)) >> slice);

					*p_dest_buffer++ = p_color[0];
					*p_dest_buffer++ = p_color[1];
					*p_dest_buffer++ = p_color[2];
				}
			}
		}
		break;

	case PNG_PIXEL_GRAY_ALPHA:
		{
			uint8_t *p_decoded = p_draw->pPixels;

			for (int x = 0 ; x < i_width ; x++)
			{
				uint_fast16_t px = static_cast<uint_fast16_t>(*p_decoded++);
				uint8_t a = *p_decoded++;
				AlphaMult(px, a);

				*p_dest_buffer++ = static_cast<uint8_t>(px);
				*p_dest_buffer++ = static_cast<uint8_t>(px);
				*p_dest_buffer++ = static_cast<uint8_t>(px);
			}
		}
		break;

	case PNG_PIXEL_GRAYSCALE:
		{
			uint8_t *p_decoded = p_draw->pPixels;
			int i_bpp = p_draw->iBpp;
			int i_pixels_per_byte = 8 / i_bpp;
			uint8_t bitmask_selector = (1 << i_bpp) - 1;
			uint8_t i_mul = 1;
			if (i_bpp == 4) {
				i_mul = 17;
			} else if (i_bpp == 2) {
				i_mul = 85;
			} else if (i_bpp == 1) {
				i_mul = 255;
			}

			if (b_transparency) // transparent color has to be replaced
			{
				uint8_t transparent_byte = static_cast<uint8_t>(transparent_color);

				for (int x = 0; x < i_width; x += i_pixels_per_byte)
				{
					uint8_t i_level = *p_decoded++;

					for (int slice = 8 - i_bpp; slice >= 0; slice -= i_bpp)
					{
						uint8_t i_level_slice = (i_level & (bitmask_selector << slice)) >> slice;

						if (i_level_slice == transparent_byte) {
							i_level_slice = 0;
						} else {
							i_level_slice *= i_mul;
						}

						*p_dest_buffer++ = i_level_slice;
						*p_dest_buffer++ = i_level_slice;
						*p_dest_buffer++ = i_level_slice;
					}
				}
			}
			else // no transparent color
			{
				for (int x = 0; x < i_width; x += i_pixels_per_byte)
				{
					uint8_t i_level = *p_decoded++;

					for (int slice = 8 - i_bpp; slice >= 0; slice -= i_bpp)
					{
						uint8_t i_level_slice = ((i_level & (bitmask_selector << slice)) >> slice) * i_mul;

						*p_dest_buffer++ = i_level_slice;
						*p_dest_buffer++ = i_level_slice;
						*p_dest_buffer++ = i_level_slice;
					}
				}
			}
		}
		break;

	default:
		break;
	}
}
