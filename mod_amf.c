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
#include <fcntl.h>
#include <httpd.h>
#include <http_config.h>
#include <http_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#ifdef CURL_SUPPORT
#include <curl/curl.h>
#endif

#include "apr_env.h"
#include "apr_pools.h"

#define BUFFER_SIZE 1024
#define MAX_SIZE 10000
#define MAX_ERROR_MSG 0x1000
#define AMF_VERSION "1.5.0.0"

#define AMF_HOST "github.com"
#define ISMOBILE_URL "/ifuschini/AMFPlus/blob/master/repository/litemobiledetectionPlus.config"
#define ISTABLET_URL "/ifuschini/AMFPlus/blob/master/repository/litetabletdetectionPlus.config"
#define ISTOUCH_URL  "/ifuschini/AMFPlus/blob/master/repository/litetouchdetectionPlus.config"
#define ISTV_URL  "/ifuschini/AMFPlus/blob/master/repository/litetvdetectionPlus.config"
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

int setFullBrowser=0;

#ifdef CURL_SUPPORT
int setDownloadParam=1;
#else
int setDownloadParam=0;
#endif 
int first=0;
int AMFOn=0;
int AMFLog=1;
int AMFReadConfigFile=0;
int AMFProduction=0;

char *isMobileString, *isTabletString, *isTouchString, *isTVString;
char *ProxyUrl=NULL;
char *ProxyUsr=NULL;
char *ProxyPwd=NULL;
char *hostName, *checkHostString;
char KeyFullBrowser[MAX_SIZE];
char HomeDir[MAX_SIZE];
#pragma mark handler
static int handlerAMF(request_rec* r)
{
    if (AMFOn == 1) {
        const char *params[NUMBER_OF_AMF_PARAMS];
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
        const char *x_ch_ua_mobile = NULL;
        int foundParam = 0;
        int n=0;
        if (AMFProduction == 1) {
            char *cookieParam;
            cookieParam=get_cookie_device_param(r);
            if (strcmp("nc", cookieParam)!=0) {
                int pos=0;
                char *paramFromCookie;
                paramFromCookie=strtok (cookieParam,",");
                while (paramFromCookie != NULL)
                {
                    if (pos>0) { 
                        params[pos - 1]=paramFromCookie;
                    }
                    pos++;
                    paramFromCookie = strtok (NULL, ",");
                }
                foundParam=1;
            }
        }
        if  (foundParam == 0) {
            if (apr_table_get(r->headers_in, "User-Agent") != NULL) {
                if (r->headers_in != NULL)
                {
                    userAgent = apr_table_get(r->headers_in, "User-Agent");
                    x_operamini_phone_ua = apr_table_get(r->headers_in, "X-OperaMini-Phone-Ua");
                    x_operamini_ua = apr_table_get(r->headers_in, "X-OperaMini-Ua");
                    x_ch_ua = apr_table_get(r->headers_in, "Sec-Ch-UA");
                    x_ch_ua_arch = apr_table_get(r->headers_in, "Sec-Ch-UA-Arch");
                    x_ch_ua_model = apr_table_get(r->headers_in, "Sec-Ch-UA-Model");
                    x_ch_ua_platform = apr_table_get(r->headers_in, "Sec-Ch-UA-Platform");
                    x_ch_ua_mobile = apr_table_get(r->headers_in, "Sec-Ch-UA-Mobile");
                    n++;
                }
                // verify for opera mini browser
                char user_agent[strlen(apr_table_get(r->headers_in,"User-Agent"))];
                strcpy(user_agent, apr_table_get(r->headers_in, "User-Agent"));
               if (x_user_agent) {
                    strcpy(user_agent,x_user_agent);
                }
                if (x_operamini_phone_ua) {
                    strcpy(user_agent,x_operamini_phone_ua);
                }
                int i=0;    
                while( user_agent[i] ) {
                    //store back the lower letter to mystr
                    user_agent[i] = tolower(user_agent[i]);
                    i++;
                }     
                //ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "ua: %s", string);
                if (checkIsMobile(user_agent, x_ch_ua_mobile) == 1)
                {
                    params[IS_MOBILE]="true";
                    if (checkIsTablet(user_agent, x_ch_ua_model) == 1)
                    {
                        params[IS_TABLET]="true";
                    }
                    if (checkIsTouch(user_agent)==1) {
                        params[IS_TOUCH]="true";
                    }
                    params[OPERATIVE_SYSTEM] = getOperativeSystem(user_agent, x_ch_ua_platform);
                    params[OPERATIVE_SYSTEM_VERSION] = getOperativeSystemVersion(user_agent, (char *)params[OPERATIVE_SYSTEM], x_ch_ua_platform);
                } else {
                    if (checkIsTV(user_agent)==1) {
                        params[IS_TV]="true";
                    } else {
                        params[IS_DESKTOP]="true";
                        params[OPERATIVE_SYSTEM] = getOperativeSystemDesktop(user_agent, x_ch_ua_platform);
                        params[OPERATIVE_SYSTEM_VERSION] = getOperativeSystemVersion(user_agent, (char *)params[OPERATIVE_SYSTEM], x_ch_ua_platform);
                    } 
                }
                struct browserTypeVersion browser=getBrowserVersion(user_agent);
                
                params[BROWSER_TYPE]=browser.type;
                params[BROWSER_VERSION]=browser.version;
                
                /*if (strcmp("?0", x_ch_ua_mobile) == 0)
                {
                    value_ch_ua_mobile = "false";
                }*/
            }
            if (AMFProduction==1) {
                char stringCookie[MAX_SIZE];
                sprintf(stringCookie, "AMFParams=dummy,%s,%s,%s,%s,%s,%s,%s,%s,%s; path=/;",params[IS_MOBILE],params[IS_TABLET],params[IS_TOUCH],params[IS_TV],params[IS_DESKTOP],params[OPERATIVE_SYSTEM],params[OPERATIVE_SYSTEM_VERSION],params[BROWSER_TYPE],params[BROWSER_VERSION]);
                apr_table_add(r->headers_out, "Set-Cookie",stringCookie);
                apr_table_add(r->headers_out, "Set-Cookie","AMFDetect=true; path=/;"); 
            }
        }
        apr_table_t *e = r->subprocess_env;
        apr_table_setn(e, "AMF_ID", "mod_amf_detection");
        apr_table_setn(e, "AMF_DEVICE_IS_MOBILE", params[IS_MOBILE]);
        apr_table_setn(e, "AMF_DEVICE_IS_TABLET", params[IS_TABLET]);
        apr_table_setn(e, "AMF_DEVICE_IS_TOUCH", params[IS_TOUCH]);
        apr_table_setn(e, "AMF_DEVICE_IS_TV", params[IS_TV]);
        apr_table_setn(e, "AMF_DEVICE_IS_DESKTOP", params[IS_DESKTOP]);
        apr_table_setn(e, "AMF_DEVICE_OS", params[OPERATIVE_SYSTEM] );
        apr_table_setn(e, "AMF_DEVICE_OS_VERSION", params[OPERATIVE_SYSTEM_VERSION]);
        apr_table_setn(e, "AMF_BROWSER_TYPE", params[BROWSER_TYPE]);
        apr_table_setn(e, "AMF_BROWSER_VERSION", params[BROWSER_VERSION]);
        apr_table_setn(e, "AMF_CH_UA", x_ch_ua);
        apr_table_setn(e, "AMF_CH_UA_ARCH", x_ch_ua_arch);
        apr_table_setn(e, "AMF_CH_UA_MODEL", x_ch_ua_model);
        apr_table_setn(e, "AMF_CH_UA_PLATFORM", x_ch_ua_platform);
        apr_table_setn(e, "AMF_CH_UA_MOBILE", x_ch_ua_mobile);
        apr_table_setn(e, "AMF_VER", AMF_VERSION);
        if (setFullBrowser==1) {
            if (r->args) {
                if (checkQueryStringIsFull (r->args)==1) {
                    apr_table_add(r->headers_out, "Set-Cookie","amfFull=true; path=/;");
                    apr_table_add(e, "AMF_FORCE_TO_DESKTOP", "true");
                }
            }
            if (get_cookie_param(r)==1) {
                apr_table_add(e, "AMF_FORCE_TO_DESKTOP", "true");
            }
        }
        apr_table_setn(e, "AMF_VER", AMF_VERSION);
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
    /* "N_matches" is the maximum number of matches allowed. */
    const int n_matches = 144   ;
    /* "M" contains the matches found. */
    regmatch_t m[n_matches];
    int nomatch = regexec (r, p, n_matches, m, 0);
    if (nomatch) {
        //printf ("No more matches.\n");
        return 1;
    } else {
        return 0;
    }
    return 0;
}
char *match_regex_string (regex_t * r, const char * to_match, int matchOS)
{
    char returnValue[MAX_SIZE];
    const char * p = to_match;
    /* "N_matches" is the maximum number of matches allowed. */
    const int n_matches = 10;
    /* "M" contains the matches found. */
    regmatch_t m[n_matches];
    while (1) {
        int i = 0;
        int nomatch = regexec (r, p, n_matches, m, 0);
        if (nomatch) {
            return "nc";
        }
        for (i = 0; i < n_matches; i++) {
            int start;
            int finish;
            if (m[i].rm_so == -1) {
                break;
            }
            start = m[i].rm_so + (p - to_match);
            finish = m[i].rm_eo + (p - to_match);
            if (i == matchOS) {
                sprintf(returnValue, "%.*s", (finish - start), to_match + start);
                size_t size=strlen(returnValue) + 1;
                return strndup(returnValue, size);
            }
        }
        p += m[0].rm_eo;
    }
    sprintf(returnValue, "nc");
    size_t size=strlen(returnValue) + 1;
    return strndup(returnValue, size);
}

#ifdef CURL_SUPPORT
int downloadFile (char *host,char *URI, char fileName[]) {
  int returnValue=0;
  CURL* handle = curl_easy_init();
  CURLcode res;
  char url[BUFFER_SIZE];
  char tmpFile[BUFFER_SIZE];
  sprintf(url,"https://%s%s",host,URI);
  sprintf(tmpFile,"%s.tmp",fileName);
  curl_easy_setopt( handle, CURLOPT_URL, url ) ;
  FILE* fp = fopen( tmpFile, "w");
  if (fp == NULL) {
			printf("Error to open this file: %s",fileName);
            exit(1);
  }
  
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp) ; 
  curl_easy_setopt(handle, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
  
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
	if (ProxyUsr != NULL) {
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
  
  res=curl_easy_perform( handle );
  fclose(fp);
  if (res == CURLE_OK) {
	   rename(tmpFile, fileName);
	   returnValue=1;
   } else {
	   remove(tmpFile);	
   }
  return returnValue;
 }
 #endif
int socket_connect(char *host, in_port_t port,int check){
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;
    
    if((hp = gethostbyname(host)) == NULL){
        if (check==1) {
            herror("Connection error:");
            printf("for host %s",host);
            exit(1);
        } else {
            return -1;
        }
    } else {
        bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
        if(sock == -1){
            perror("setsockopt");
            exit(1);
        } else {
            if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
                perror("connect");
                exit(1);
            }
        }
    }
    return sock;
} 
int get_cookie_param(request_rec *r)
{
    const char *cookies;
    const char *start_cookie;
    int returnValue=0;
    
    if ((cookies = apr_table_get(r->headers_in, "Cookie"))) {
        char cookiesCompare[strlen(cookies)];
        strcpy(cookiesCompare,cookies);
        if (compare("amfFull=true",cookiesCompare)==1) {
            returnValue=1;
        }
        
    }
    return returnValue;
}


int checkQueryStringIsFull (char *queryString) {
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
        char * pch;
        char * stringApp=NULL;
        char * copy = malloc(strlen(isMobileString) + 1);
        strcpy(copy, isMobileString);
        pch = strtok (copy,",");
        while (pch != NULL)
        {
            regex_t r;
            if (compile_regex(& r, pch)==0) {
                if (match_regex(& r, userAgent)==0) {
                    regfree (& r);
                    returnValue=1;
                    break;
                }
            }
            regfree (& r);
            pch = strtok (NULL, ",");
        }
        copy=NULL;
        pch=NULL;
        free(copy);
        free(pch);
    } else {
        if (strcmp("?1",ch_ua_mobile) == 0)
            returnValue=1;
    }
    return returnValue;
}
int checkIsTouch (char *userAgent) {
    int returnValue=0;
    char * pch;
    char * copy = malloc(strlen(isTouchString) + 1);
    strcpy(copy, isTouchString);
    pch = strtok (copy,",");
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, userAgent)==0) {
                regfree (& r);
 				returnValue=1;
				break;
           }
        }
        regfree (& r);
        pch = strtok (NULL, ",");
    }
	copy=NULL;
	pch=NULL;
	free(copy);
    free(pch); 
    return returnValue;
}

