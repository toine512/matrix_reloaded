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

#include <string.h>
#include <esp_err.h>

// https://www.rkoucha.fr/tech_corner/c_preprocessor.html

#define SZ_COMP(_a, _b) (0 == strcmp(_a, _b))
#define SZ_COMP2(_a1, _b1, _a2, _b2) (SZ_COMP(_a1, _b1) && SZ_COMP(_a2, _b2))
#define GPIO_TO_MASK(_pin) (1ULL << _pin)

#define RETURN_FAILED(exp) \
do { \
	esp_err_t res = (exp);           \
	if (res != ESP_OK) {return res;} \
} while(0)

/* ESP_ERROR_CHECK whatever the configuration
   Copy of esp_err.h code.                    */
#define ABORT_ON_ERROR(x) \
do { \
	esp_err_t err_rc_ = (x);                                                   \
	if (unlikely(err_rc_ != ESP_OK))                                           \
	{                                                                          \
		_esp_error_check_failed(err_rc_, __FILE__, __LINE__, __ASSERT_FUNC, #x); \
	}                                                                          \
} while (0)


/* x is a variable modified in place
   a can be an expression */
#define AlphaMult(x, a) \
do { \
	x = x * (a) + 0x0080;    \
	x = (x + (x >> 8)) >> 8; \
} while (0)

#define FbByteMul(x, a) \
do { \
	uint32_t t = (x & 0xff00ff) * (a) + 0x800080; \
	t = (t + ((t >> 8) & 0xff00ff)) >> 8;         \
	t &= 0xff00ff;                                \
	x = ((x >> 8) & 0xff00ff) * (a) + 0x800080;   \
	x = (x + ((x >> 8) & 0xff00ff));              \
	x &= 0xff00ff00;                              \
	x += t;                                       \
} while (0)
