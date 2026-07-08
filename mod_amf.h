//mod_amf.h is module for Apache httpd web server for detect mobile devices
//Copyright (C) 2009-2020  Idel Fuschini
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

#ifndef _mod_amf_h
#define _mod_amf_h

#define CURL_SUPPORT 1

struct browserTypeVersion
{
    char *type;
    char *version;
};

int compare(const char *string, const char *userAgent);
int compile_regex(regex_t *r, const char *regex_text);
int checkIsMobile(char *userAgent, const char *ch_ua_mobile);
int checkIsTablet(char *userAgent, const char *ch_ua_tablet);
int checkIsTouch(char *userAgent);
int checkIsTV(char *userAgent);
int checkQueryStringIsFull(const char *queryString);
int get_cookie_param(request_rec *r);
char *get_cookie_device_param(request_rec *r);
int socket_connect(char *host, in_port_t port, int check);
#ifdef CURL_SUPPORT
int downloadFile(char *host, char *URI, char fileName[]);
#endif
//int socket_connect(char *host, in_port_t port);
//char *downloadFile (char *URI, char fileName[]);
void loadParameters(int flag);
char *getOperativeSystem(apr_pool_t *pool, char *useragent, const char *ch_ua_platform);
char *getOperativeSystemDesktop(apr_pool_t *pool, char *useragent,const char *ch_ua_platform);
char *getOperativeSystemVersion(apr_pool_t *pool, char *useragent, const char *os,const char *ch_ua_platform);
struct browserTypeVersion getBrowserVersion(apr_pool_t *pool, char *useragent);
char *substring(const char *str, size_t begin, size_t len);
int match_regex(regex_t *r, const char *to_match);
char *match_regex_string(apr_pool_t *pool, regex_t *r, const char *to_match, int matchOS);

#endif
