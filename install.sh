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
var=0;
while [ $var -eq 0 ]
do
    myapxs=`which apxs2 || which apxs`
    #echo "Found apxs/apxs2: $myapxs"
    echo "Please confirm the path to apxs/apxs2 [$myapxs]:"
    read apxs
    if [ -z $apxs ]; then
        apxs=$myapxs
    fi
    if [ -f $apxs ]; then
        echo "apxs exists"           
        echo "Checking if libcurl exists....."
        CURL_VERSION=$(curl-config --version 2> /dev/null)
        if [ $? -eq 0 ]; then
            echo "Libcurl version is $CURL_VERSION"
            cp mod_amf.tmpl mod_amf.h
            RUN_SUDO=$(sudo $apxs -c -a -i mod_amf.c -lcurl)
            if [ ${RUN_SUDO} -eq 0 ]; then
                $apxs -c -a -i mod_amf.c -lcurl
            fi
        else
            echo "Libcurl not found"
            grep -v "#define CURL_SUPPORT" mod_amf.tmpl > mod_amf.h
            RUN_SUDO=$(sudo $apxs -c -a -i mod_amf.c)
            if [ ${RUN_SUDO} -eq 0 ]; then
                $apxs -c -a -i mod_amf.c
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
            echo "If you don't want to manually configure it, install libcurl from https://curl.haxx.se/libcurl/ and rerun the AMF installation script. Add AMFDownloadParam on to your httpd.conf and AMF will update automatically every time."
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
