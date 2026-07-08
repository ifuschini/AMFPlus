//mod_amf.c is module for Apache httpd web server for detect mobile devices
//Copyright (C) 2009-2020,  Idel Fuschini
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Affero General Public License as
//published by the Free Software Foundation, either version 3 of the
//License, or (at your option) any later version.
//
//                     _                     __
// _ __ ___   ___   __| |    __ _ _ __ ___  / _|
//| '_ ` _ \ / _ \ / _` |   / _` | '_ ` _ \| |_
//| | | | | | (_) | (_| |  | (_| | | | | | |  _|
//|_| |_| |_|\___/ \__,_|___\__,_|_| |_| |_|_|
//                     |_____|
//
//Organizations that intend to use this file in it commercially
//or under different licensing terms should contact ApacheMobileFilter
//(http://www.apachemobilefilter.org/contacts/) to learn about licensing options for AMF.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Affero General Public License for more details.
//
//You should have received a copy of the GNU Affero General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "mod_amf.h"
#include <ctype.h>
#include <httpd.h>
#include <http_config.h>
#include <http_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CURL_SUPPORT
#include <curl/curl.h>
#endif

#include "apr_env.h"
#include "apr_pools.h"
#include "apr_strings.h"

#define BUFFER_SIZE 1024
#define MAX_SIZE 10000
#define MAX_ERROR_MSG 0x1000
#define AMF_VERSION "2.0.0"

#define AMF_HOST "raw.githubusercontent.com"
#define ISMOBILE_URL "/ifuschini/AMFPlus/master/repository/litemobiledetectionPlus.config"
#define ISTABLET_URL "/ifuschini/AMFPlus/master/repository/litetabletdetectionPlus.config"
#define ISTOUCH_URL  "/ifuschini/AMFPlus/master/repository/litetouchdetectionPlus.config"
#define ISTV_URL  "/ifuschini/AMFPlus/master/repository/litetvdetectionPlus.config"
#define IS_MOBILE 0
#define IS_TABLET 1
#define IS_TOUCH 2
#define IS_TV 3
#define IS_DESKTOP 4
#define OPERATIVE_SYSTEM 5
#define OPERATIVE_SYSTEM_VERSION 6
#define BROWSER_TYPE 7
#define BROWSER_VERSION 8
#define NUMBER_OF_AMF_PARAMS 9

/* regula expression */
#define REGEX_ANDROID_VERSION "android ([0-9]\\.[0-9](\\.[0-9])?)"
#define REGEX_IOS_VERSION "os ([0-9]([0-9]?)_([0-9]?)([0-9]?)(_?)([0-9]?)([0-9]?))"
#define REGEX_WINDOWS_MOBILE_VERSION "( phone| phone os) ([0-9]\\.[0-9](\\.[0-9])?)"
#define REGEX_SYMBIANOS_VERSION "symbianos/([0-9]\\.[0-9](\\.[0-9])?)"
#define REGEX_OSX_VERSION "os x ([0-9]([0-9]?)_([0-9]?)([0-9]?)(_?)([0-9]?)([0-9]?))"
#define REGEX_BROWSER_TYPE "(firefox|msie|chrome|chromium|safari|edge|seamonkey|opera)\\/(([0-9]?)([0-9]?)([0-9]?)(.?)([0-9]?)([0-9]?)([0-9]?)(.?)([0-9]?)([0-9]?)([0-9]?)(.?)([0-9]?)([0-9]?)([0-9]?)(.?)([0-9]?)([0-9]?)([0-9]?))"
#define REGEX_BROWSER_TYPE2  "(firefox|msie|chrome|chromium|safari|edge|seamonkey|opera)\\/([0-9](.?)([0-9]?)(.?)([0-9]?))"
#define MATCH_REGEX_MAX_MATCHES 144
#define MATCH_REGEX_STRING_MAX_MATCHES 10

int setFullBrowser=0;

#ifdef CURL_SUPPORT
int setDownloadParam=1;
#else
int setDownloadParam=0;
#endif 
int AMFOn=0;
int AMFLog=1;
int AMFProduction=0;

char *isMobileString=NULL, *isTabletString=NULL, *isTouchString=NULL, *isTVString=NULL;
char *ProxyUrl=NULL;
char *ProxyUsr=NULL;
char *ProxyPwd=NULL;
char KeyFullBrowser[MAX_SIZE];
char HomeDir[MAX_SIZE];

struct regexCache
{
    regex_t *items;
    int count;
};

static struct regexCache isMobileRegexCache={NULL,0};
static struct regexCache isTabletRegexCache={NULL,0};
static struct regexCache isTouchRegexCache={NULL,0};
static struct regexCache isTVRegexCache={NULL,0};

static const char *amf_value_or_nc(const char *value)
{
    return value != NULL ? value : "nc";
}

static void amf_table_set(apr_table_t *table, const char *key, const char *value)
{
    apr_table_set(table, key, amf_value_or_nc(value));
}

