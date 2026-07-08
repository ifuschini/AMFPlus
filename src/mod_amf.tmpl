//SPDX-License-Identifier: AGPL-3.0-or-later
//mod_amf.h is module for Apache httpd web server for detect mobile devices
//Copyright (C) 2009-2026  Idel Fuschini
//
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Affero General Public License as
//published by the Free Software Foundation, either version 3 of the
//License, or (at your option) any later version.
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

#include <ctype.h>
#include <apr_pools.h>
#include <httpd.h>
#include <http_config.h>
#include <http_log.h>
#include <http_protocol.h>
#include <regex.h>

#include "amf_apache_compat.h"

#ifndef _mod_amf_h
#define _mod_amf_h

#ifndef AMF_NO_CURL_SUPPORT
#define CURL_SUPPORT 1
#endif

struct browserTypeVersion
{
    char *type;
    char *version;
};

int compare(const char *string, const char *userAgent);
int compile_regex(regex_t *r, const char *regex_text);
int checkIsMobile(const char *userAgent, const char *ch_ua_mobile);
int checkIsTablet(const char *userAgent, const char *ch_ua_model, const char *ch_ua_platform, const char *ch_ua_mobile);
int checkIsTouch(const char *userAgent);
int checkIsTV(const char *userAgent);
int checkIsConsole(const char *userAgent);
int checkIsSetTopBox(const char *userAgent);
int checkIsEReader(const char *userAgent);
int checkIsAutomotive(const char *userAgent);
int checkIsWearable(const char *userAgent);
int checkIsBot(const char *userAgent);
int checkQueryStringIsFull(const char *queryString);
int get_cookie_param(request_rec *r);
char *get_cookie_device_param(request_rec *r);
#ifdef CURL_SUPPORT
int downloadFile(char *host, char *URI, char fileName[]);
#endif
void loadParameters(int flag);
char *getOperativeSystem(apr_pool_t *pool, const char *useragent, const char *ch_ua_platform);
char *getOperativeSystemDesktop(apr_pool_t *pool, const char *useragent,const char *ch_ua_platform);
char *getOperativeSystemVersion(apr_pool_t *pool, const char *useragent, const char *os,const char *ch_ua_platform_version);
struct browserTypeVersion getBrowserVersion(apr_pool_t *pool, const char *useragent);
int match_regex(regex_t *r, const char *to_match);
char *match_regex_string(apr_pool_t *pool, regex_t *r, const char *to_match, int matchOS);

#endif
