# AMFPlus

Apache Mobile Filter Plus (AMFPlus) is an Apache httpd module that provides
server-side, coarse device and browser signals for existing web applications.
It inspects the request `User-Agent` header and, when enabled, User-Agent Client
Hints (UA-CH), then exposes the result as Apache subprocess environment
variables named `AMF_*`.

AMFPlus is best understood as a compatibility adapter for legacy Apache stacks.
It helps applications that already depend on server-side routing, CGI/PHP
environment variables, `mod_rewrite`, or Apache configuration rules to keep a
simple device-awareness layer without moving that logic into every application.

It is not a replacement for responsive design, progressive enhancement,
feature detection, or client-side capability checks. Modern frontends should
still be built to adapt in the browser. AMFPlus is useful when the server also
needs a practical first-pass classification.

Current release: 2.0.0

## What AMFPlus Does

For every request handled by Apache, AMFPlus can detect broad device classes
and common browser or operating-system information. The module writes those
values into Apache's request environment, so downstream code can read them in a
standard way.

The module can identify whether a request appears to come from a mobile phone,
tablet, touch device, TV-like device, or desktop browser. It can also expose
the detected operating system, OS version, browser family, browser version,
the AMFPlus module version, and the packaged repository version used for regex
matching.

AMFPlus 2.0.0 also supports UA Client Hints. When `AMFClientHints on` is set,
the module emits the `Accept-CH` and `Vary` headers for the Client Hints it can
use, including platform, platform version, model, architecture, and mobile
state.

## Why Use It

AMFPlus is useful when device information must be available before application
code renders a page or before Apache chooses a route. Typical reasons include:

- keeping old mobile/desktop routing rules alive while modernizing an estate;
- passing device flags to PHP, CGI, SSI, or other Apache-backed applications;
- using `mod_rewrite`, `SetEnvIf`, logging, or upstream routing decisions based
  on coarse device categories;
- preserving an existing `AMF_*` integration while adding UA-CH awareness;
- reducing duplicated User-Agent parsing logic across multiple legacy apps;
- offering a server-side fallback where client-side JavaScript is unavailable,
  delayed, or intentionally avoided.

This kind of classification is intentionally coarse. It is good for broad
routing, analytics enrichment, compatibility flags, legacy templates, and
selecting server-side defaults. It should not be used as the only source of
truth for security decisions, billing, authorization, or fine-grained feature
support.

## Exposed Environment Variables

When `AMFActivate on` is configured, AMFPlus sets these environment variables
for downstream handlers:

| Variable | Meaning |
| --- | --- |
| `AMF_ID` | Static identifier for AMFPlus detection. |
| `AMF_DEVICE_IS_MOBILE` | `true` when the request appears to be from a mobile phone. |
| `AMF_DEVICE_IS_TABLET` | `true` when the request appears to be from a tablet. |
| `AMF_DEVICE_IS_TOUCH` | `true` when the device appears to support touch. |
| `AMF_DEVICE_IS_TV` | `true` for TV-like devices, media sticks, and similar clients. |
| `AMF_DEVICE_IS_DESKTOP` | `true` when the request is classified as desktop. |
| `AMF_DEVICE_OS` | Detected operating system name, when available. |
| `AMF_DEVICE_OS_VERSION` | Detected operating system version, when available. |
| `AMF_BROWSER_TYPE` | Detected browser family, when available. |
| `AMF_BROWSER_VERSION` | Detected browser version, when available. |
| `AMF_CH_UA` | Raw `Sec-CH-UA` value, or `nc` when not provided. |
| `AMF_CH_UA_ARCH` | Raw `Sec-CH-UA-Arch` value, or `nc` when not provided. |
| `AMF_CH_UA_MODEL` | Raw `Sec-CH-UA-Model` value, or `nc` when not provided. |
| `AMF_CH_UA_PLATFORM` | Raw `Sec-CH-UA-Platform` value, or `nc` when not provided. |
| `AMF_CH_UA_PLATFORM_VERSION` | Raw `Sec-CH-UA-Platform-Version` value, or `nc` when not provided. |
| `AMF_CH_UA_MOBILE` | Raw `Sec-CH-UA-Mobile` value, or `nc` when not provided. |
| `AMF_VER` | AMFPlus module version. |
| `AMF_REPOSITORY_VERSION` | Version of the packaged regex repository. |