static char *trim_token(char *value)
{
    char *end;

    while (*value && isspace((unsigned char)*value)) {
        value++;
    }

    end = value + strlen(value);
    while (end > value && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    *end = '\0';
    return value;
}

static void clear_regex_cache(struct regexCache *cache)
{
    int i;

    if (cache == NULL || cache->items == NULL) {
        return;
    }

    for (i = 0; i < cache->count; i++) {
        regfree(&cache->items[i]);
    }
    free(cache->items);
    cache->items = NULL;
    cache->count = 0;
}

static int count_regex_tokens(const char *regexList)
{
    char *copy;
    char *last = NULL;
    char *token;
    int count = 0;

    if (regexList == NULL || *regexList == '\0') {
        return 0;
    }

    copy = strdup(regexList);
    if (copy == NULL) {
        return 0;
    }

    token = apr_strtok(copy, ",", &last);
    while (token != NULL) {
        char *regexText = trim_token(token);

        if (*regexText != '\0') {
            count++;
        }
        token = apr_strtok(NULL, ",", &last);
    }

    free(copy);
    return count;
}

static void compile_regex_cache(struct regexCache *cache, const char *regexList)
{
    char *copy;
    char *last = NULL;
    char *token;
    int capacity;

    clear_regex_cache(cache);
    capacity = count_regex_tokens(regexList);
    if (capacity == 0) {
        return;
    }

    cache->items = calloc((size_t)capacity, sizeof(regex_t));
    if (cache->items == NULL) {
        return;
    }

    copy = strdup(regexList);
    if (copy == NULL) {
        clear_regex_cache(cache);
        return;
    }

    token = apr_strtok(copy, ",", &last);
    while (token != NULL && cache->count < capacity) {
        char *regexText = trim_token(token);

        if (*regexText != '\0' && compile_regex(&cache->items[cache->count], regexText) == 0) {
            cache->count++;
        }
        token = apr_strtok(NULL, ",", &last);
    }

    free(copy);
}

static int match_regex_cache(const struct regexCache *cache, const char *userAgent)
{
    int i;

    if (cache == NULL || userAgent == NULL) {
        return 0;
    }

    for (i = 0; i < cache->count; i++) {
        if (match_regex((regex_t *)&cache->items[i], userAgent) == 0) {
            return 1;
        }
    }
    return 0;
}

static const char *trim_client_hint(apr_pool_t *pool, const char *value)
{
    const char *start;
    const char *end;
    char *normalized;
    char *cursor;

    if (pool == NULL || value == NULL) {
        return NULL;
    }

    start = value;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    if (end - start >= 2 && *start == '"' && *(end - 1) == '"') {
        start++;
        end--;
    }

    if (end <= start) {
        return NULL;
    }

    normalized = apr_pstrndup(pool, start, end - start);
    for (cursor = normalized; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((unsigned char)*cursor);
    }
    return normalized;
}

static int client_hint_equals(const char *value, const char *expected)
{
    while (value != NULL && *value && isspace((unsigned char)*value)) {
        value++;
    }
    if (value != NULL && *value == '"') {
        value++;
    }

    while (value != NULL && *value && *value != '"') {
        if (tolower((unsigned char)*value) != tolower((unsigned char)*expected)) {
            return 0;
        }
        value++;
        expected++;
    }

    return value != NULL && *expected == '\0';
}

static int client_hint_has_value(const char *value)
{
    while (value != NULL && *value && isspace((unsigned char)*value)) {
        value++;
    }
    if (value != NULL && *value == '"') {
        value++;
    }
    return value != NULL && *value != '\0' && *value != '"';
}

static int client_hint_mobile_true(const char *value)
{
    return value != NULL && strcmp(value, "?1") == 0;
}
#pragma mark handler
static int handlerAMF(request_rec* r)
{
    if (AMFOn == 1) {
        const char *params[NUMBER_OF_AMF_PARAMS];
        apr_table_t *e = r->subprocess_env;
        params[IS_MOBILE]="false";
        params[IS_TABLET]="false";
        params[IS_TOUCH]="false";
        params[IS_TV]="false";
        params[IS_DESKTOP]="false";
        params[OPERATIVE_SYSTEM]="nc";
        params[OPERATIVE_SYSTEM_VERSION]="nc";
        params[BROWSER_TYPE]="nc";
        params[BROWSER_VERSION]="nc";
        const char *userAgent = NULL;
        const char *x_operamini_phone_ua = NULL;
        const char *x_operamini_ua = NULL;
        const char *x_user_agent = NULL;
        const char *x_ch_ua = NULL;
        const char *x_ch_ua_arch = NULL;
        const char *x_ch_ua_model = NULL;
        const char *x_ch_ua_platform = NULL;
        const char *x_ch_ua_platform_version = NULL;
        const char *x_ch_ua_mobile = NULL;
        int foundParam = 0;
        if (AMFProduction == 1) {
            char *cookieParam = get_cookie_device_param(r);
            if (strcmp("nc", cookieParam)!=0) {
                int pos=0;
                char *last = NULL;
                char *paramFromCookie;
                paramFromCookie=apr_strtok(cookieParam,",",&last);
                while (paramFromCookie != NULL)
                {
                    if (pos>0 && pos <= NUMBER_OF_AMF_PARAMS) {
                        params[pos - 1]=paramFromCookie;
                    }
                    pos++;
                    paramFromCookie = apr_strtok(NULL, ",", &last);
                }
                foundParam = pos >= NUMBER_OF_AMF_PARAMS + 1;
            }
        }
        if  (foundParam == 0) {
            if (r->headers_in != NULL) {
                userAgent = apr_table_get(r->headers_in, "User-Agent");
                x_user_agent = apr_table_get(r->headers_in, "X-User-Agent");
                x_operamini_phone_ua = apr_table_get(r->headers_in, "X-OperaMini-Phone-Ua");
                x_operamini_ua = apr_table_get(r->headers_in, "X-OperaMini-Ua");
                x_ch_ua = apr_table_get(r->headers_in, "Sec-Ch-UA");
                x_ch_ua_arch = apr_table_get(r->headers_in, "Sec-Ch-UA-Arch");
                x_ch_ua_model = apr_table_get(r->headers_in, "Sec-Ch-UA-Model");
                x_ch_ua_platform = apr_table_get(r->headers_in, "Sec-Ch-UA-Platform");
                x_ch_ua_platform_version = apr_table_get(r->headers_in, "Sec-Ch-UA-Platform-Version");
                x_ch_ua_mobile = apr_table_get(r->headers_in, "Sec-Ch-UA-Mobile");
            }

            if (userAgent != NULL) {
                const char *source_user_agent = userAgent;
                char *user_agent;
                char *cursor;
                int isTablet;
                int isMobile;

                if (x_user_agent != NULL) {
                    source_user_agent = x_user_agent;
                }
                if (x_operamini_ua != NULL) {
                    source_user_agent = x_operamini_ua;
                }
                if (x_operamini_phone_ua != NULL) {
                    source_user_agent = x_operamini_phone_ua;
                }

                user_agent = apr_pstrdup(r->pool, source_user_agent);
                for (cursor = user_agent; *cursor != '\0'; cursor++) {
                    *cursor = (char)tolower((unsigned char)*cursor);
                }

                //ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "ua: %s", string);
                isTablet = checkIsTablet(user_agent, x_ch_ua_model, x_ch_ua_platform, x_ch_ua_mobile);
                isMobile = checkIsMobile(user_agent, x_ch_ua_mobile) == 1 || isTablet == 1;
                if (isMobile == 1)
                {
                    params[IS_MOBILE]="true";
                    if (isTablet == 1)
                    {
                        params[IS_TABLET]="true";
                    }
                    if (checkIsTouch(user_agent)==1) {
                        params[IS_TOUCH]="true";
                    }
                    params[OPERATIVE_SYSTEM] = getOperativeSystem(r->pool, user_agent, x_ch_ua_platform);
                    params[OPERATIVE_SYSTEM_VERSION] = getOperativeSystemVersion(r->pool, user_agent, params[OPERATIVE_SYSTEM], x_ch_ua_platform_version);
                } else {
                    if (checkIsTV(user_agent)==1) {
                        params[IS_TV]="true";
                    } else {
                        params[IS_DESKTOP]="true";
                        params[OPERATIVE_SYSTEM] = getOperativeSystemDesktop(r->pool, user_agent, x_ch_ua_platform);
                        params[OPERATIVE_SYSTEM_VERSION] = getOperativeSystemVersion(r->pool, user_agent, params[OPERATIVE_SYSTEM], x_ch_ua_platform_version);
                    } 
                }
                struct browserTypeVersion browser=getBrowserVersion(r->pool, user_agent);
                
                params[BROWSER_TYPE]=browser.type;
                params[BROWSER_VERSION]=browser.version;
                
                /*if (strcmp("?0", x_ch_ua_mobile) == 0)
                {
                    value_ch_ua_mobile = "false";
                }*/
            }
            if (AMFProduction==1) {
                const char *stringCookie = apr_psprintf(r->pool, "AMFParams=dummy,%s,%s,%s,%s,%s,%s,%s,%s,%s; path=/; HttpOnly; SameSite=Lax",params[IS_MOBILE],params[IS_TABLET],params[IS_TOUCH],params[IS_TV],params[IS_DESKTOP],params[OPERATIVE_SYSTEM],params[OPERATIVE_SYSTEM_VERSION],params[BROWSER_TYPE],params[BROWSER_VERSION]);
                apr_table_add(r->headers_out, "Set-Cookie",stringCookie);
                apr_table_add(r->headers_out, "Set-Cookie","AMFDetect=true; path=/; HttpOnly; SameSite=Lax");
            }
        }
        amf_table_set(e, "AMF_ID", "mod_amf_detection");
        amf_table_set(e, "AMF_DEVICE_IS_MOBILE", params[IS_MOBILE]);
        amf_table_set(e, "AMF_DEVICE_IS_TABLET", params[IS_TABLET]);
        amf_table_set(e, "AMF_DEVICE_IS_TOUCH", params[IS_TOUCH]);
        amf_table_set(e, "AMF_DEVICE_IS_TV", params[IS_TV]);
        amf_table_set(e, "AMF_DEVICE_IS_DESKTOP", params[IS_DESKTOP]);
        amf_table_set(e, "AMF_DEVICE_OS", params[OPERATIVE_SYSTEM] );
        amf_table_set(e, "AMF_DEVICE_OS_VERSION", params[OPERATIVE_SYSTEM_VERSION]);
        amf_table_set(e, "AMF_BROWSER_TYPE", params[BROWSER_TYPE]);
        amf_table_set(e, "AMF_BROWSER_VERSION", params[BROWSER_VERSION]);
        amf_table_set(e, "AMF_CH_UA", x_ch_ua);
        amf_table_set(e, "AMF_CH_UA_ARCH", x_ch_ua_arch);
        amf_table_set(e, "AMF_CH_UA_MODEL", x_ch_ua_model);
        amf_table_set(e, "AMF_CH_UA_PLATFORM", x_ch_ua_platform);
        amf_table_set(e, "AMF_CH_UA_PLATFORM_VERSION", x_ch_ua_platform_version);
        amf_table_set(e, "AMF_CH_UA_MOBILE", x_ch_ua_mobile);
        amf_table_set(e, "AMF_VER", AMF_VERSION);
        if (setFullBrowser==1) {
            if (r->args) {
                if (checkQueryStringIsFull (r->args)==1) {
                    apr_table_add(r->headers_out, "Set-Cookie","amfFull=true; path=/; HttpOnly; SameSite=Lax");
                    apr_table_add(e, "AMF_FORCE_TO_DESKTOP", "true");
                }
            }
            if (get_cookie_param(r)==1) {
                apr_table_add(e, "AMF_FORCE_TO_DESKTOP", "true");
            }
        }
        amf_table_set(e, "AMF_VER", AMF_VERSION);
        apr_table_set(r->headers_out, "AMFplus-Ver", AMF_VERSION);
    }
    return DECLINED;
}
/* Compile the regular expression described by "regex_text" into
 "r". */
#pragma mark function
int compile_regex (regex_t * r, const char * regex_text)
{
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
    if (status != 0) {
        char error_message[MAX_ERROR_MSG];
        regerror (status, r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                regex_text, error_message);
        return 1;
    }
    return 0;
}

/*
 Match the string in "to_match" against the compiled regular
 expression in "r".
 */

int match_regex (regex_t * r, const char * to_match)
{
    /* "P" is a pointer into the string which points to the end of the
     previous match. */
    const char * p = to_match;
    /* "M" contains the matches found. */
    regmatch_t m[MATCH_REGEX_MAX_MATCHES];
    int nomatch;

    if (to_match == NULL) {
        return 1;
    }

    nomatch = regexec (r, p, MATCH_REGEX_MAX_MATCHES, m, 0);
    if (nomatch) {
        //printf ("No more matches.\n");
        return 1;
    } else {
        return 0;
    }
    return 0;
}
char *match_regex_string (apr_pool_t *pool, regex_t * r, const char * to_match, int matchOS)
{
    const char * p = to_match;
    /* "M" contains the matches found. */
    regmatch_t m[MATCH_REGEX_STRING_MAX_MATCHES];
    int nomatch;

    if (pool == NULL || to_match == NULL || matchOS < 0 || matchOS >= MATCH_REGEX_STRING_MAX_MATCHES) {
        return pool != NULL ? apr_pstrdup(pool, "nc") : "nc";
    }

    nomatch = regexec (r, p, MATCH_REGEX_STRING_MAX_MATCHES, m, 0);
    if (nomatch || m[matchOS].rm_so == -1) {
        return apr_pstrdup(pool, "nc");
    }

    return apr_pstrndup(pool, to_match + m[matchOS].rm_so, m[matchOS].rm_eo - m[matchOS].rm_so);
}

#ifdef CURL_SUPPORT
int downloadFile (char *host,char *URI, char fileName[]) {
    int returnValue=0;
    CURL* handle = curl_easy_init();
    CURLcode res;
    char url[BUFFER_SIZE];
    char tmpFile[BUFFER_SIZE];
    FILE* fp;

    if (handle == NULL) {
        return 0;
    }

    if (snprintf(url, sizeof(url), "https://%s%s", host, URI) >= (int)sizeof(url) ||
        snprintf(tmpFile, sizeof(tmpFile), "%s.tmp", fileName) >= (int)sizeof(tmpFile)) {
        curl_easy_cleanup(handle);
        return 0;
    }

    fp = fopen(tmpFile, "w");
    if (fp == NULL) {
        if (AMFLog==1)
            printf("Error to open this file: %s\n",fileName);
        curl_easy_cleanup(handle);
        return 0;
    }

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 60L);

    if (ProxyUrl != NULL) {
        if (AMFLog==1)
            printf("Proxy is setted \n");
        curl_easy_setopt(handle, CURLOPT_PROXY, ProxyUrl);
        if (ProxyUsr != NULL) {
            if (AMFLog==1)
                printf("ProxyUsr is correctly setted \n");
            curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, ProxyUsr);
        } else {
            if (AMFLog==1)
                printf("ProxyUsr is not setted \n");
        }
        if (ProxyPwd != NULL) {
            if (AMFLog==1)
                printf("ProxyPwd is correctly setted \n");
            curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, ProxyPwd);
        } else {
            if (AMFLog==1)
                printf("ProxyPwd is not setted \n");
        }
    } else {
        if (AMFLog==1)
            printf("Proxy is not setted \n");
    }

    res=curl_easy_perform(handle);
    fclose(fp);
    if (res == CURLE_OK && rename(tmpFile, fileName) == 0) {
        returnValue=1;
    } else {
        remove(tmpFile);
    }
    curl_easy_cleanup(handle);
    return returnValue;
}
#endif
int get_cookie_param(request_rec *r)
{
    const char *cookies;
    int returnValue=0;
    
    if ((cookies = apr_table_get(r->headers_in, "Cookie"))) {
        if (compare("amfFull=true",cookies)==1) {
            returnValue=1;
        }
        
    }
    return returnValue;
}


