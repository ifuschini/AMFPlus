//SPDX-License-Identifier: AGPL-3.0-or-later
//Compatibility checks for Apache httpd headers used by AMFPlus
//Copyright (C) 2009-2026  Idel Fuschini

#ifndef AMF_APACHE_COMPAT_H
#define AMF_APACHE_COMPAT_H

/*
 * AMFPlus uses the stable Apache httpd 2.x module API: request_rec,
 * command_rec, ap_hook_header_parser, ap_hook_post_read_request,
 * apr_table_* helpers, and STANDARD20_MODULE_STUFF.
 *
 * Apache 2.0 headers do not expose every version macro consistently across
 * platforms, so keep the checks conditional. When the headers do expose
 * version numbers, fail early for unverified major versions or future minor
 * branches beyond the intended 2.0-2.6 compatibility range.
 */
#if defined(AP_SERVER_MAJORVERSION_NUMBER) && AP_SERVER_MAJORVERSION_NUMBER != 2
#error "AMFPlus supports Apache httpd 2.x module headers only"
#endif

#if defined(AP_SERVER_MAJORVERSION_NUMBER) && \
    defined(AP_SERVER_MINORVERSION_NUMBER) && \
    AP_SERVER_MAJORVERSION_NUMBER == 2 && \
    AP_SERVER_MINORVERSION_NUMBER > 6
#error "AMFPlus is intended for Apache httpd 2.0 through 2.6; review compatibility before building with newer headers"
#endif

#ifndef AP_MODULE_DECLARE_DATA
#define AP_MODULE_DECLARE_DATA
#endif

#endif
