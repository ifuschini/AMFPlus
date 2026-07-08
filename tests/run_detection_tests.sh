#!/bin/sh
set -eu

APXS="$(command -v apxs || command -v apxs2)"
APR_CONFIG="$(command -v apr-1-config || "$APXS" -q APR_CONFIG)"
APU_CONFIG="$(command -v apu-1-config || "$APXS" -q APU_CONFIG || true)"
HTTPD_INCLUDE="$("$APXS" -q INCLUDEDIR)"
OUT="${TMPDIR:-/tmp}/amf_detection_tests"

APR_CFLAGS="$("$APR_CONFIG" --cflags --includes)"
APR_LIBS="$("$APR_CONFIG" --link-ld)"
APU_CFLAGS=""
APU_LIBS=""

if [ -n "$APU_CONFIG" ]; then
    APU_CFLAGS="$("$APU_CONFIG" --includes)"
    APU_LIBS="$("$APU_CONFIG" --link-ld)"
fi

cc -DAMF_TEST -DAMF_NO_CURL_SUPPORT -Wall -I. -I"$HTTPD_INCLUDE" $APR_CFLAGS $APU_CFLAGS -Wno-unused-function tests/detection_harness.c -o "$OUT" $APR_LIBS $APU_LIBS
"$OUT"