int checkQueryStringIsFull (const char *queryString) {
    int returnValue=0;
    if (compare("desktop=true",queryString)==1) {
        returnValue=1;
    }
    return returnValue;
}
int checkIsMobile(char *userAgent, const char *ch_ua_mobile)
{
    int returnValue=0;
    if (ch_ua_mobile == NULL) {
        returnValue = match_regex_cache(&isMobileRegexCache, userAgent);
    } else {
        if (client_hint_mobile_true(ch_ua_mobile))
            returnValue=1;
    }
    return returnValue;
}
int checkIsTouch (char *userAgent) {
    return match_regex_cache(&isTouchRegexCache, userAgent);
}

int checkIsTablet(char *userAgent, const char *ch_ua_model, const char *ch_ua_platform, const char *ch_ua_mobile)
{
    if (match_regex_cache(&isTabletRegexCache, userAgent) == 1) {
        return 1;
    }

    if (!client_hint_mobile_true(ch_ua_mobile) &&
        client_hint_has_value(ch_ua_model) &&
        client_hint_equals(ch_ua_platform, "android")) {
        return 1;
    }

    return 0;
}
int checkIsTV (char *userAgent) {
    return match_regex_cache(&isTVRegexCache, userAgent);
}
int compare (const char *stringToSearch, const char *userAgentSearch) {
    
    int returnValue=0;

    if (stringToSearch == NULL || userAgentSearch == NULL) {
        return 0;
    }

    if (stringToSearch[0] == '^') {
        const char *prefix = stringToSearch + 1;
        size_t prefix_len = strlen(prefix);

        if (strncmp(userAgentSearch, prefix, prefix_len) == 0) {
            returnValue=1;
        }
    } else if (strstr(userAgentSearch,stringToSearch) != NULL) {
        returnValue=1;
    }
    return returnValue;
}
#pragma mark get mobile OS
struct browserTypeVersion getBrowserVersion(apr_pool_t *pool, char *useragent)
{
    struct browserTypeVersion browser;
    char pch[MAX_SIZE];
    snprintf(pch,sizeof(pch),REGEX_BROWSER_TYPE);
    browser.type=apr_pstrdup(pool, "nc");
    browser.version=apr_pstrdup(pool, "nc");
    regex_t r;
    if (compile_regex(& r, pch)==0) {
        browser.type=match_regex_string(pool, & r, useragent,1);
        browser.version=match_regex_string(pool, & r, useragent,2);
        regfree (& r);
        if (strcmp("nc",browser.type)==0) {
            regex_t r2;
            snprintf(pch,sizeof(pch),REGEX_BROWSER_TYPE2);
            if (compile_regex(& r2, pch)==0) {
                browser.type=match_regex_string(pool, & r2, useragent,1);
                browser.version=match_regex_string(pool, & r2, useragent,2);
                regfree (& r2);
            }
        }
    } 
    return browser;

}
char *getOperativeSystemVersion(apr_pool_t *pool, char *useragent, const char *os, const char *ch_ua_platform_version)
{
    char pch[MAX_SIZE];
    const char *platform_version = trim_client_hint(pool, ch_ua_platform_version);
    int matchOS=-1;

    if (platform_version != NULL && strcmp(platform_version, "0.0.0") != 0) {
        return apr_pstrdup(pool, platform_version);
    }

    if (strcmp("android", os)==0) {
        snprintf(pch,sizeof(pch),REGEX_ANDROID_VERSION);
        matchOS=1;
    } else if (strcmp("ios", os)==0) {
        snprintf(pch,sizeof(pch),REGEX_IOS_VERSION);
        matchOS=1;
    } else if (strcmp("windows phone", os)==0) {
        snprintf(pch,sizeof(pch),REGEX_WINDOWS_MOBILE_VERSION);
        matchOS=2;
    } else if (strcmp("symbian", os)==0) {
        snprintf(pch,sizeof(pch),REGEX_SYMBIANOS_VERSION);
        matchOS=1;
    } else if (strcmp("mac", os)==0) {
        snprintf(pch,sizeof(pch),REGEX_OSX_VERSION);
        matchOS=1;
    } 
    
    if (matchOS!=-1) {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            char *returnMatch=match_regex_string(pool, & r, useragent,matchOS);
            regfree (& r);
            return returnMatch;
        }
    }
    return apr_pstrdup(pool, "nc");
}
char* get_cookie_device_param(request_rec *r)
{
    const char *cookies;
    char pch[MAX_SIZE];
    snprintf(pch,sizeof(pch),"AMFParams=([^;]+)");
    if ((cookies = apr_table_get(r->headers_in, "Cookie"))) {
        regex_t cookie_regex;
        if (compile_regex(& cookie_regex, pch)==0) {
            char *returnMatch=match_regex_string(r->pool, & cookie_regex, cookies,1);
            regfree (& cookie_regex);
            return returnMatch;
        }
    }
    return apr_pstrdup(r->pool, "nc");

}
char *getOperativeSystem(apr_pool_t *pool, char *useragent, const char *ch_ua_platform)
{
    //Android ([0-9]\.[0-9](\.[0-9])?)
    char ostypes[MAX_SIZE]="android,iphone|ipad|ipod,windows phone,symbianos,blackberry,kindle";
    const char *osnames[] = {"android","ios","windows phone","symbian","blackberry","kindle"};
    const char *platform = trim_client_hint(pool, ch_ua_platform);
    char *pch;
    char *last = NULL;
    int osNumber=0;

    if (platform != NULL) {
        if (strcmp(platform, "android") == 0) {
            return apr_pstrdup(pool, "android");
        }
        if (strcmp(platform, "ios") == 0 || strcmp(platform, "ipados") == 0) {
            return apr_pstrdup(pool, "ios");
        }
        if (strcmp(platform, "windows") == 0 || strcmp(platform, "windows phone") == 0) {
            return apr_pstrdup(pool, "windows phone");
        }
    }

    pch = apr_strtok(ostypes,",",&last);
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, useragent)==0) {
                regfree (& r);
                return apr_pstrdup(pool, osnames[osNumber]);
            }
            regfree (& r);
        }
        osNumber++;
        pch = apr_strtok(NULL, ",", &last);
    }
    return apr_pstrdup(pool, "nc");
}
char *getOperativeSystemDesktop(apr_pool_t *pool, char *useragent, const char *ch_ua_platform)
{
    //Android ([0-9]\.[0-9](\.[0-9])?)
    char ostypes[MAX_SIZE]="windows,mac,linux";
    const char *osnames[] = {"windows","mac","linux"};
    const char *platform = trim_client_hint(pool, ch_ua_platform);
    char *pch;
    char *last = NULL;
    int osNumber=0;

    if (platform != NULL) {
        if (strcmp(platform, "windows") == 0) {
            return apr_pstrdup(pool, "windows");
        }
        if (strcmp(platform, "macos") == 0 || strcmp(platform, "mac os") == 0) {
            return apr_pstrdup(pool, "mac");
        }
        if (strcmp(platform, "linux") == 0 || strcmp(platform, "chrome os") == 0) {
            return apr_pstrdup(pool, "linux");
        }
    }

    pch = apr_strtok(ostypes,",",&last);
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, useragent)==0) {
                regfree (& r);
                return apr_pstrdup(pool, osnames[osNumber]);
            }
            regfree (& r);
        }
        osNumber++;
        pch = apr_strtok(NULL, ",", &last);
    }
    return apr_pstrdup(pool, "nc");
}
char* readFile(char *nameFile, char *type) {
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char dummy[MAX_SIZE];
    size_t used = 0;

    dummy[0] = '\0';
    fp = fopen(nameFile, "r");
    if (fp == NULL) {
        if (AMFLog==1)
            printf("I couldn't open %s for reading.\n",nameFile );
        return strdup("");
    }

    while((read = getline(&line, &len, fp)) != -1) {
        size_t copy_len = (size_t)read;

        if (used + copy_len >= sizeof(dummy)) {
            copy_len = sizeof(dummy) - used - 1;
        }
        memcpy(dummy + used, line, copy_len);
        used += copy_len;
        dummy[used] = '\0';

        if (used == sizeof(dummy) - 1) {
            break;
        }
    }
    free(line);
    fclose(fp);

    if (AMFLog==1)
        printf("Configuration for %s device loaded correctly\n",type);
    return strdup(dummy);
}

