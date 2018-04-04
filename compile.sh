#!/bin/sh
echo "Please set"

#sudo /opt/local/apache2/bin/apxs -c -a -i mod_amf51.c
sudo /opt/local/apache2/bin/apxs -c -a -i mod_amf.c -lcurl 
#sudo /opt/local/apache24/bin/apxs -c -a -i mod_amf.c -lcurl 
#sudo /opt/local/apache2/bin/apxs -i -a -L/usr/local/lib -I/usr/local/include -c mod_amf.c
