# AMFPlus

Welcome to the Apache Mobile Filter Plus - the fastest mobile detection.

Installation is very simple:

1. Install gcc, Apache Webserver 2.0.x/2.2.x/2.4.x or newer and Apache Extensions Tool (APXS, included in `httpd-devel`) (downloadable from http://httpd.apache.org). `libcurl` is also recommended.

2. As root, run the install script: `. ./install.sh`. Note that if you have `apxs` installed in a non-standard directory (i.e. `/opt` as used by Red Hat Software Collections) you will need to manually enter the path.

3. Update your Apache configuration with the path to store the AMF config files and add other parameters as necessary.

4. Restart Apache: `apachectl restart`

That's it!!!

Enjoy AMF+

For more info: http://www.apachemobilefilter.org

Copyrights (c) 2009-2018 apachemobilefilter.org, All Rights Reserved