void loadParameters(int flag) {
    int size=0;
    size=strlen(HomeDir);
    if (size==0) {
        perror("AMFHome is not setted");
        exit(1);
    }
    setDownloadParam=flag;
    if (AMFLog==1)
        printf ("AMFDownloadParam is correctly setted\n");
    char nameFile[MAX_SIZE];
#ifdef CURL_SUPPORT
    int returnCode=0;
#endif
    snprintf(nameFile,sizeof(nameFile),"%s/litemobiledetectionPlus.config",HomeDir);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        returnCode=downloadFile(AMF_HOST,ISMOBILE_URL,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for mobile device detection downloaded and saved correctly\n");
        } else {
            if (AMFLog==1)
                printf("Configuration for mobile device detection downloaded failed, try to take old configuration\n");
        } 
        isMobileString=readFile(nameFile,"mobile");
        compile_regex_cache(&isMobileRegexCache, isMobileString);
    } else {
        isMobileString=readFile(nameFile,"mobile");
        compile_regex_cache(&isMobileRegexCache, isMobileString);
    }
#else
        isMobileString=readFile(nameFile,"mobile");
        compile_regex_cache(&isMobileRegexCache, isMobileString);
#endif
    // Detect tablet devices
    snprintf(nameFile,sizeof(nameFile),"%s/litetabletdetectionPlus.config",HomeDir);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        returnCode=downloadFile(AMF_HOST,ISTABLET_URL,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for tablet device detection downloaded and saved correctly\n");
        } else {
            if (AMFLog==1)
                printf("Configuration for tablet device detection downloaded failed, try to take old configuration\n");
			}
        isTabletString=readFile(nameFile,"tablet");
        compile_regex_cache(&isTabletRegexCache, isTabletString);
    } else {
        isTabletString=readFile(nameFile,"tablet");
        compile_regex_cache(&isTabletRegexCache, isTabletString);
    }
