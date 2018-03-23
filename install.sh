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
    echo "Found apxs/apxs2: $myapxs"
    echo "Please give the path to apxs/apxs2 [$myapxs]:"
    read apxs
    if [ -z $apxs ]; then
        apxs=$myapxs
    fi
    if [ -f $apxs ]; then
            echo "apxs file exists"           
            echo "Check libcurl exist....."
            CURL_VERSION=$(cur-config --version 2> /dev/null)
            if [ $? -eq 0 ]; then
                echo "Libcurl Version is $CURL_VERSION"
                cp mod_amf.tmpl mod_amf.h
                sudo $apxs -c -a -i mod_amf.c -lcurl
            else
                echo "Libcurl not found"
                grep -v "#define CURL_SUPPORT" mod_amf.tmpl > mod_amf.h
                sudo $apxs -c -a -i mod_amf.c
                echo "Remember you mus configure manually in your httpd.conf this parameters downloadable from (https://sourceforge.net/projects/mobilefilter/files/AMFPlusRepository/):"
                echo
                echo "AMFmobile from litemobiledetectionPlus.config file"
                echo
                echo "AMFtouch from litetabletdetectionPlus.config file"
                echo
                echo "AMFtablet from litetouchdetectionPlus.config file"
                echo
                echo "AMFtv from litetvdetectionPlus.config file"
                echo
                echo "If you don't wont to configure it install the libcurl from https://curl.haxx.se/libcurl/ and rerun the amf installation script, and AMF download automatically every time"
            fi
        var=1
        else
            echo "File does not exist $apxs\n\nPlease try again !!!!!!\n"
            var=0
    fi
done