`AMF_FORCE_TO_DESKTOP` may also be set when the full-browser override is used.

## Common Use Cases

AMFPlus can be useful in projects where Apache is still the integration point
between traffic and application code:

- Legacy PHP pages can read `$_SERVER["AMF_DEVICE_IS_MOBILE"]` to choose a
  server-side template or redirect path.
- CGI scripts can consume the `AMF_*` variables without embedding their own
  User-Agent parser.
- Apache rewrite rules can keep old `/mobile` or `/desktop` entry points
  working during a gradual migration.
- Logs and analytics pipelines can receive consistent coarse device labels from
  the edge server.
- Reverse-proxy setups can pass the environment-derived values upstream as
  headers or routing metadata.

## Installation

1. Install `gcc`, Apache httpd 2.0.x, 2.2.x, 2.4.x or newer, and Apache
   Extensions Tool (`apxs`, usually included in `httpd-devel` or an equivalent
   package). `libcurl` is recommended; AMFPlus also builds without it.

2. As root, run the install script:

   ```sh
   . ./install.sh
   ```

   If `apxs` is installed in a non-standard directory, such as `/opt`, the
   installer will ask for its path.

3. Configure Apache with an `AMFHome` directory where AMFPlus can read its
   regex repository files.

4. Copy the files from `repository/` into your configured `AMFHome` directory,
   or enable `AMFDownloadParam on` if you explicitly want AMFPlus to refresh
   them over HTTPS.

5. Restart Apache:

   ```sh
   apachectl restart
   ```

## Example Configuration

```apache
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
repository files are versioned with the AMFPlus release.

## Configuration Directives

| Directive | Purpose |
| --- | --- |
| `AMFHome` | Directory containing AMFPlus repository configuration files. |
| `AMFActivate` | Enables or disables AMFPlus detection. |
| `AMFClientHints` | Emits UA-CH request headers through `Accept-CH` and cache variation through `Vary`. |
| `AMFDownloadParam` | Enables optional repository refresh over HTTPS. Disabled by default. |
| `AMFLog` | Enables startup and configuration logging. |
| `AMFProduction` | Stores detection values in cookies to avoid repeating full detection work on later requests. |
| `AMFFullBrowser` | Enables the full-browser override behavior. |
| `AMFFullBrowserAccessKey` | Query-string key used to force desktop/full-browser behavior. |
| `AMFProxy`, `AMFProxyUsr`, `AMFProxyPwd` | Optional proxy settings for repository downloads. |
| `AMFmobile`, `AMFtablet`, `AMFtouch`, `AMFtv` | Override regex values directly from Apache configuration. |

## Detection Repository

AMFPlus uses regex configuration files for device-class matching. The release
tarball includes a `repository/` directory with versioned defaults:

- `litemobiledetectionPlus.config`
- `litetabletdetectionPlus.config`
- `litetouchdetectionPlus.config`
- `litetvdetectionPlus.config`
- `VERSION`

For repeatable deployments, copy these files into `AMFHome` as part of your
configuration management process. Use automatic downloads only when you
deliberately want the server to refresh these files at startup.

## Compatibility Notes

User-Agent strings are increasingly reduced by browser vendors, and UA-CH
availability depends on browser support, HTTPS, cache policy, and whether the
client sends the requested hints. AMFPlus therefore exposes the best practical
server-side classification it can infer, but some values may be `nc` or only
approximately correct.

Use AMFPlus for coarse compatibility decisions, not as a precise hardware
inventory or capability detector. For layout, interaction, media support, and
feature availability, keep using responsive CSS, progressive enhancement, and
browser-side feature checks.

## More Information

Project website: http://www.apachemobilefilter.org

UA Client Hints reference: https://wicg.github.io/ua-client-hints/

## License

Copyright (C) 2009-2026 Idel Fuschini.

AMFPlus is licensed under the GNU Affero General Public License, version 3 or
later. See `LICENSE` for the full license text.