#else
        isTabletString=readFile(nameFile,"tablet");
        compile_regex_cache(&isTabletRegexCache, isTabletString);

#endif
    // Detect touch
    snprintf(nameFile,sizeof(nameFile),"%s/litetouchdetectionPlus.config",HomeDir);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        returnCode=downloadFile(AMF_HOST,ISTOUCH_URL,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for touch  device detection downloaded and saved correctly\n");
        } else {
            if (AMFLog==1)
                printf("Configuration for touch device detection downloaded failed, try to take old configuration\n");
			}
        isTouchString=readFile(nameFile,"touch");
        compile_regex_cache(&isTouchRegexCache, isTouchString);
    } else {
        isTouchString=readFile(nameFile,"touch");
        compile_regex_cache(&isTouchRegexCache, isTouchString);
    } 
#else
        isTouchString=readFile(nameFile,"touch");
        compile_regex_cache(&isTouchRegexCache, isTouchString);
#endif
    // Detect tv
    snprintf(nameFile,sizeof(nameFile),"%s/litetvdetectionPlus.config",HomeDir);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        returnCode=downloadFile(AMF_HOST,ISTV_URL,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for tv device detection downloaded and saved correctly\n");
        } else {
            if (AMFLog==1)
                printf("Configuration for tv device detection downloaded failed, try to take old configuration\n");
				}
        isTVString=readFile(nameFile,"tv");
        compile_regex_cache(&isTVRegexCache, isTVString);
    } else {
        isTVString=readFile(nameFile,"tv");
        compile_regex_cache(&isTVRegexCache, isTVString);
    } 
