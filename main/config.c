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

#include "config.h"
#include "inih_58/ini.h"
#include "helpers.h"

#define LOG_TAG "Configuration Reader"

bool wifi_check_cc(const char *country_code)
{
	// List taken from esp_wifi.h
	const char valid_cc[][2] = {{'0','1'},{'A','T'},{'A','U'},{'B','E'},{'B','G'},{'B','R'},{'C','A'},{'C','H'},{'C','N'},{'C','Y'},{'C','Z'},{'D','E'},{'D','K'},{'E','E'},{'E','S'},{'F','I'},{'F','R'},{'G','B'},{'G','R'},{'H','K'},{'H','R'},{'H','U'},{'I','E'},{'I','N'},{'I','S'},{'I','T'},{'J','P'},{'K','R'},{'L','I'},{'L','T'},{'L','U'},{'L','V'},{'M','T'},{'M','X'},{'N','L'},{'N','O'},{'N','Z'},{'P','L'},{'P','T'},{'R','O'},{'S','E'},{'S','I'},{'S','K'},{'T','W'},{'U','S'}};

	for (unsigned int i = 0 ; i < sizeof(valid_cc)/sizeof(valid_cc[0]) ; i++)
	{
		if (0 == strncmp(country_code, valid_cc[i], 2)) {
			return true;
		}
	}

	return false;
}

typedef struct reader_ctx {
	global_config_t *p_cfg;
	bool valid_entries[GLOBAL_CONFIG_ENTRIES];
} reader_ctx_t;

// 0 = error, 1 = success
static int line_handler(void* p_user, const char* sz_section, const char* sz_name, const char* sz_value)
{
	reader_ctx_t *p_ctx = (reader_ctx_t *)p_user;
	global_config_t *p_cfg = p_ctx->p_cfg;

	#define ENTRY(_section, _key) SZ_COMP2(sz_section, _section, sz_name, _key)
	#define VALIDATE(_index) p_ctx->valid_entries[_index] = true; return 1

	if ENTRY("hub75", "brightness")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val < 1) {
			ESP_LOGE(LOG_TAG, "brightness => unable to interpret value or set to 0");
			return 0;
		}
		else if (val > 255) {
			ESP_LOGE(LOG_TAG, "brightness => value must be in range [1,255]");
			return 0;
		}
		else if (val > MTX_MAX_BRIGHTNESS) {
			ESP_LOGE(LOG_TAG, "brightness => set value %lu exceeds hardcoded power maximum %u", val, MTX_MAX_BRIGHTNESS);
			return 0;
		} else {
			p_cfg->i_disp_brightness = (uint8_t)val;
			VALIDATE(0);
		}
	}
	else if ENTRY("hub75", "lcdcam_clkdiv_num")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val > 256) { // let it overflow by 1 as 0 means N = 256
			ESP_LOGE(LOG_TAG, "lcdcam_clkdiv_num => f_LCDCLK = f_LCDCLKS / (N + b / a) / 2, NUM must be in range [0,256] (0 or 256: N=256, 1: N=2, else: N=NUM)");
			return 0;
		} else {
			p_cfg->i_lcdcam_clkdiv_num = (uint8_t)val;
			VALIDATE(1);
		}
	}
	else if ENTRY("hub75", "lcdcam_clkdiv_a")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val < 1) {
			ESP_LOGE(LOG_TAG, "lcdcam_clkdiv_a => unable to interpret value or set to 0: f_LCDCLK = f_LCDCLKS / (N + b / a) / 2, a can't be 0, a must be in range [1,63]");
			return 0;
		}
		else if (val > 63) {
			ESP_LOGE(LOG_TAG, "lcdcam_clkdiv_a => f_LCDCLK = f_LCDCLKS / (N + b / a) / 2, a must be in range [1,63]");
			return 0;
		} else {
			p_cfg->i_lcdcam_clkdiv_a = (uint8_t)val;
			VALIDATE(2);
		}
	}
	else if ENTRY("hub75", "lcdcam_clkdiv_b")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val > 63) {
			ESP_LOGE(LOG_TAG, "lcdcam_clkdiv_b => f_LCDCLK = f_LCDCLKS / (N + b / a) / 2, b must be in range [0,63]");
			return 0;
		} else {
			p_cfg->i_lcdcam_clkdiv_b = (uint8_t)val;
			VALIDATE(3);
		}
	}
	else if ENTRY("show", "image_min_duration")
	{
		p_cfg->i_disp_img_min_dur = (uint32_t)strtoul(sz_value, NULL, 10);
		VALIDATE(4);
	}
	else if ENTRY("show", "gif_max_total_duration")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val < 10) {
			ESP_LOGE(LOG_TAG, "gif_max_total_duration => unable to interpret value or is < 10 ms");
			return 0;
		} else {
			p_cfg->i_disp_gif_max_total_dur = (uint32_t)val;
			VALIDATE(5);
		}
	}
	else if ENTRY("show", "gif_min_loops")
	{
		unsigned long val = strtoul(sz_value, NULL, 10);
		if (val < 1) {
			ESP_LOGE(LOG_TAG, "gif_min_loops => unable to interpret value or set to 0");
			return 0;
		}
		else if (val > 255) {
			ESP_LOGE(LOG_TAG, "gif_min_loops => value must be in range [1,255]");
			return 0;
		}
		else {
			p_cfg->i_disp_gif_min_loops = (uint8_t)val;
			VALIDATE(6);
		}
	}
	else if ENTRY("show", "gif_transparency_mode")
	{
		int val = atoi(sz_value);
		if (val < 0 || val > 2) {
			ESP_LOGE(LOG_TAG, "gif_transparency_mode => must be in range [0,2]");
			return 0;
		} else {
			p_cfg->i_disp_gif_aggressive_transp_lvl = val;
			VALIDATE(7);
		}
	}
	else if ENTRY("wifi", "country")
	{
		if ( wifi_check_cc(sz_value) ) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
// Truncation according to strncpy definition is intended.
			strncpy(p_cfg->s_wifi_cc, sz_value, 2);
#pragma GCC diagnostic pop
			VALIDATE(8);
		} else {
			ESP_LOGE(LOG_TAG, "country => invalid ESP Wi-Fi country code");
			return 0;
		}
	}
	else if ENTRY("wifi", "ssid")
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
// Truncation according to strncpy definition is intended.
		strncpy(p_cfg->s_wifi_ssid, sz_value, 32);
