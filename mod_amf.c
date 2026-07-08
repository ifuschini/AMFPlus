//SPDX-License-Identifier: AGPL-3.0-or-later
//mod_amf.c is module for Apache httpd web server for detect mobile devices
//Copyright (C) 2009-2026,  Idel Fuschini
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
#define AMF_VERSION "2.0.1"
#define AMF_REPOSITORY_VERSION "2.0.1"
#define AMF_CLIENT_HINTS_HEADERS "Sec-CH-UA, Sec-CH-UA-Mobile, Sec-CH-UA-Platform, Sec-CH-UA-Platform-Version, Sec-CH-UA-Model, Sec-CH-UA-Arch"

#define AMF_HOST "raw.githubusercontent.com"
#define ISMOBILE_URL "/ifuschini/AMFPlus/master/rules/litemobiledetectionPlus.config"
#define ISTABLET_URL "/ifuschini/AMFPlus/master/rules/litetabletdetectionPlus.config"
#define ISTOUCH_URL  "/ifuschini/AMFPlus/master/rules/litetouchdetectionPlus.config"
#define ISTV_URL  "/ifuschini/AMFPlus/master/rules/litetvdetectionPlus.config"
#define ISCONSOLE_URL  "/ifuschini/AMFPlus/master/rules/liteconsoledetectionPlus.config"
#define ISSETTOPBOX_URL  "/ifuschini/AMFPlus/master/rules/litesettopboxdetectionPlus.config"
#define ISEREADER_URL  "/ifuschini/AMFPlus/master/rules/liteereaderdetectionPlus.config"
#define ISAUTOMOTIVE_URL  "/ifuschini/AMFPlus/master/rules/liteautomotivedetectionPlus.config"
#define ISWEARABLE_URL  "/ifuschini/AMFPlus/master/rules/litewearabledetectionPlus.config"
#define ISBOT_URL  "/ifuschini/AMFPlus/master/rules/litebotdetectionPlus.config"
#define IS_MOBILE 0
#define IS_TABLET 1
#define IS_TOUCH 2
#define IS_TV 3
#define IS_CONSOLE 4
#define IS_SET_TOP_BOX 5
#define IS_E_READER 6
#define IS_AUTOMOTIVE 7
#define IS_WEARABLE 8
#define IS_BOT 9
#define IS_DESKTOP 10
#define OPERATIVE_SYSTEM 11
#define OPERATIVE_SYSTEM_VERSION 12
#define BROWSER_TYPE 13
#define BROWSER_VERSION 14
#define NUMBER_OF_AMF_PARAMS 15

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
int setDownloadParam=0;
#else
int setDownloadParam=0;
#endif 
int AMFOn=0;
int AMFLog=1;
int AMFProduction=0;
int AMFClientHints=0;

char *isMobileString=NULL, *isTabletString=NULL, *isTouchString=NULL, *isTVString=NULL;
char *isConsoleString=NULL, *isSetTopBoxString=NULL, *isEReaderString=NULL;
char *isAutomotiveString=NULL, *isWearableString=NULL, *isBotString=NULL;
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
static struct regexCache isConsoleRegexCache={NULL,0};
static struct regexCache isSetTopBoxRegexCache={NULL,0};
static struct regexCache isEReaderRegexCache={NULL,0};
static struct regexCache isAutomotiveRegexCache={NULL,0};
static struct regexCache isWearableRegexCache={NULL,0};
static struct regexCache isBotRegexCache={NULL,0};

static const char *amf_value_or_nc(const char *value)
{
    return value != NULL ? value : "nc";
}

static void amf_table_set(apr_table_t *table, const char *key, const char *value)
{
    apr_table_set(table, key, amf_value_or_nc(value));
}

