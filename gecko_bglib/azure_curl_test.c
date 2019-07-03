#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <azureiot/azure_c_shared_utility/sastoken.h>
#include <azureiot/azure_c_shared_utility/urlencode.h>

#define VERBOSE_CURL 1

#define DEVICE_ID "105d3088-c5ee-42c1-9f99-adc83f3202ba"
#define SCOPE_ID "0ne00063ECA"
#define PRIMARY_KEY "VTPzQ7XVpk0WbqjrTYvyK9crMZDNmDyXPnYOEvDCD90="
#define CURL_BUFFER_SIZE (1024*256)
static char _curl_buffer[CURL_BUFFER_SIZE] = {0};
static uint32_t _buffer_offset = 0;

unsigned long long get_utc_ms_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long utcTimeInMilliseconds =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;

    return utcTimeInMilliseconds;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    if (_buffer_offset + nmemb > BUFFER_SIZE) {
        printf("Too much data\n");
        exit(1);
    }

    memcpy((_curl_buffer + _buffer_offset), ptr, nmemb);
    _buffer_offset += nmemb;
}

int get_auth_string(char *auth_buffer, char *scope, char *target, char *key_name) {
    unsigned long long expiration_timestamp = ((get_utc_ms_timestamp() + (7200 * 1000)) / 1000);
    char scope_string[128];
    snprintf(scope_string, 128, "%s%%2f%s%%2f%s", target, SCOPE_ID, DEVICE_ID);

    STRING_HANDLE sas_token;
    sas_token = SASToken_CreateString(PRIMARY_KEY, scope_string, "registration", 1562032924);

    sprintf(auth_buffer, "%s", STRING_c_str(sas_token));

    return 0;
}

int make_request(char *method, char *url, char *data, char *scope, bool dps) {
    CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    char *reason = dps ? "registrations" : "devices";
    char *target = dps ? "registration" : NULL;
    char other_header_buff[256];


    memset(_curl_buffer, 0, CURL_BUFFER_SIZE);
    _buffer_offset = 0;

    curl = curl_easy_init();
    if (!curl) {
        res = CURLE_FAILED_INIT;
        printf("ERROR: Curl Init Failed\n");
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
    if (get_auth_string(auth_string, scope, target, target) != 0) {
        printf("ERROR: auth failed.\n");
        exit(-1);
    }

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "accept: application/json");
    chunk = curl_slist_append(chunk, data ? strlen(data) : 0);
    chunk = curl_slist_append(chunk, auth_string);
    chunk = curl_slist_append(chunk, "Content-Type: application/json");

    if (!dps) {
        snprintf(other_header_buff, 256, "iothub-to: /devices/%s/messages/events", DEVICE_ID);
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

    curl_easy_cleanup(curl);
}

int get_operation_id() {

}

int main(int argc, char *argv[]) {
    CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    json_object *new_obj;


    res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
      printf("curl_global_init failed\n");
      return -1;
    }
#if 0
    curl = curl_easy_init();
    if (!curl) {
        res = CURLE_FAILED_INIT;
        printf("Curl init failed\n");
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    if (res) {
      printf("Failed to set write func\n");
      return -1;
    }

    char address[512];
    snprintf(address, 512, "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-11-01", SCOPE_ID, DEVICE_ID);
    printf("\nURL: %s\n", address);

    res = curl_easy_setopt(curl, CURLOPT_URL, address);
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

    res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

    char postfields[512] = {0};
    snprintf(postfields, 512, "{\"registrationId\":\"%s\"}", DEVICE_ID);
    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    if (res) {
        printf("curl_easy_setopt CURLOPT_POSTFIELDS failed\n");
        return -1;
    }


    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "accept: application/json");
 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long utcTimeInMilliseconds =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;
    unsigned long long expires = ((utcTimeInMilliseconds + (7200 * 1000)) / 1000);
    char sr_temp[1024];
    char sr_tempnl[2048];
    printf("got here\n");
    snprintf(sr_temp, 1024, "%s%%2fregistrations%%2f%s", SCOPE_ID, DEVICE_ID);
    snprintf(sr_tempnl, 2048, "%s\n%llu", sr_temp, expires);

    printf("About to create token\n");
    STRING_HANDLE test;
    test = SASToken_CreateString(PRIMARY_KEY, sr_temp, "registration", 1562032924);
    //test = SASToken_Create(PRIMARY_KEY, sr_temp, "registration", expires);
    printf("token created\n");
    printf("SAS: %s\n", STRING_c_str(test));

    char authstring[1024] = {0};
    snprintf(authstring, 1024, "Authorization: %s", STRING_c_str(test));

    char contentLength[64];
    sprintf(contentLength, "Content-Length: %d", strlen(postfields));
    chunk = curl_slist_append(chunk, contentLength);
    chunk = curl_slist_append(chunk, authstring);
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    if (res) {
        printf("curl_easy_setopt failed\n");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);


    res = curl_easy_perform(curl);
    if (res) {
        printf("curl_easy_perform failed: %d\n", res);
        return -1;
    }

    curl_easy_cleanup(curl);
#endif
    char postfields[512] = {0};
    snprintf(postfields, 512, "{\"registrationId\":\"%s\"}", DEVICE_ID);
    char address[512];
    snprintf(address, 512, "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-11-01", SCOPE_ID, DEVICE_ID);
    printf("\nURL: %s\n", address,);
    make_request("PUT", address, postfields, SCOPE_ID, TRUE);
    printf("RESPONSE:\n\n%s\n\n", _curl_buffer);

    return 0;
}