#pragma GCC diagnostic pop
		VALIDATE(9);
	}
	else if ENTRY("wifi", "key")
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
// Truncation according to strncpy definition is intended.
		strncpy(p_cfg->s_wifi_key, sz_value, 64);
#pragma GCC diagnostic pop
		VALIDATE(10);
	}
	else if ENTRY("mdns", "hostname")
	{
		strncpy(p_cfg->sz_mdns_hostname, sz_value, 64);
		p_cfg->sz_mdns_hostname[64] = '\0';
		VALIDATE(11);
	}
	/*else if ENTRY("mdns", "instance_name")
	{
		strncpy(p_cfg->sz_mdns_inst_name, sz_value, 64);
		p_cfg->sz_mdns_inst_name[64] = '\0';
		VALIDATE(12);
	}*/

	// Else pass unknown entry
	ESP_LOGW(LOG_TAG, "Ignoring unknown entry [%s] %s.", sz_section, sz_name);
	return 1;

	#undef ENTRY
	#undef VALIDATE
}

esp_err_t config_load(global_config_t *cfg, const char *sz_path)
{
	reader_ctx_t ctx;
	ctx.p_cfg = cfg;
	memset((void *)ctx.valid_entries, 0, GLOBAL_CONFIG_ENTRIES*sizeof(bool));

	int res = ini_parse(sz_path, line_handler, (void *)&ctx);

	if (res == 0)
	{
		for (unsigned int i = 0 ; i < GLOBAL_CONFIG_ENTRIES ; i++)
		{
			if (!ctx.valid_entries[i])
			{
				ESP_LOGE(LOG_TAG, "Missing configuration entry! (code %u)", i);
				return ESP_ERR_NOT_FINISHED;
			}
		}

		return ESP_OK;
	}

	else if (res == -1)
	{
		ESP_LOGE(LOG_TAG, "Unable to open configuration file '%s'!", sz_path);
		return ESP_ERR_NOT_FOUND;
	}

	// else
	// Stop on first error must be enabled! (INI_STOP_ON_FIRST_ERROR 1)
	ESP_LOGE(LOG_TAG, "'%s' parse error on line %d!", sz_path, res);
	return ESP_ERR_INVALID_ARG;
}

#undef LOG_TAG
