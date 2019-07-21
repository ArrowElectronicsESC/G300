/*******************************************************************************
* Copyright Arrow Electronics, Inc., 2019
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
* INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
* LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
* OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*******************************************************************************/

/*******************************************************************************
*  Author: Andrew Reisdorph
*  Date:   2019/07/21
*******************************************************************************/

#ifndef __INCLUDE_AZURE_FUNCTIONS_H
#define __INCLUDE_AZURE_FUNCTIONS_H

#define VERBOSE_CURL     0
#define CONFIG_FILE_NAME "/data/azure_config.json"
#define CURL_BUFFER_SIZE (1024*512)
#define AZURE_URL_TELEMETRY "https://%s/devices/%s/messages/events/?api-version=2016-11-14"
#define AZURE_URL_OPERATION_ID "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-11-01"
#define AZURE_URL_HOST_NAME "https://global.azure-devices-provisioning.net/%s/registrations/%s/operations/%s?api-version=2018-11-01"
#define TRACE() {printf("\nLINE: %d -- FUNCTION: %s\n", __LINE__, __FUNCTION__);}

#define HOST_NAME_RETRIES 5

#define TRUE 1
#define FALSE 0


typedef struct AzureConfig {
    char device_id[64];
    char scope_id[16];
    char primary_key[64];
    char operation_id[64];
    char host_name[64];
} AzureConfig;

int azure_init();
int azure_post_telemetry(char *json_string);
int wait_for_network_connection(int attempts);

#endif // __INCLUDE_AZURE_FUNCTIONS_H