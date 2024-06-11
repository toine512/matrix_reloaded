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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// ESP-IDF
#include <esp_err.h>
#include <esp_log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLOBAL_CONFIG_ENTRIES 12
typedef struct global_config {
	uint8_t i_disp_brightness; // 8 b
	uint8_t i_lcdcam_clkdiv_num; // 8 b with 0, 1 as special values
	uint8_t i_lcdcam_clkdiv_a; // 6 b
	uint8_t i_lcdcam_clkdiv_b; // 6 b
	uint32_t i_disp_img_min_dur;
	uint32_t i_disp_gif_max_total_dur;
	uint8_t i_disp_gif_min_loops;
	int i_disp_gif_aggressive_transp_lvl;
	char s_wifi_cc[2];
	char s_wifi_ssid[32];
	char s_wifi_key[64];
	char sz_mdns_hostname[65];
} global_config_t;


esp_err_t config_load(global_config_t *cfg, const char *sz_path);

#ifdef __cplusplus
}
#endif
