#include <stdint.h>
#include <unistd.h>
#include <curl/curl.h>
#include <azureiot/azure_c_shared_utility/sastoken.h>
#include <azureiot/parson.h>

#include "azure_functions.h"

static unsigned long long get_utc_ms_timestamp();
static int extract_json_string(JSON_Object* json_object, const char* field_name, char *destination);
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
static int get_auth_string(char *auth_buffer, char *scope, char *target, char *key_name);
static int fetch_operation_id();
static int init_azure_config();
static void print_azure_config();
static int make_request(char *method, char *url, char *data, char *scope, bool dps);
static int fetch_host_name();

static char _curl_buffer[CURL_BUFFER_SIZE] = {0};
static JSON_Value *_json_response = NULL;
static uint32_t _buffer_offset = 0;
static const char* const JSON_NODE_OPERATION_ID = "operationId";
static const char* const JSON_NODE_ASSIGNED_HUB = "assignedHub";
static const char* const JSON_NODE_REG_STATUS = "registrationState";


static AzureConfig _azure_config = {0};
#define DATA_BUFFER_SIZE 512
static char _url_buffer[DATA_BUFFER_SIZE];
static char _request_data_buffer[DATA_BUFFER_SIZE];

int azure_init() {
    if (init_azure_config() != 0) {
        return -1;
    }

    print_azure_config();

    if (fetch_operation_id() != 0) {
        return -1;
    }

    if (fetch_host_name() != 0) {
        return -1;
    }

    return 0;
}

int azure_post_telemetry(char *json_string) {
    snprintf(_url_buffer, DATA_BUFFER_SIZE, "https://%s/devices/%s/messages/events/?api-version=2016-11-14", _azure_config.host_name, _azure_config.device_id);
    make_request("POST", _url_buffer, json_string, _azure_config.host_name, FALSE);
    
    return (_json_response == NULL) ? 0 : -1;
}

static int init_azure_config() {
    JSON_Value *root_value = json_parse_file(CONFIG_FILE_NAME);

    if (root_value == NULL) {
        printf("ERROR: Could not read azure config file.\n");
        return -1;
    }

    JSON_Object *root_object = json_value_get_object(root_value);
    JSON_Value *field = json_object_get_value(root_object, "DEVICE_ID");
    const char* json_item = json_value_get_string(field);
    strcpy(_azure_config.device_id, json_item);

    field = json_object_get_value(root_object, "SCOPE_ID");
    json_item = json_value_get_string(field);
    strcpy(_azure_config.scope_id, json_item);

    field = json_object_get_value(root_object, "PRIMARY_KEY");
    json_item = json_value_get_string(field);
    strcpy(_azure_config.primary_key, json_item);

    json_value_free(root_value);

    return 0;
}

static int fetch_operation_id() {
    snprintf(_url_buffer, DATA_BUFFER_SIZE, "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-11-01", _azure_config.scope_id, _azure_config.device_id);
    snprintf(_request_data_buffer, DATA_BUFFER_SIZE, "{\"registrationId\":\"%s\"}", _azure_config.device_id);

    if (make_request("PUT", _url_buffer, _request_data_buffer, _azure_config.scope_id, true)) {
        printf("ERROR: make_request failed.\n");
        return -1;
    }

    extract_json_string(json_value_get_object(_json_response), JSON_NODE_OPERATION_ID, _azure_config.operation_id);
    printf("\n\nOPERATION ID: %s\n\n", _azure_config.operation_id);

    return 0;
}

static int fetch_host_name() {
    snprintf(_url_buffer, DATA_BUFFER_SIZE, "https://global.azure-devices-provisioning.net/%s/registrations/%s/operations/%s?api-version=2018-11-01", _azure_config.scope_id, _azure_config.device_id, _azure_config.operation_id);
    
    uint8_t retries = 0;
    for (retries = 0; retries < 5; retries++) {
        sleep(2);
        if (make_request("GET", _url_buffer, NULL, _azure_config.scope_id, TRUE)) {
            printf("ERROR: make_request failed.\n");
            return -1;
        }
        
        JSON_Object *reg_status_object = json_object_get_object(json_value_get_object(_json_response), JSON_NODE_REG_STATUS);
        if (!reg_status_object) {
            continue;
        }

        if (extract_json_string(reg_status_object, JSON_NODE_ASSIGNED_HUB, _azure_config.host_name)) {
            continue;
        }

        break;
    }

    printf("\nHOST NAME: %s\n\n", _azure_config.host_name);

    return 0;
}