#else
        isTVString=readFile(nameFile,"tv");
        compile_regex_cache(&isTVRegexCache, isTVString);
#endif
    
}

/* DIRECTIVE HTTPD.CONF */

static const char *set_fullDesktop(cmd_parms *parms, void *dummy, int flag)
{
    int size=0;
    size=strlen(HomeDir);
    if (size==0) {
        perror("AMFHome is not setted");
        exit(1);
    }
    setFullBrowser=flag;
    if (AMFLog==1)
        printf ("AMFFullBrowser is correctly setted\n");
    return NULL;
}
static const char *set_activate(cmd_parms *parms, void *dummy, int flag)
{
    int size=0;
    size=strlen(HomeDir);
    if (size==0) {
        perror("AMFHome is not setted");
        exit(1);
    }
    AMFOn=flag;
    if (AMFOn==1) {
        if (AMFLog==1)
            printf ("AMF is on\n");
    } else {
        if (AMFLog==1)
            printf ("AMF is off\n");
    }
#ifndef CURL_SUPPORT
    loadParameters(0);
#endif
    return NULL;
}
static const char *set_amflog(cmd_parms *parms, void *dummy, int flag)
{
    AMFLog=flag;
    return NULL;
}
static const char *set_amfReadConfigFile(cmd_parms *parms, void *dummy, int flag)
{
    if (AMFLog==1)
        printf ("AMFReadConfigFile is deprecated; configuration files are read automatically\n");
    return NULL;
}
 
