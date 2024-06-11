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

#include "wifi.h"

static esp_netif_t *wifi_sta_intf = NULL;

void wifi_connection_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
/*
	if (event_base != WIFI_EVENT) {
		return;
	}
	switch (event_id) {

		case WIFI_EVENT_STA_START:
			ESP_ERROR_CHECK( esp_wifi_connect() );
			break;

	}
*/
	if (event_base == WIFI_EVENT) {
		if (event_id == WIFI_EVENT_STA_START || event_id == WIFI_EVENT_STA_DISCONNECTED) {
			ESP_ERROR_CHECK( esp_wifi_connect() );
		}
	}
}

// ESP-NETIF must be initialised beforehand
esp_err_t wifi_sta_init()
{
	wifi_sta_intf = esp_netif_create_default_wifi_sta(); // creates a Wi-Fi station driver and binds to ESP-NETIF

	wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT(); // parameters from menuconfig
	RETURN_FAILED( esp_wifi_init(&init_cfg) ); // creates the Wi-Fi task

	return esp_wifi_set_mode(WIFI_MODE_STA);
}

// Actions
/* ESP Wi-Fi driver must be initialised before configuration.
   Reconfiguring while connected causes to reconnect.         */
esp_err_t wifi_sta_config(const char *country_code, const char *ssid, const char *password)
{
	wifi_config_t cfg;
	RETURN_FAILED( esp_wifi_get_config(WIFI_IF_STA, &cfg) );

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
// Truncation according to strncpy definition is intended.
	strncpy((char *)cfg.sta.ssid, ssid, 32*sizeof(uint8_t));
	strncpy((char *)cfg.sta.password, password, 64*sizeof(uint8_t));
#pragma GCC diagnostic pop
	cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	cfg.sta.channel = 0;
	cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

	RETURN_FAILED( esp_wifi_set_config(WIFI_IF_STA, &cfg) );

	RETURN_FAILED( esp_wifi_set_country_code(country_code, false) );

	return esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40);
}

esp_err_t wifi_sta_start()
{
	RETURN_FAILED( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_connection_handler, NULL) );
	return esp_wifi_start();
}

esp_err_t wifi_sta_stop()
{
	RETURN_FAILED( esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_connection_handler) );
	return esp_wifi_stop();
}
