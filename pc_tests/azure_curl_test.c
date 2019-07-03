// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/time.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>  

// #include <stdint.h>
#include <curl/curl.h>
// #include <azureiot/parson.h>
// #include <azureiot/azure_c_shared_utility/sastoken.h>
// #include <azureiot/azure_c_shared_utility/urlencode.h>

#include "azure_functions.h"

int main(int argc, char *argv[]) {
    //CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    //json_object *new_obj;


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
    azure_init();
/*
    char postfields[512] = {0};
    snprintf(postfields, 512, "{\"registrationId\":\"%s\"}", DEVICE_ID);
    char address[512];
    snprintf(address, 512, "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-11-01", SCOPE_ID, DEVICE_ID);
    printf("\nURL: %s\n", address);
    printf("\nDATA: %s\n", postfields);
    make_request("PUT", address, postfields, SCOPE_ID, TRUE);
    printf("RESPONSE:\n\n%s\n\n", _curl_buffer);
*/
    return 0;
}