static const char *set_amfproduction(cmd_parms *parms, void *dummy, int flag)
{
    AMFProduction=flag;
    if (AMFLog==1 && AMFProduction==1) {
            printf ("AMF is in production mode\n");
    } else if (AMFLog==1) {
            printf ("AMF is in test mode\n");
    }
    return NULL;
}

static const char *set_fullBrowserKey(cmd_parms *cmd, void *dummy, const char *map)
{
    int size=0;
    size=strlen(HomeDir);
    if (size==0) {
        perror("AMFHome is not setted");
        exit(1);
    }
    snprintf(KeyFullBrowser,sizeof(KeyFullBrowser),"%s",map);
    
    if (AMFLog==1)
        printf ("AMFKeyFullBrowser is %s \nFor access the device to fullbrowser set the link: <url>%s=true\n",KeyFullBrowser,KeyFullBrowser);
    return NULL;
}
#ifdef CURL_SUPPORT
static const char *set_proxy(cmd_parms *cmd, void *dummy, const char *map)
{
    ProxyUrl=(char *) map;
	if (AMFLog==1)
		printf("AMFProxy is setted to: %s\n",ProxyUrl);
    return NULL;
}
static const char *set_proxy_usr(cmd_parms *cmd, void *dummy, const char *map)
{
    ProxyUsr=(char *) map;
	if (AMFLog==1)
		printf("AMFProxyUsr is setted to: %s\n",ProxyUsr);
    return NULL;
}
static const char *set_proxy_pwd(cmd_parms *cmd, void *dummy, const char *map)
{
    ProxyPwd=(char *) map;
	if (AMFLog==1)
		printf("AMFProxyPwd is setted to: xxxxxxxx\n");
    return NULL;
}
static const char *set_downloadParam(cmd_parms *parms, void *dummy, int flag)
{
    loadParameters(flag);
    return NULL;
}
#endif

