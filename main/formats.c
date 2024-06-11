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

#include "formats.h"

// Requires at least 8 B of data!!
inline enum ImageFormat detect_image_format(uint8_t *p_data)
{
	static const uint8_t png_tag[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}; // 0x89 P N G CR LF EOF LF
	static const uint8_t gif87a_tag[6] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61}; // G I F 8 7 a
	static const uint8_t gif89a_tag[6] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61}; // G I F 8 9 a

	if (0 == memcmp((void *)p_data, (void *)&png_tag, 8))
	{
		return IMG_FMT_PNG;
	}
	else if (0 == memcmp((void *)p_data, (void *)&gif89a_tag, 6) || 0 == memcmp((void *)p_data, (void *)&gif87a_tag, 6))
	{
		return IMG_FMT_GIF;
	}

	return IMG_FMT_UNSUPPORTED;
}