int checkIsTablet(char *userAgent, const char *ch_ua_tablet)
{
    int returnValue=0;
    char * pch;
    char * copy = malloc(strlen(isTabletString) + 1);
    strcpy(copy, isTabletString);
    pch = strtok (copy,",");
    
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, userAgent)==0) {
                regfree (& r);
				returnValue=1;
				break;
            }
        }
        regfree (& r);
        pch = strtok (NULL, ",");
    }
	copy=NULL;
	pch=NULL;
	free(copy);
    free(pch); 
    return returnValue;
}
int checkIsTV (char *userAgent) {
    int returnValue=0;
    char * pch;
    char * copy = malloc(strlen(isTVString) + 1);
    strcpy(copy, isTVString);
    pch = strtok (copy,",");   
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, userAgent)==0) {
                regfree (& r);
				returnValue=1;
				break;
            }
        }
        regfree (& r);
        pch = strtok (NULL, ",");
    }
	copy=NULL;
	pch=NULL;
	free(copy);
    free(pch); 
    return returnValue;
}
int compare (char *stringToSearch, char *userAgentSearch) {
    
    int returnValue=0;
    char *stringCompare;
    char *firstChar=substring(stringToSearch, 0,1);
    char *stringCompareFirst=strstr(firstChar,"^");
    if (stringCompareFirst !=NULL) {
        if (strlen(stringToSearch) - 1 <= strlen(userAgentSearch)) {
            char *stringToSearchSub=substring(stringToSearch, 1,strlen(stringToSearch) - 1);
            char *userAgentCompare=substring(userAgentSearch, 0,strlen(stringToSearchSub));
            char *firstCompareStartWith=strstr(stringToSearchSub,userAgentCompare);
            if (firstCompareStartWith != NULL) {
                returnValue=1;
            }
        }
    } else {
        stringCompare=strstr(userAgentSearch,stringToSearch);
        if(stringCompare != NULL) {
            returnValue=1;
        }
    }
    return returnValue;
}
#pragma mark get mobile OS
struct browserTypeVersion getBrowserVersion(char *useragent)
{
    struct browserTypeVersion browser;
    char pch[MAX_SIZE];
    sprintf(pch,REGEX_BROWSER_TYPE);
    browser.type="nc";
    browser.version="nc";
    regex_t r;
    if (compile_regex(& r, pch)==0) {
        browser.type=match_regex_string(& r, useragent,1);
        browser.version=match_regex_string(& r, useragent,2);
        if (strcmp("nc",browser.type)==0) {
            regex_t r2;
            sprintf(pch,REGEX_BROWSER_TYPE2);
            if (compile_regex(& r2, pch)==0) {
                browser.type=match_regex_string(& r2, useragent,1);
                browser.version=match_regex_string(& r2, useragent,2);         
            }
            regfree (& r);        
        }
    } 
    regfree (& r);
    return browser;

}
char *getOperativeSystemVersion(char *useragent, char *os, const char *ch_ua_platform)
{
    char returnValue[MAX_SIZE];
    sprintf(returnValue, "nc");
    char pch[MAX_SIZE];
    int matchOS=-1;
    if (strcmp("android", os)==0) {
        sprintf(pch,REGEX_ANDROID_VERSION);
        matchOS=1;
    } else if (strcmp("ios", os)==0) {
        sprintf(pch,REGEX_IOS_VERSION);
        matchOS=1;
    } else if (strcmp("windows phone", os)==0) {
        sprintf(pch,REGEX_WINDOWS_MOBILE_VERSION);
        matchOS=2;
    } else if (strcmp("symbian", os)==0) {
        sprintf(pch,REGEX_SYMBIANOS_VERSION);
        matchOS=1;
    } else if (strcmp("mac", os)==0) {
        sprintf(pch,REGEX_OSX_VERSION);
        matchOS=1;
    } 
    
    if (matchOS!=-1) {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            char *returnMatch=match_regex_string(& r, useragent,matchOS);
            strcpy(returnValue,returnMatch);
            size_t size=strlen(returnValue) + 1;
            return strndup(returnValue, size);
        } else {
            sprintf(returnValue, "nc");
        }
        regfree (& r);
    }
    size_t size=strlen(returnValue) + 1;
    return strndup(returnValue, size);
}
char* get_cookie_device_param(request_rec *r)
{
    const char *cookies;
    const char *start_cookie;
    char pch[MAX_SIZE];
    char returnValue[MAX_SIZE];
    sprintf(pch,"AMFParams=([^;]+)");
    if ((cookies = apr_table_get(r->headers_in, "Cookie"))) {
        char cookiesCompare[strlen(cookies)];        
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            char *returnMatch=match_regex_string(& r, cookies,1);
            strcpy(returnValue,returnMatch);
            //sprintf(returnValue, "%s",cookies);
            size_t size=strlen(returnValue) + 1;
            return strndup(returnValue, size);
        } else {
            sprintf(returnValue, "nc");
        }
        regfree (& r);
    }
    size_t size=strlen(returnValue) + 1;
    return strndup(returnValue, size);

}
char *getOperativeSystem(char *useragent, const char *ch_ua_platform)
{
    //Android ([0-9]\.[0-9](\.[0-9])?)
    char returnValue[MAX_SIZE];
    char ostypes[MAX_SIZE]="android,iphone|ipad|ipod,windows phone,symbianos,blackberry,kindle";
    char *pch;
    int osNumber=0;
    pch = strtok (ostypes,",");
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, useragent)==0) {
                if (osNumber==0)
                    sprintf(returnValue, "android");
                else if (osNumber==1)
                    sprintf(returnValue, "ios");
                else if (osNumber==2)
                    sprintf(returnValue, "windows phone");
                else if (osNumber==3)
                    sprintf(returnValue, "symbian");
                else if (osNumber==4)
                    sprintf(returnValue, "blackberry");
                else if (osNumber==5)
                    sprintf(returnValue, "kindle");
                size_t size=strlen(returnValue) + 1;
                return strndup(returnValue, size);
            }
        }
        osNumber++;
        regfree (& r);
        pch = strtok (NULL, ",");
    }
    sprintf(returnValue, "nc");
    size_t size=strlen(returnValue) + 1;
    return strndup(returnValue, size);
}
char *getOperativeSystemDesktop(char *useragent, const char *ch_ua_platform)
{
    //Android ([0-9]\.[0-9](\.[0-9])?)
    char returnValue[MAX_SIZE];
    char ostypes[MAX_SIZE]="windows,mac,linux";
    char *pch;
    int osNumber=0;
    pch = strtok (ostypes,",");
    while (pch != NULL)
    {
        regex_t r;
        if (compile_regex(& r, pch)==0) {
            if (match_regex(& r, useragent)==0) {
                if (osNumber==0)
                    sprintf(returnValue, "windows");
                else if (osNumber==1)
                    sprintf(returnValue, "mac");
                else if (osNumber==2)
                    sprintf(returnValue, "linux");
                size_t size=strlen(returnValue) + 1;
                return strndup(returnValue, size);
            }
        }
        osNumber++;
        regfree (& r);
        pch = strtok (NULL, ",");
    }
    sprintf(returnValue, "nc");
    size_t size=strlen(returnValue) + 1;
    return strndup(returnValue, size);
}
char* readFile(char *nameFile, char *type) {
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char dummy[MAX_SIZE];
    size_t size = strlen(dummy) + 1;
    if (AMFReadConfigFile == 1)
    {
        fp = fopen(nameFile, "r");
        if (fp == NULL) {
            if (AMFLog==1)
                printf("I couldn't open %s for reading.\n",nameFile );
                exit(1);
        } else {
            while((read = getline(&line, &len, fp)) != -1) {
                sprintf(dummy,"%s", line);
            }
        }
        if (AMFLog==1)
            printf("Configuration for %s device loaded correctly\n",type);
        size = strlen(dummy) + 1;
    }
    return strndup(dummy, size);
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
    char *hostname;
    sprintf(nameFile,"%s/litemobiledetectionPlus.config",HomeDir);
    int fd = -1;
	int returnCode=0;
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
    } else {
        isMobileString=readFile(nameFile,"mobile");
    }