static void set_client_hint_headers(request_rec *r)
{
    apr_table_mergen(r->headers_out, "Accept-CH", AMF_CLIENT_HINTS_HEADERS);
    apr_table_mergen(r->headers_out, "Vary", AMF_CLIENT_HINTS_HEADERS);
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

static char *amf_pstrdup_nc(apr_pool_t *pool)
{
    return apr_pstrdup(pool, "nc");
}

static int is_version_char(int ch)
{
    return isdigit((unsigned char)ch) || ch == '.' || ch == '_';
}

static char *extract_version_after(apr_pool_t *pool, const char *value, const char *marker)
{
    const char *start;
    const char *end;

    if (pool == NULL || value == NULL || marker == NULL) {
        return pool != NULL ? amf_pstrdup_nc(pool) : "nc";
    }

    start = strstr(value, marker);
    if (start == NULL) {
        return amf_pstrdup_nc(pool);
    }

    start += strlen(marker);
    if (!isdigit((unsigned char)*start)) {
        return amf_pstrdup_nc(pool);
    }

    end = start;
    while (*end != '\0' && is_version_char((unsigned char)*end)) {
        end++;
    }

    if (end == start) {
        return amf_pstrdup_nc(pool);
    }
    return apr_pstrndup(pool, start, end - start);
}

static struct browserTypeVersion make_unknown_browser(apr_pool_t *pool)
{
    struct browserTypeVersion browser;

    browser.type = amf_pstrdup_nc(pool);
    browser.version = amf_pstrdup_nc(pool);
    return browser;
}

static int browser_token_match(apr_pool_t *pool, const char *useragent, const char *token, struct browserTypeVersion *browser)
{
    const char *cursor;
    size_t token_len;

    if (pool == NULL || useragent == NULL || token == NULL || browser == NULL) {
        return 0;
    }

    token_len = strlen(token);
    cursor = useragent;
    while ((cursor = strstr(cursor, token)) != NULL) {
        const char *version = cursor + token_len;
        const char *end;

        if (*version != '/') {
            cursor += token_len;
            continue;
        }

        version++;
        if (!isdigit((unsigned char)*version)) {
            cursor += token_len;
            continue;
        }

        end = version;
        while (*end != '\0' && (isdigit((unsigned char)*end) || *end == '.')) {
            end++;
        }

        browser->type = apr_pstrdup(pool, token);
        browser->version = apr_pstrndup(pool, version, end - version);
        return 1;
    }
    return 0;
}

static int is_android_tablet_fallback_candidate_with_context(const char *userAgent, int isTV, int isEReader, int isAutomotive, int isWearable, int isBot)
{
    if (userAgent == NULL || strstr(userAgent, "android") == NULL) {
        return 0;
    }
    if (strstr(userAgent, "mobile") != NULL || isTV == 1 || isEReader == 1 || isAutomotive == 1 || isWearable == 1 || isBot == 1) {
        return 0;
    }
    return 1;
}

static int checkIsTabletWithContext(const char *userAgent, const char *ch_ua_model, const char *ch_ua_platform, const char *ch_ua_mobile, int isTV, int isEReader, int isAutomotive, int isWearable, int isBot)
{
    if (match_regex_cache(&isTabletRegexCache, userAgent) == 1) {
        return 1;
    }

    if (is_android_tablet_fallback_candidate_with_context(userAgent, isTV, isEReader, isAutomotive, isWearable, isBot) == 1) {
        return 1;
    }

    if (!client_hint_mobile_true(ch_ua_mobile) &&
        client_hint_has_value(ch_ua_model) &&
        client_hint_equals(ch_ua_platform, "android")) {
        return 1;
    }

    return 0;
}
#pragma mark handler
static int handlerAMF(request_rec* r)
{
    if (AMFOn == 1) {
        const char *params[NUMBER_OF_AMF_PARAMS];
        apr_table_t *e = r->subprocess_env;

        if (apr_table_get(r->notes, "amfplus_processed") != NULL) {
            return DECLINED;
        }
        apr_table_setn(r->notes, "amfplus_processed", "1");
        if (AMFClientHints == 1) {
            set_client_hint_headers(r);
        }

        params[IS_MOBILE]="false";
        params[IS_TABLET]="false";
        params[IS_TOUCH]="false";
        params[IS_TV]="false";
        params[IS_CONSOLE]="false";
        params[IS_SET_TOP_BOX]="false";
        params[IS_E_READER]="false";
        params[IS_AUTOMOTIVE]="false";
        params[IS_WEARABLE]="false";
        params[IS_BOT]="false";
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
                int isTV;
                int isConsole;
                int isSetTopBox;
                int isEReader;
                int isAutomotive;
                int isWearable;
                int isBot;

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
                isBot = checkIsBot(user_agent);
                isConsole = checkIsConsole(user_agent);
                isSetTopBox = checkIsSetTopBox(user_agent);
                isEReader = checkIsEReader(user_agent);
                isAutomotive = checkIsAutomotive(user_agent);
                isWearable = checkIsWearable(user_agent);
                isTV = checkIsTV(user_agent) == 1 || isConsole == 1 || isSetTopBox == 1;
                isTablet = 0;
                isMobile = 0;
                if (isBot == 0 && isTV == 0 && isEReader == 0 && isAutomotive == 0 && isWearable == 0) {
                    isTablet = checkIsTabletWithContext(user_agent, x_ch_ua_model, x_ch_ua_platform, x_ch_ua_mobile, isTV, isEReader, isAutomotive, isWearable, isBot);
                    isMobile = checkIsMobile(user_agent, x_ch_ua_mobile) == 1 || isTablet == 1;
                }
                if (isBot == 1) {
                    params[IS_BOT]="true";
                } else if (isTV == 1) {
                    params[IS_TV]="true";
                    if (isConsole == 1) {
                        params[IS_CONSOLE]="true";
                    }
                    if (isSetTopBox == 1) {
                        params[IS_SET_TOP_BOX]="true";
                    }
                } else if (isEReader == 1) {
                    params[IS_E_READER]="true";
                } else if (isAutomotive == 1) {
                    params[IS_AUTOMOTIVE]="true";
                    if (checkIsTouch(user_agent)==1) {
                        params[IS_TOUCH]="true";
                    }
                } else if (isWearable == 1) {
                    params[IS_WEARABLE]="true";
                    if (checkIsTouch(user_agent)==1) {
                        params[IS_TOUCH]="true";
                    }
                } else if (isMobile == 1)
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
                    params[IS_DESKTOP]="true";
                    params[OPERATIVE_SYSTEM] = getOperativeSystemDesktop(r->pool, user_agent, x_ch_ua_platform);
                    params[OPERATIVE_SYSTEM_VERSION] = getOperativeSystemVersion(r->pool, user_agent, params[OPERATIVE_SYSTEM], x_ch_ua_platform_version);
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
                const char *stringCookie = apr_psprintf(r->pool, "AMFParams=dummy,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s; path=/; HttpOnly; SameSite=Lax",params[IS_MOBILE],params[IS_TABLET],params[IS_TOUCH],params[IS_TV],params[IS_CONSOLE],params[IS_SET_TOP_BOX],params[IS_E_READER],params[IS_AUTOMOTIVE],params[IS_WEARABLE],params[IS_BOT],params[IS_DESKTOP],params[OPERATIVE_SYSTEM],params[OPERATIVE_SYSTEM_VERSION],params[BROWSER_TYPE],params[BROWSER_VERSION]);
                apr_table_add(r->headers_out, "Set-Cookie",stringCookie);
                apr_table_add(r->headers_out, "Set-Cookie","AMFDetect=true; path=/; HttpOnly; SameSite=Lax");
            }
        }
        amf_table_set(e, "AMF_ID", "mod_amf_detection");
        amf_table_set(e, "AMF_DEVICE_IS_MOBILE", params[IS_MOBILE]);
        amf_table_set(e, "AMF_DEVICE_IS_TABLET", params[IS_TABLET]);
        amf_table_set(e, "AMF_DEVICE_IS_TOUCH", params[IS_TOUCH]);
        amf_table_set(e, "AMF_DEVICE_IS_TV", params[IS_TV]);
        amf_table_set(e, "AMF_DEVICE_IS_CONSOLE", params[IS_CONSOLE]);
        amf_table_set(e, "AMF_DEVICE_IS_SET_TOP_BOX", params[IS_SET_TOP_BOX]);
        amf_table_set(e, "AMF_DEVICE_IS_E_READER", params[IS_E_READER]);
        amf_table_set(e, "AMF_DEVICE_IS_AUTOMOTIVE", params[IS_AUTOMOTIVE]);
        amf_table_set(e, "AMF_DEVICE_IS_WEARABLE", params[IS_WEARABLE]);
        amf_table_set(e, "AMF_DEVICE_IS_BOT", params[IS_BOT]);
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
        amf_table_set(e, "AMF_REPOSITORY_VERSION", AMF_REPOSITORY_VERSION);
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
        amf_table_set(e, "AMF_REPOSITORY_VERSION", AMF_REPOSITORY_VERSION);
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
    if (to_match == NULL) {
        return 1;
    }

    return regexec(r, to_match, 0, NULL, 0) == 0 ? 0 : 1;
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
static int is_downloaded_config_valid(const char *fileName)
{
    FILE *fp;
    long size;
    int first;

    fp = fopen(fileName, "r");
    if (fp == NULL) {
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }
    size = ftell(fp);
    if (size <= 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }

    do {
        first = fgetc(fp);
    } while (first != EOF && isspace(first));
    fclose(fp);

    return first != EOF && first != '<';
}

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
    if (res == CURLE_OK && is_downloaded_config_valid(tmpFile) && rename(tmpFile, fileName) == 0) {
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
int checkIsMobile(const char *userAgent, const char *ch_ua_mobile)
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
int checkIsTouch (const char *userAgent) {
    return match_regex_cache(&isTouchRegexCache, userAgent);
}

int checkIsConsole(const char *userAgent)
{
    return match_regex_cache(&isConsoleRegexCache, userAgent);
}

int checkIsSetTopBox(const char *userAgent)
{
    return match_regex_cache(&isSetTopBoxRegexCache, userAgent);
}

int checkIsEReader(const char *userAgent)
{
    return match_regex_cache(&isEReaderRegexCache, userAgent);
}

int checkIsAutomotive(const char *userAgent)
{
    return match_regex_cache(&isAutomotiveRegexCache, userAgent);
}

int checkIsWearable(const char *userAgent)
{
    return match_regex_cache(&isWearableRegexCache, userAgent);
}

int checkIsBot(const char *userAgent)
{
    return match_regex_cache(&isBotRegexCache, userAgent);
}

static int is_android_tablet_fallback_candidate(const char *userAgent)
{
    if (userAgent == NULL || strstr(userAgent, "android") == NULL) {
        return 0;
    }
    if (strstr(userAgent, "mobile") != NULL ||
        checkIsTV(userAgent) == 1 ||
        checkIsConsole(userAgent) == 1 ||
        checkIsSetTopBox(userAgent) == 1 ||
        checkIsEReader(userAgent) == 1 ||
        checkIsAutomotive(userAgent) == 1 ||
        checkIsWearable(userAgent) == 1 ||
        checkIsBot(userAgent) == 1) {
        return 0;
    }
    return 1;
}

int checkIsTablet(const char *userAgent, const char *ch_ua_model, const char *ch_ua_platform, const char *ch_ua_mobile)
{
    if (match_regex_cache(&isTabletRegexCache, userAgent) == 1) {
        return 1;
    }

    if (is_android_tablet_fallback_candidate(userAgent) == 1) {
        return 1;
    }

    if (!client_hint_mobile_true(ch_ua_mobile) &&
        client_hint_has_value(ch_ua_model) &&
        client_hint_equals(ch_ua_platform, "android")) {
        return 1;
    }

    return 0;
}
int checkIsTV (const char *userAgent) {
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
struct browserTypeVersion getBrowserVersion(apr_pool_t *pool, const char *useragent)
{
    struct browserTypeVersion browser = make_unknown_browser(pool);
    static const char *browser_tokens[] = {
        "firefox", "msie", "chrome", "chromium", "safari", "edge", "seamonkey", "opera", NULL
    };
    int i;

    for (i = 0; browser_tokens[i] != NULL; i++) {
        if (browser_token_match(pool, useragent, browser_tokens[i], &browser) == 1) {
            break;
        }
    }
    return browser;
}
char *getOperativeSystemVersion(apr_pool_t *pool, const char *useragent, const char *os, const char *ch_ua_platform_version)
{
    const char *platform_version = trim_client_hint(pool, ch_ua_platform_version);

    if (platform_version != NULL && strcmp(platform_version, "0.0.0") != 0) {
        return apr_pstrdup(pool, platform_version);
    }

    if (strcmp("android", os)==0) {
        return extract_version_after(pool, useragent, "android ");
    } else if (strcmp("ios", os)==0) {
        return extract_version_after(pool, useragent, "os ");
    } else if (strcmp("windows phone", os)==0) {
        char *version = extract_version_after(pool, useragent, " phone os ");

        if (strcmp(version, "nc") != 0) {
            return version;
        }
        return extract_version_after(pool, useragent, " phone ");
    } else if (strcmp("symbian", os)==0) {
        return extract_version_after(pool, useragent, "symbianos/");
    } else if (strcmp("mac", os)==0) {
        return extract_version_after(pool, useragent, "os x ");
    } 

    return amf_pstrdup_nc(pool);
}
char* get_cookie_device_param(request_rec *r)
{
    const char *cookies;

    if ((cookies = apr_table_get(r->headers_in, "Cookie"))) {
        const char *start = strstr(cookies, "AMFParams=");

        if (start != NULL) {
            const char *end;

            start += strlen("AMFParams=");
            end = strchr(start, ';');
            if (end == NULL) {
                end = start + strlen(start);
            }
            if (end > start) {
                return apr_pstrndup(r->pool, start, end - start);
            }
        }
    }
    return amf_pstrdup_nc(r->pool);

}
char *getOperativeSystem(apr_pool_t *pool, const char *useragent, const char *ch_ua_platform)
{
    const char *platform = trim_client_hint(pool, ch_ua_platform);

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

    if (strstr(useragent, "android") != NULL) {
        return apr_pstrdup(pool, "android");
    }
    if (strstr(useragent, "iphone") != NULL || strstr(useragent, "ipad") != NULL || strstr(useragent, "ipod") != NULL) {
        return apr_pstrdup(pool, "ios");
    }
    if (strstr(useragent, "windows phone") != NULL) {
        return apr_pstrdup(pool, "windows phone");
    }
    if (strstr(useragent, "symbianos") != NULL) {
        return apr_pstrdup(pool, "symbian");
    }
    if (strstr(useragent, "blackberry") != NULL) {
        return apr_pstrdup(pool, "blackberry");
    }
    if (strstr(useragent, "kindle") != NULL) {
        return apr_pstrdup(pool, "kindle");
    }
    return amf_pstrdup_nc(pool);
}
char *getOperativeSystemDesktop(apr_pool_t *pool, const char *useragent, const char *ch_ua_platform)
{
    const char *platform = trim_client_hint(pool, ch_ua_platform);

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

    if (strstr(useragent, "windows") != NULL) {
        return apr_pstrdup(pool, "windows");
    }
    if (strstr(useragent, "mac") != NULL) {
        return apr_pstrdup(pool, "mac");
    }
    if (strstr(useragent, "linux") != NULL) {
        return apr_pstrdup(pool, "linux");
    }
    return amf_pstrdup_nc(pool);
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

static char *load_detection_config(const char *fileName, const char *downloadUri, const char *type, struct regexCache *cache)
{
    char nameFile[MAX_SIZE];
    char *config;

    snprintf(nameFile,sizeof(nameFile),"%s/%s",HomeDir,fileName);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        int returnCode=downloadFile(AMF_HOST,(char *)downloadUri,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for %s device detection downloaded and saved correctly\n",type);
        } else {
            if (AMFLog==1)
                printf("Configuration for %s device detection downloaded failed, try to take old configuration\n",type);
        }
    }
#endif
    config=readFile(nameFile,(char *)type);
    compile_regex_cache(cache, config);
    return config;
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

    isMobileString=load_detection_config("litemobiledetectionPlus.config", ISMOBILE_URL, "mobile", &isMobileRegexCache);
    isTabletString=load_detection_config("litetabletdetectionPlus.config", ISTABLET_URL, "tablet", &isTabletRegexCache);
    isTouchString=load_detection_config("litetouchdetectionPlus.config", ISTOUCH_URL, "touch", &isTouchRegexCache);
    isTVString=load_detection_config("litetvdetectionPlus.config", ISTV_URL, "tv", &isTVRegexCache);
    isConsoleString=load_detection_config("liteconsoledetectionPlus.config", ISCONSOLE_URL, "console", &isConsoleRegexCache);
    isSetTopBoxString=load_detection_config("litesettopboxdetectionPlus.config", ISSETTOPBOX_URL, "set-top box", &isSetTopBoxRegexCache);
    isEReaderString=load_detection_config("liteereaderdetectionPlus.config", ISEREADER_URL, "e-reader", &isEReaderRegexCache);
    isAutomotiveString=load_detection_config("liteautomotivedetectionPlus.config", ISAUTOMOTIVE_URL, "automotive", &isAutomotiveRegexCache);
    isWearableString=load_detection_config("litewearabledetectionPlus.config", ISWEARABLE_URL, "wearable", &isWearableRegexCache);
    isBotString=load_detection_config("litebotdetectionPlus.config", ISBOT_URL, "bot", &isBotRegexCache);
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

static const char *set_clientHints(cmd_parms *parms, void *dummy, int flag)
{
    AMFClientHints=flag;
    if (AMFLog==1 && AMFClientHints==1) {
            printf ("AMF Client Hints response headers are on\n");
    } else if (AMFLog==1) {
            printf ("AMF Client Hints response headers are off\n");
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
static const char *set_console(cmd_parms *cmd, void *dummy, const char *map)
{
    isConsoleString=(char *) map;
    compile_regex_cache(&isConsoleRegexCache, isConsoleString);
    return NULL;
}
static const char *set_settopbox(cmd_parms *cmd, void *dummy, const char *map)
{
    isSetTopBoxString=(char *) map;
    compile_regex_cache(&isSetTopBoxRegexCache, isSetTopBoxString);
    return NULL;
}
static const char *set_ereader(cmd_parms *cmd, void *dummy, const char *map)
{
    isEReaderString=(char *) map;
    compile_regex_cache(&isEReaderRegexCache, isEReaderString);
    return NULL;
}
static const char *set_automotive(cmd_parms *cmd, void *dummy, const char *map)
{
    isAutomotiveString=(char *) map;
    compile_regex_cache(&isAutomotiveRegexCache, isAutomotiveString);
    return NULL;
}
static const char *set_wearable(cmd_parms *cmd, void *dummy, const char *map)
{
    isWearableString=(char *) map;
    compile_regex_cache(&isWearableRegexCache, isWearableString);
    return NULL;
}
static const char *set_bot(cmd_parms *cmd, void *dummy, const char *map)
{
    isBotString=(char *) map;
    compile_regex_cache(&isBotRegexCache, isBotString);
    return NULL;
}

#ifndef AMF_TEST
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
    AP_INIT_FLAG("AMFClientHints", set_clientHints, NULL,
                 RSRC_CONF, "Emit Accept-CH and Vary response headers for UA Client Hints"),
    AP_INIT_TAKE1("AMFHome", set_homeDir, NULL,
                  RSRC_CONF, "Define HomeDirectory"),
    AP_INIT_FLAG("AMFFullBrowser", set_fullDesktop, NULL,
                 RSRC_CONF, "Define if you want to manage fullbrowser"),
    AP_INIT_TAKE1("AMFFullBrowserAccessKey", set_fullBrowserKey, NULL,
                  RSRC_CONF, "Define the accesskey for your fullbrowser"),
#ifdef CURL_SUPPORT
    AP_INIT_TAKE1("AMFProxy", set_proxy, NULL,
                  RSRC_CONF, "Define proxy for downloading rules"),
    AP_INIT_TAKE1("AMFProxyUsr", set_proxy_usr, NULL,
                  RSRC_CONF, "Define proxy username for downloading rules"),
    AP_INIT_TAKE1("AMFProxyPwd", set_proxy_pwd, NULL,
                  RSRC_CONF, "Define proxy password for downloading rules"),
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
    AP_INIT_TAKE1("AMFconsole", set_console, NULL,
                  RSRC_CONF, "Define console param"),
    AP_INIT_TAKE1("AMFsettopbox", set_settopbox, NULL,
                  RSRC_CONF, "Define set-top box param"),
    AP_INIT_TAKE1("AMFereader", set_ereader, NULL,
                  RSRC_CONF, "Define e-reader param"),
    AP_INIT_TAKE1("AMFautomotive", set_automotive, NULL,
                  RSRC_CONF, "Define automotive param"),
    AP_INIT_TAKE1("AMFwearable", set_wearable, NULL,
                  RSRC_CONF, "Define wearable param"),
    AP_INIT_TAKE1("AMFbot", set_bot, NULL,
                  RSRC_CONF, "Define bot param"),
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
#endif
