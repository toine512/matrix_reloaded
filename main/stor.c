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

#include "stor.h"

#define LOG_TAG "Flash Storage"

static wl_handle_t hidf_storage = WL_INVALID_HANDLE;

esp_err_t storage_init_spiflash(const char *s_partition_label, tusb_msc_callback_t fn_callback_pre, tusb_msc_callback_t fn_callback_post)
{
	// Find partition
	const esp_partition_t *p_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, s_partition_label);
	if (p_partition == NULL)
	{
		ESP_LOGE(LOG_TAG, "Failed to find FATFS partition \"%s\". Check the partition table.", s_partition_label);
		return ESP_ERR_NOT_FOUND;
	}

	// Initialise wear levelling
	esp_err_t res = wl_mount(p_partition, &hidf_storage);
	if (res != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to initilise Wear Levelling. Code %d", res);
		return res;
	}

	// Initialise USB MSC driver
	const tinyusb_msc_spiflash_config_t tinyusb_msc_cfg = {
		.wl_handle = hidf_storage,
		.callback_mount_changed = fn_callback_post,
		.callback_premount_changed = fn_callback_pre
	};
	return tinyusb_msc_storage_init_spiflash(&tinyusb_msc_cfg);
}

#undef LOG_TAG