#else
        isMobileString=readFile(nameFile,"mobile");
#endif
    // Detect tablet devices
    sprintf(nameFile,"%s/litetabletdetectionPlus.config",HomeDir);
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
    } else {
        isTabletString=readFile(nameFile,"tablet");
    }
#else
        isTabletString=readFile(nameFile,"tablet");

#endif
    // Detect touch
    sprintf(nameFile,"%s/litetouchdetectionPlus.config",HomeDir);
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
    } else {
        isTouchString=readFile(nameFile,"touch");
    } 
#else
        isTouchString=readFile(nameFile,"touch");
#endif
    // Detect tv
    sprintf(nameFile,"%s/litetvdetectionPlus.config",HomeDir);
#ifdef CURL_SUPPORT
    if (setDownloadParam==1) {
        returnCode=downloadFile(AMF_HOST,ISTV_URL,nameFile);
        if (returnCode == 1) {
            if (AMFLog==1)
                printf("Configuration for touch  device detection downloaded and saved correctly\n");
        } else {
            if (AMFLog==1)
                printf("Configuration for touch device detection downloaded failed, try to take old configuration\n");
		}
        isTVString=readFile(nameFile,"tv");
    } else {
        isTVString=readFile(nameFile,"tv");
    } 
#else
        isTVString=readFile(nameFile,"tv");
