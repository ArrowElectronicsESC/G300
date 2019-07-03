#ifndef __INCLUDE_AZURE_FUNCTIONS_H
#define __INCLUDE_AZURE_FUNCTIONS_H

#define VERBOSE_CURL     1
#define CONFIG_FILE_NAME "/data/azure_config.json"
#define CURL_BUFFER_SIZE (1024*256)


#define TRACE() {printf("\nLINE: %d -- FUNCTION: %s\n", __LINE__, __FUNCTION__);}


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

#endif // __INCLUDE_AZURE_FUNCTIONS_H