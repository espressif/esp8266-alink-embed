/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"

#include "cJSON.h"

#include "esp_alink_log.h"
#include "esp_alink.h"

static const char *TAG = "esp_json_parser";

alink_err_t __esp_json_parse(const char *json_str, const char *key, void *value, int value_type)
{
    ALINK_PARAM_CHECK(!json_str);
    ALINK_PARAM_CHECK(!key);
    ALINK_PARAM_CHECK(!value);

    cJSON *pJson = cJSON_Parse(json_str);
    ALINK_ERROR_CHECK(!pJson, ALINK_ERR, "cJSON_Parse");

    cJSON *pSub = cJSON_GetObjectItem(pJson, key);

    if (!pSub) {
        cJSON_Delete(pJson);
        return -EINVAL;
    }

    // ALINK_ERROR_CHECK(!pSub, -EINVAL, "cJSON_GetObjectItem %s", key);

    char *p = NULL;

    switch (value_type) {
        case 1:
            *((int *)value) = pSub->valueint;
            break;

        case 2:
            *((float *)value) = (float)(pSub->valuedouble);
            break;

        case 3:
            *((double *)value) = pSub->valuedouble;
            break;

        default:
            switch (pSub->type) {
                case cJSON_False:
                    *((char *)value) = cJSON_False;
                    break;

                case cJSON_True:
                    *((char *)value) = cJSON_True;
                    break;

                case cJSON_Number:
                    *((char *)value) = pSub->valueint;
                    break;

                case cJSON_String:
                    memcpy(value, pSub->valuestring, strlen(pSub->valuestring) + 1);
                    break;

                case cJSON_Object:
                    p = cJSON_Print(pSub);

                    if (!p) {
                        cJSON_Delete(pJson);
                    }

                    ALINK_ERROR_CHECK(!p, -ENOMEM, "cJSON_Print");
                    memcpy(value, p, strlen(p) + 1);
                    free(p);
                    break;

                default:
                    ALINK_LOGE("does not support this type(%d) of data parsing", pSub->type);
                    break;
            }
    }

    cJSON_Delete(pJson);
    return ALINK_OK;
}

ssize_t esp_json_pack_double(char *json_str, const char *key, double value)
{
    ALINK_PARAM_CHECK(!json_str);
    ALINK_PARAM_CHECK(!key);

    int ret = 0;

    if (*json_str != '{') {
        *json_str = '{';
    } else {
        ret = (strlen(json_str) - 1);
        json_str += ret;
        ALINK_ERROR_CHECK(*(json_str) != '}', -EINVAL, "json_str Not initialized to empty");
        *json_str = ',';
    }

    json_str++;
    ret++;

    ret += sprintf(json_str, "\"%s\": %lf}", key, (double)value);
    return ret;
}

ssize_t __esp_json_pack(char *json_str, const char *key, int value, int value_type)
{
    ALINK_PARAM_CHECK(!json_str);
    ALINK_PARAM_CHECK(!key);

    char identifier = '{';

    if (*key == '[') {
        identifier = '[';
        key = NULL;
    }

    int ret = 0;

    if (*json_str != identifier) {
        *json_str = identifier;
    } else {
        ret = (strlen(json_str) - 1);
        json_str += ret;
        // ALINK_ERROR_CHECK(*(json_str) != '}', -EINVAL, "json_str Not initialized to empty");
        *json_str = ',';
    }

    json_str++;
    ret++;

    if ((!value_type) && (value > 0x3FF00000)) {
        value_type = 3;
    }

    int tmp = 0;

    if (key) {
        tmp = sprintf(json_str, "\"%s\": ", key);
        json_str += tmp;
        ret += tmp;
    }

    switch (value_type) {
        case 1:
            tmp = sprintf(json_str, "%d", (int)value);
            break;

        case 2:
            tmp = sprintf(json_str, "%lf", (double)value);
            break;

        case 3:
            if (*((char *)value) == '{' || *((char *)value) == '[') {
                tmp = sprintf(json_str, "%s", (char *)value);
            } else {
                tmp = sprintf(json_str, "\"%s\"", (char *)value);
            }

            break;

        default:
            ALINK_LOGE("invalid type: %d, value_add: %x", value_type, value);
            ret = ALINK_ERR;
            return ret;
    }

    // printf("ret : %d, strlen: %d, json_str: %s\n", ret, (int)strlen(json_str), json_str - ret + 1);
    *(json_str + tmp) = '}';
    *(json_str + tmp + 1) = '\0';

    if (identifier == '[') {
        *(json_str + tmp) = ']';
    }

    // *(json_str + ret) = 0;
    ret += tmp + 1;
    return ret;
}

double floor(double x)
{
    double y = x;

    if ((*(((int *) &y) + 1) & 0x80000000)  != 0) {
        return (float)((int)x) - 1;
    } else {
        return (float)((int)x);
    }
}

double pow(double f, double q)
{
    double c = 1, i;

    for (i = 1; i <= q; i++) {
        c = f * c;
    }

    return c;
}
