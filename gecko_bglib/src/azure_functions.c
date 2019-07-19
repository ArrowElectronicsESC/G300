#include <stdint.h>
#include <unistd.h>
#include <curl/curl.h>
#include <azureiot/azure_c_shared_utility/sastoken.h>
#include <azureiot/parson.h>

#include "azure_functions.h"
#include "main.h"
#include "log.h"

static unsigned long long get_utc_ms_timestamp();
static int extract_json_string(JSON_Object* json_object, const char* field_name, char *destination, int size);
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
static const char* const JSON_NODE_ERROR_CODE = "errorCode";
static const char* const JSON_NODE_OPERATION_ID = "operationId";
static const char* const JSON_NODE_ASSIGNED_HUB = "assignedHub";
static const char* const JSON_NODE_REG_STATUS = "registrationState";


static AzureConfig _azure_config = {0};
#define DATA_BUFFER_SIZE 512
static char _url_buffer[DATA_BUFFER_SIZE];
static char _request_data_buffer[DATA_BUFFER_SIZE];

static char _telemetry_post_url[DATA_BUFFER_SIZE];


int wait_for_network_connection(int attempts) {
    CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    int network_up = 0;
    int quit = 0;

    log_trace("Checking for internet connection...\n");

    curl = curl_easy_init();
    if (curl) {
        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        if (res) {
            log_error("Failed to set curl write func. (%d) %s", res, curl_easy_strerror(res));
            return 1;
        }

        res = curl_easy_setopt(curl, CURLOPT_URL, "www.google.com");
        if (res) {
            log_error("curl_easy_setopt CURLOPT_URL failed. (%d) %s", res, curl_easy_strerror(res));
            return 1;
        }
        for (int attempt_counter = 0; attempt_counter < attempts && !quit; attempt_counter++) {
            res = curl_easy_perform(curl);
            switch (res) {
                case CURLE_COULDNT_CONNECT:
                case CURLE_COULDNT_RESOLVE_HOST:
                case CURLE_COULDNT_RESOLVE_PROXY:
                    log_warn("No internet connection. Retrying [%d/%d]...", attempt_counter, attempts);
                    if (attempt_counter % 2) {
                        set_led_color(LED_YELLOW);
                    } else {
                        set_led_color(LED_GREEN);
                    }
                    sleep(10);
                    break;
                case CURLE_OK:
                    network_up = 1;
                    quit = 1;
                break;
                default:
                    log_error("ERROR: Unknown CURL error: %d -- %s", res, curl_easy_strerror(res));
                    quit = 1;
                break;
            }
        }

        if (network_up) {
            log_trace("Interent connection established.");
        } else {
            log_trace("No internet connection.");
        }
    } else {
        log_error("curl_easy_init failed. (%d) %s", res, curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);

    return !network_up;
}

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

    // Create telemetry post url
    int result = snprintf(_telemetry_post_url, DATA_BUFFER_SIZE,
        AZURE_URL_TELEMETRY, _azure_config.host_name, _azure_config.device_id);
    if (result == -1 || result > DATA_BUFFER_SIZE) {
        log_error("Error creating telemetry post URL");
        return -1;
    }

    return 0;
}

int azure_post_telemetry(char *json_string) {
    int request_status = make_request("POST", _telemetry_post_url, json_string,
        _azure_config.host_name, FALSE);
    
    return request_status || (_json_response == NULL) ? 0 : -1;
}