#endif
    
}


size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}
char* substring(const char* str, size_t begin, size_t len)
{
    if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
        return 0;
    
    return strndup(str + begin, len);
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
    AMFReadConfigFile = flag;
    return NULL;
}
 
static const char *set_amfproduction(cmd_parms *parms, void *dummy, int flag)
{
    AMFProduction=flag;
    if (AMFProduction==1) {
            printf ("AMF is in production mode\n");
    } else {
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
    sprintf(KeyFullBrowser,"%s",map);
    
    if (AMFLog==1)
        printf ("AMFKeyFullBrowser is %s \nFor access the device to fullbrowser set the link: <url>%s=true\n",KeyFullBrowser,KeyFullBrowser);
    return NULL;
}
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

static const char *set_homeDir(cmd_parms *cmd, void *dummy, const char *map)
{
    if (AMFLog==1) {
        printf("*************************************************\n");
        printf("*     APACHE MOBILE FILTER + VERSION %s    *\n",AMF_VERSION);
        printf("*************************************************\n");
        printf ("AMFHome is correctly setted as: %s\n",map);
    }
    sprintf(HomeDir,"%s",map);
    // Detect mobile
    return NULL;
}
static const char *set_mobile(cmd_parms *cmd, void *dummy, const char *map)
{
    isMobileString=(char *) map;
    return NULL;
}
static const char *set_touch(cmd_parms *cmd, void *dummy, const char *map)
{
    isTouchString=(char *) map;
    return NULL;
}
static const char *set_tablet(cmd_parms *cmd, void *dummy, const char *map)
{
    isTabletString=(char *) map;
    return NULL;
}
static const char *set_tv(cmd_parms *cmd, void *dummy, const char *map)
{
    isTVString=(char *) map;
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
static void amf_child_init(apr_pool_t * p, server_rec * s)
{
    
}

static int amf_post_config(apr_pool_t * p, apr_pool_t * plog,
                           apr_pool_t * ptemp, server_rec * s)
{
    return OK;
}
/*static void amf_server_init(apr_pool_t * p, server_rec * s)
 {
 
 } */

///////////////////////
static void register_hooks(apr_pool_t *p)
{
    /* make sure we run before mod_rewrite's handler */
    static const char * const aszSucc[] = {"mod_setenvif.c", "mod_rewrite.c", NULL };
    ap_hook_header_parser(amf_per_dir, NULL, aszSucc, APR_HOOK_MIDDLE);
    ap_hook_post_read_request(amf_per_dir, NULL, aszSucc, APR_HOOK_MIDDLE);
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