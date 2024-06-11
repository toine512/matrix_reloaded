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

#include "gif.h"

GIFRenderer::GIFRenderer(uint8_t *p_destination, int i_transparency_mode) :
	decoder{},
	ctx{p_destination, decoder, i_transparency_mode, false, false, false, {0, 0, 0}}
{ }

void GIFRenderer::render(GIFDRAW *p_draw)
{
	// The context was passed to AnimatedGIF::playFrame()
	GIFRenderContext *ctx(static_cast<GIFRenderContext *>(p_draw->pUser));

	// Parameters
	bool b_first_frame = ctx->b_first_frame;
	bool b_transparency = static_cast<bool>(p_draw->ucHasTransparency);
	uint8_t i_transparent_index = p_draw->ucTransparent;

	// Determine background using first frame
	if (b_first_frame)
	{
		ctx->b_first_frame = false;

		if (p_draw->bHasGlobalPalette) // Is background valid?
		{
			ctx->b_background = true; // background is valid

			uint8_t i_background_index = p_draw->ucBackground; // global palette index of background color

			// Is the background transparent? 3 modes
			switch (ctx->i_transparency_mode)
			{
			case 1: // aggressive level 1: background is transparent if the background color is the transparent color of the first frame
				ctx->b_background_transparent = b_transparency && (i_transparent_index == i_background_index);
				break;

			case 2: // aggressive level 2: background is transparent if first frame has any transparent color
				ctx->b_background_transparent = b_transparency;
				break;

			default: // aggressive level 0: background color is honored
				ctx->b_background_transparent = false;
				break;
			}

			// Copy the background color as defined in the global palette
			uint8_t *p_color = reinterpret_cast<uint8_t *>(p_draw->pGlobalPalette) + 3 * i_background_index;
			ctx->background_color[0] = p_color[0]; // R
			ctx->background_color[1] = p_color[1]; // G
			ctx->background_color[2] = p_color[2]; // B
		}
		else
		{
			ctx->b_background = false;
			ctx->b_background_transparent = false;
		}
	}

	// Palette
	uint8_t *p_palette = reinterpret_cast<uint8_t *>(p_draw->pPalette);

	// Pixels
	uint8_t *p_decoded = p_draw->pPixels;

	// Output buffer
	uint8_t *p_dest_buffer = ctx->p_destination;
	// Compute offset: frame can be drawn anywhere on the canvas
	p_dest_buffer += ((p_draw->y + p_draw->iY) * ctx->decoder.getCanvasWidth() + p_draw->iX) * 3; // Destination buffer is 3x8bit RGB

	// Line length in pixels
	int i_width = p_draw->iWidth;

	if (b_transparency) // transparent color has to be replaced
	{
		bool b_restore_to_background = p_draw->ucDisposalMethod == 2;
		bool b_background = ctx->b_background;
		uint8_t black_color[3] = {0, 0, 0};
		uint8_t *background_color = ctx->b_background_transparent ? black_color : ctx->background_color; // For us a transparent background is transparent to black, a color is a 3 byte array

		for (int x = 0; x < i_width; x++)
		{
			uint8_t i_index = *p_decoded++;
			uint8_t *p_color;

			if (i_index == i_transparent_index)
			{
				if (b_restore_to_background && b_background) // If restore to background but no valid background, fallback to mode 1 keep
				{
					p_color = background_color;
				}
				else
				{
					p_dest_buffer += 3;
					continue;
				}
			}
			else
			{
				p_color = p_palette + 3 * i_index;
			}

			*p_dest_buffer++ = p_color[0]; // R
			*p_dest_buffer++ = p_color[1]; // G
			*p_dest_buffer++ = p_color[2]; // B
		}
	}
	else // no transparent color
	{
		for (int x = 0; x < i_width; x++)
		{
			uint8_t *p_color = p_palette + 3 * *p_decoded++;

			*p_dest_buffer++ = p_color[0]; // R
			*p_dest_buffer++ = p_color[1]; // G
			*p_dest_buffer++ = p_color[2]; // B
		}
	}
}