static int init_azure_config() {
    JSON_Value *root_value = json_parse_file(CONFIG_FILE_NAME);

    if (root_value == NULL) {
        log_error("Could not read azure config file.\n");
        return -1;
    }

    JSON_Object *root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        log_error("Could not create json object.");
        return -1;
    }

    JSON_Value *field = json_object_get_value(root_object, "DEVICE_ID");
    if (field == NULL) {
        log_error("Could not read DEVICE_ID");
        return -1;
    }

    const char* json_item = json_value_get_string(field);
    if (json_item == NULL) {
        log_error("Could not create string from DEVICE_ID");
        return -1;
    }
    strcpy(_azure_config.device_id, json_item);

    field = json_object_get_value(root_object, "SCOPE_ID");
    if (field == NULL) {
        log_error("Could not read SCOPE_ID");
        return -1;
    }

    json_item = json_value_get_string(field);
    if (json_item == NULL) {
        log_error("Could not create string from SCOPE_ID");
        return -1;
    }
    strcpy(_azure_config.scope_id, json_item);

    field = json_object_get_value(root_object, "PRIMARY_KEY");
    if (field == NULL) {
        log_error("Could not read PRIMARY_KEY");
        return -1;
    }
    
    json_item = json_value_get_string(field);
    if (json_item == NULL) {
        log_error("Could not create string from PRIMARY_KEY");
        return -1;
    }
    strcpy(_azure_config.primary_key, json_item);

    json_value_free(root_value);

    return 0;
}

static int fetch_operation_id() {
    snprintf(_url_buffer, DATA_BUFFER_SIZE, AZURE_URL_OPERATION_ID,
        _azure_config.scope_id, _azure_config.device_id);
    snprintf(_request_data_buffer, DATA_BUFFER_SIZE,
        "{\"registrationId\":\"%s\"}", _azure_config.device_id);

    if (make_request("PUT", _url_buffer, _request_data_buffer,
        _azure_config.scope_id, true)) {
        log_error("make_request failed.");
        return -1;
    }

    if (_json_response) {
        JSON_Object *response_object = json_value_get_object(_json_response);

        if (response_object) {
            char error_code[64];
            if (extract_json_string(response_object, JSON_NODE_ERROR_CODE,
                error_code, sizeof(error_code)) == 0) {
                log_fatal("Azure error: %s", error_code);
                return -1;
            }

            if (extract_json_string(response_object, JSON_NODE_OPERATION_ID,
                _azure_config.operation_id,
                sizeof(_azure_config.operation_id))) {
                log_fatal("No Operation ID");
                return -1;
            }

            log_info("OPERATION ID: %s", _azure_config.operation_id);
        } else {
            log_fatal("Could not create valid json response object.");
            return -1;
        }
    } else {
        log_fatal("could not create json_response object");
    }

    return 0;
}

static int fetch_host_name() {
    snprintf(_url_buffer, DATA_BUFFER_SIZE, AZURE_URL_HOST_NAME,
        _azure_config.scope_id, _azure_config.device_id,
        _azure_config.operation_id);
    
    uint8_t retries;
    for (retries = 0; retries < HOST_NAME_RETRIES; retries++) {
        sleep(2);
        if (make_request("GET", _url_buffer, NULL, _azure_config.scope_id,
            TRUE)) {
            log_error("make_request failed.");
            return -1;
        }
        
        JSON_Object *response_object = json_value_get_object(_json_response);
        if (response_object == NULL) {
            continue;
        }

        JSON_Object *reg_status_object = json_object_get_object(response_object,
            JSON_NODE_REG_STATUS);
        if (!reg_status_object) {
            continue;
        }

        if (extract_json_string(reg_status_object, JSON_NODE_ASSIGNED_HUB,
            _azure_config.host_name, sizeof(_azure_config.host_name))) {
            continue;
        }

        break;
    }

    if (retries == HOST_NAME_RETRIES) {
        return -1;
    }

    log_info("HOST NAME: %s", _azure_config.host_name);

    return 0;
}

static void print_azure_config() {
    log_info("AZURE CONFIG");
    log_info("Device ID: %s", _azure_config.device_id);
    log_info("Scope ID: %s", _azure_config.scope_id);
    log_info("Primary Key: %s", _azure_config.primary_key);
}

static unsigned long long get_utc_ms_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long utcTimeInMilliseconds =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;

    return utcTimeInMilliseconds;
}

