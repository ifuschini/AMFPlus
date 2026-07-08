#!/bin/sh
#  Script.sh
#
#
#  Created by Idel Fuschini on 10/02/2014
#  and contributed by Dany Gielow
#  last revision 09/03/2017
#
clear
echo "*********************************************"
echo "***   APACHE MOBILE FILTER + INSTALLER    ***"
echo "*********************************************"
echo

run_apxs() {
    if [ "$(id -u)" -eq 0 ]; then
        "$apxs" "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$apxs" "$@"
    else
        "$apxs" "$@"
    fi
}

var=0;
while [ $var -eq 0 ]
do
    myapxs=`command -v apxs2 || command -v apxs`
    #echo "Found apxs/apxs2: $myapxs"
    echo "Please confirm the path to apxs/apxs2 [$myapxs]:"
    read apxs
    if [ -z "$apxs" ]; then
        apxs=$myapxs
    fi
    if [ -f "$apxs" ]; then
        echo "apxs exists"           
        echo "Checking if libcurl exists....."
        CURL_VERSION=$(curl-config --version 2> /dev/null)
        if [ $? -eq 0 ]; then
            echo "Libcurl version is $CURL_VERSION"
            cp mod_amf.tmpl mod_amf.h
            if ! run_apxs -c -a -i mod_amf.c -lcurl; then
                echo "apxs failed to compile or install mod_amf with libcurl"
                exit 1
            fi
        else
            echo "Libcurl not found"
            cp mod_amf.tmpl mod_amf.h
            if ! run_apxs -c -a -i -Wc,-DAMF_NO_CURL_SUPPORT mod_amf.c; then
                echo "apxs failed to compile or install mod_amf"
                exit 1
            fi
            echo "Remember you must manually update the configuration files (downloadable from https://sourceforge.net/projects/mobilefilter/files/AMFPlusRepository/):"
            echo
            echo "AMFmobile from litemobiledetectionPlus.config file"
            echo
            echo "AMFtouch from litetabletdetectionPlus.config file"
            echo
            echo "AMFtablet from litetouchdetectionPlus.config file"
            echo
            echo "AMFtv from litetvdetectionPlus.config file"
            echo
            echo "If you don't want to manually configure it, install libcurl from https://curl.haxx.se/libcurl/ and rerun the AMF installation script. Add AMFDownloadParam on to your httpd.conf only when you explicitly want AMF to refresh repository files."
            echo
        fi
        var=1
    else
        echo "File does not exist: $apxs"
        echo "Please try again!"
        echo
        var=0
    fi
    echo "Remember to update your httpd.conf with the AMFHome parameter:"
    echo "AMFHome /your/preferred/path/to/amf/config/files"
done