static void print_azure_config() {
    printf("\n=======AZURE CONFIG========\n");
    printf("Device ID: %s\n", _azure_config.device_id);
    printf("Scope ID: %s\n", _azure_config.scope_id);
    printf("Primary Key: %s\n", _azure_config.primary_key);
    printf("===========================\n");
}

static unsigned long long get_utc_ms_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long utcTimeInMilliseconds =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;

    return utcTimeInMilliseconds;
}

static int extract_json_string(JSON_Object* json_object, const char* field_name, char *destination) {
    JSON_Value* json_field;
    json_field = json_object_get_value(json_object, field_name);
    
    if (json_field) {
        const char* json_item = json_value_get_string(json_field);
        strcpy(destination, json_item);
    } else {
        printf("ERROR: failure retrieving json object value %s", field_name);
        return -1;
    }

    return 0;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    if (_buffer_offset + nmemb > CURL_BUFFER_SIZE) {
        printf("Too much data\n");
        return -1;
    }

    memcpy((_curl_buffer + _buffer_offset), ptr, nmemb);
    _buffer_offset += nmemb;

    return nmemb;
}

static int get_auth_string(char *auth_buffer, char *scope, char *target, char *key_name) {
    unsigned long long expiration_timestamp = ((get_utc_ms_timestamp() + (7200 * 1000)) / 1000);
    char scope_string[128];
    snprintf(scope_string, 128, "%s%%2f%s%%2f%s", scope, target, _azure_config.device_id);

    STRING_HANDLE sas_token;
    sas_token = SASToken_CreateString(_azure_config.primary_key, scope_string, key_name, expiration_timestamp);

    sprintf(auth_buffer, "%s", STRING_c_str(sas_token));

    printf("AUTH: %s\n\n",auth_buffer);

    return 0;
}

int make_request(char *method, char *url, char *data, char *scope, bool dps) {
    CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    char *reason = dps ? "registrations" : "devices";
    char *target = dps ? "registration" : NULL;
    char other_header_buff[256];

    // Reset response variables
    memset(_curl_buffer, 0, CURL_BUFFER_SIZE);
    _buffer_offset = 0;
    if (_json_response) {
        json_value_free(_json_response);
        _json_response = NULL;
    }

    curl = curl_easy_init();
    if (!curl) {
        res = CURLE_FAILED_INIT;
        printf("ERROR: Curl Init Failed\n");
        return -1;
    }
    
    //char *ca_path = "/etc/cacert.pem";
    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    if (res) {
      printf("ERROR: Failed to set curl write func\n");
      return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    if (res) {
      printf("ERROR: Failed to set curl write func\n");
      return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

    if (data) {
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        if (res) {
            printf("curl_easy_setopt CURLOPT_POSTFIELDS failed\n");
            return -1;
        }
    }

    char auth_string[256];
    if (get_auth_string(auth_string, scope, reason, target) != 0) {
        printf("ERROR: auth failed.\n");
        return -1;
    }
    char auth_header[512];
    sprintf(auth_header, "authorization: %s", auth_string);

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "accept: application/json");
    char content_length_header[64];
    snprintf(content_length_header, 64, "Content-Length: %d", data ? strlen(data) :0);
    chunk = curl_slist_append(chunk, content_length_header);
    chunk = curl_slist_append(chunk, auth_header);
    chunk = curl_slist_append(chunk, "Content-Type: application/json");

    if (!dps) {
        snprintf(other_header_buff, 256, "iothub-to: /devices/%s/messages/events", _azure_config.device_id);
        chunk = curl_slist_append(chunk, other_header_buff);
    }

    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

#if VERBOSE_CURL
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

    res = curl_easy_perform(curl);
    if (res) {
        printf("curl_easy_perform failed: %d\n", res);
        return -1;
    }

#if VERBOSE_CURL
    printf("\n\nRESPONSE:\n%s\n\n", _curl_buffer);
#endif

    _json_response = json_parse_string(_curl_buffer);

    curl_easy_cleanup(curl);

    return 0;
}