static int extract_json_string(JSON_Object* json_object, const char* field_name,
    char *destination, int size) {
    JSON_Value* json_field;
    json_field = json_object_get_value(json_object, field_name);
    
    if (json_field) {
        const char* json_item = json_value_get_string(json_field);
        if (json_item == NULL) {
            log_error("Could not create string from %s field", json_field);
            return -1;
        }

        int result = snprintf(destination, size, "%s", json_item);
        if (result == -1 || result > size) {
            log_error("String too long for destination buffer");
            return -1;
        }
    } else {
        log_warn("failure retrieving json object value %s", field_name);
        return -1;
    }

    return 0;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    if (_buffer_offset + nmemb > CURL_BUFFER_SIZE) {
        log_error("Too much data");
        return -1;
    }

    memcpy((_curl_buffer + _buffer_offset), ptr, nmemb);
    _buffer_offset += nmemb;

    return nmemb;
}

static int get_auth_string(char *auth_buffer, char *scope, char *target,
    char *key_name) {
    unsigned long long expiration_timestamp =
        ((get_utc_ms_timestamp() + (7200 * 1000)) / 1000);
    char scope_string[128];
    snprintf(scope_string, 128, "%s%%2f%s%%2f%s", scope, target,
        _azure_config.device_id);

    STRING_HANDLE sas_token;
    sas_token = SASToken_CreateString(_azure_config.primary_key, scope_string,
        key_name, expiration_timestamp);

    sprintf(auth_buffer, "%s", STRING_c_str(sas_token));

    log_trace("AUTH: %s",auth_buffer);

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
        log_error("Curl Init Failed. (%d) %s", res, curl_easy_strerror(res));
        return -1;
    }
    
    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    if (res) {
        log_error("curl_easy_setopt CURLOPT_SSL_VERIFYPEER Failed. (%d) %s",
            res, curl_easy_strerror(res));
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    if (res) {
        log_error("curl_easy_setopt CURLOPT_SSL_VERIFYHOST Failed. (%d) %s",
            res, curl_easy_strerror(res));
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    if (res) {
      log_error("curl_easy_setopt CURLOPT_WRITEFUNCTION Failed. (%d) %s", res,
        curl_easy_strerror(res));
      return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (res) {
        log_error("curl_easy_setopt CURLOPT_URL Failed. (%d) %s", res,
            curl_easy_strerror(res));
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    if (res) {
        log_error("curl_easy_setopt CURLOPT_CUSTOMREQUEST Failed. (%d) %s", res,
            curl_easy_strerror(res));
        return -1;
    }

    if (data) {
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        if (res) {
            log_error("curl_easy_setopt CURLOPT_POSTFIELDS Failed. (%d) %s",
                res, curl_easy_strerror(res));
            return -1;
        }
    }

    char auth_string[256];
    if (get_auth_string(auth_string, scope, reason, target) != 0) {
        log_error("Auth failed.");
        return -1;
    }
    char auth_header[512];
    sprintf(auth_header, "authorization: %s", auth_string);

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "accept: application/json");
    char content_length_header[64];
    snprintf(content_length_header, 64, "Content-Length: %d",
        data ? strlen(data) : 0);
    chunk = curl_slist_append(chunk, content_length_header);
    chunk = curl_slist_append(chunk, auth_header);
    chunk = curl_slist_append(chunk, "Content-Type: application/json");

    if (!dps) {
        snprintf(other_header_buff, 256,
            "iothub-to: /devices/%s/messages/events", _azure_config.device_id);
        chunk = curl_slist_append(chunk, other_header_buff);
    }

    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    if (res) {
        log_error("curl_easy_setopt CURLOPT_HTTPHEADER Failed. (%d) %s", res,
            curl_easy_strerror(res));
        return -1;
    }

#if VERBOSE_CURL
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

    res = curl_easy_perform(curl);
    if (res) {
        log_error("curl_easy_perform Failed. (%d) %s", res,
            curl_easy_strerror(res));
        return -1;
    }

    log_trace("\nRESPONSE:\n%s", _curl_buffer);

    _json_response = json_parse_string(_curl_buffer);

    curl_easy_cleanup(curl);

    return 0;
}