static const char *set_homeDir(cmd_parms *cmd, void *dummy, const char *map)
{
    if (AMFLog==1) {
        printf("*************************************************\n");
        printf("*     APACHE MOBILE FILTER + VERSION %s    *\n",AMF_VERSION);
        printf("*************************************************\n");
        printf ("AMFHome is correctly setted as: %s\n",map);
    }
    snprintf(HomeDir,sizeof(HomeDir),"%s",map);
    // Detect mobile
    return NULL;
}
static const char *set_mobile(cmd_parms *cmd, void *dummy, const char *map)
{
    isMobileString=(char *) map;
    compile_regex_cache(&isMobileRegexCache, isMobileString);
    return NULL;
}
static const char *set_touch(cmd_parms *cmd, void *dummy, const char *map)
{
    isTouchString=(char *) map;
    compile_regex_cache(&isTouchRegexCache, isTouchString);
    return NULL;
}
static const char *set_tablet(cmd_parms *cmd, void *dummy, const char *map)
{
    isTabletString=(char *) map;
    compile_regex_cache(&isTabletRegexCache, isTabletString);
    return NULL;
}
static const char *set_tv(cmd_parms *cmd, void *dummy, const char *map)
{
    isTVString=(char *) map;
    compile_regex_cache(&isTVRegexCache, isTVString);
    return NULL;
}

static int amf_per_dir(request_rec * r)
{
    return handlerAMF(r);
}
static int amf_post_read_request(request_rec * r)
{
    return handlerAMF(r);
}

///////////////////////
static void register_hooks(apr_pool_t *p)
{
    /* make sure we run before mod_rewrite's handler */
    static const char * const aszSucc[] = {"mod_setenvif.c", "mod_rewrite.c", NULL };
    ap_hook_header_parser(amf_per_dir, NULL, aszSucc, APR_HOOK_MIDDLE);
    ap_hook_post_read_request(amf_post_read_request, NULL, aszSucc, APR_HOOK_MIDDLE);
}
#pragma mark commands directive
static const command_rec amf_cmds[] = {
    /*
	 *RSRC_CONF - httpd.conf at top level or in a VirtualHost context. All directives using server config should use this, as other contexts are meaningless for a server config.
    ACCESS_CONF - httpd.conf in a Directory context. This is appropriate to per-dir config directives for a server administrator only, and is often combined (using OR) with RSRC_CONF to allow its use anywhere within httpd.conf.
    OR_LIMIT, OR_OPTIONS, OR_FILEINFO, OR_AUTHCFG, OR_INDEXES - extend to allow use of the directive in .htaccess according to AllowOverride setting.
	 */
    AP_INIT_FLAG("AMFActivate", set_activate, NULL,
                 RSRC_CONF, "Define for activate AMF"),
    AP_INIT_FLAG("AMFLog", set_amflog, NULL,
                 RSRC_CONF, "Define for log for AMF"),
    AP_INIT_FLAG("AMFReadConfigFile", set_amfReadConfigFile, NULL,
                 RSRC_CONF, "Define read configuration file or parameters for AMF"),
    AP_INIT_FLAG("AMFProduction", set_amfproduction, NULL,
                 RSRC_CONF, "Define if is AMF is set for production environment (increase pereformance)"),
    AP_INIT_TAKE1("AMFHome", set_homeDir, NULL,
                  RSRC_CONF, "Define HomeDirectory"),
    AP_INIT_FLAG("AMFFullBrowser", set_fullDesktop, NULL,
                 RSRC_CONF, "Define if you want to manage fullbrowser"),
    AP_INIT_TAKE1("AMFFullBrowserAccessKey", set_fullBrowserKey, NULL,
                  RSRC_CONF, "Define the accesskey for your fullbrowser"),
#ifdef CURL_SUPPORT
    AP_INIT_TAKE1("AMFProxy", set_proxy, NULL,
                  RSRC_CONF, "Define proxy for download repository"),
    AP_INIT_TAKE1("AMFProxyUsr", set_proxy_usr, NULL,
                  RSRC_CONF, "Define proxy username for download repository"),
    AP_INIT_TAKE1("AMFProxyPwd", set_proxy_pwd, NULL,
                  RSRC_CONF, "Define proxy password for download repository"),
    AP_INIT_FLAG("AMFDownloadParam", set_downloadParam, NULL,
                 RSRC_CONF, "Define if you want to download param"),
#endif
    AP_INIT_TAKE1("AMFmobile", set_mobile, NULL,
                  RSRC_CONF, "Define mobile param"),
    AP_INIT_TAKE1("AMFtouch", set_touch, NULL,
                  RSRC_CONF, "Define touch param"),
    AP_INIT_TAKE1("AMFtablet", set_tablet, NULL,
                  RSRC_CONF, "Define tablet  param"),
    AP_INIT_TAKE1("AMFtv", set_tv, NULL,
                  RSRC_CONF, "Define tv  param"),
    {NULL}};




module AP_MODULE_DECLARE_DATA amf_module =
{
    STANDARD20_MODULE_STUFF,
    NULL, /* per-directory config creator */
    NULL,     /* dir config merger */
    NULL,     /* server config creator */
    NULL,     /* server config merger */
    amf_cmds, /* command table */
    register_hooks
};
