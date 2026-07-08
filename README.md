# AMFPlus

Welcome to Apache Mobile Filter Plus, a lightweight Apache httpd compatibility
adapter for legacy User-Agent and User-Agent Client Hints based routing.

AMF+ is intended for existing Apache, CGI, PHP, and mod_rewrite stacks that
need coarse device signals exposed as `AMF_*` environment variables. It is not
a replacement for responsive design, feature detection, or progressive
enhancement in modern frontends.

Compatible with UA-CH specific https://wicg.github.io/ua-client-hints/

Current release: 2.0.0

Installation is very simple:

1. Install gcc, Apache Webserver 2.0.x/2.2.x/2.4.x or newer and Apache Extensions Tool (APXS, included in `httpd-devel`) (downloadable from http://httpd.apache.org). `libcurl` is recommended; AMF+ also builds without it.

2. As root, run the install script: `. ./install.sh`. Note that if you have `apxs` installed in a non-standard directory (i.e. `/opt` as used by Red Hat Software Collections) you will need to manually enter the path.

3. Update your Apache configuration with the path to store the AMF config files and add other parameters if necessary.

4. Copy the files from `repository/` into your `AMFHome` directory, or enable `AMFDownloadParam on` if you want AMF+ to refresh them over HTTPS.

5. Restart Apache: `apachectl restart`

Example configuration:

```
AMFHome /var/lib/amfplus
AMFActivate on
AMFClientHints on
AMFDownloadParam off
```

`AMFClientHints on` emits `Accept-CH` and `Vary` for the UA Client Hints used by
the module. Keep it off if another layer already manages Client Hints or cache
variation.

`AMFDownloadParam` is off by default. Enable it only when you explicitly want
the module to refresh regex files from the configured repository. The packaged
repository files are versioned with the AMF+ release.

That's it!!!

Enjoy AMF+

For more info: http://www.apachemobilefilter.org

Copyrights (c) 2009-2020 apachemobilefilter.org, All Rights Reserved
