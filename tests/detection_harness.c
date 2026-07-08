#define AMF_TEST 1
#define AMF_NO_CURL_SUPPORT 1

#include "../mod_amf.c"

static int failures = 0;

static void assert_true(const char *name, int value)
{
    if (!value) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

static void assert_string(const char *name, const char *actual, const char *expected)
{
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "FAIL: %s expected=%s actual=%s\n", name, expected, actual);
        failures++;
    }
}

int main(void)
{
    apr_pool_t *pool;

    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "FAIL: apr_initialize\n");
        return 1;
    }
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        fprintf(stderr, "FAIL: apr_pool_create\n");
        apr_terminate();
        return 1;
    }

    set_mobile(NULL, NULL, "android,iphone|ipod,mobile");
    set_tablet(NULL, NULL, "ipad,tablet");
    set_touch(NULL, NULL, "android,iphone,ipad,touch");
    set_tv(NULL, NULL, "smarttv,appletv");

    assert_true("android user-agent is mobile", checkIsMobile("mozilla/5.0 android", NULL) == 1);
    assert_true("sec-ch-ua-mobile true is mobile", checkIsMobile("mozilla/5.0", "?1") == 1);
    assert_true("ipad user-agent is tablet", checkIsTablet("mozilla/5.0 ipad", NULL, NULL, NULL) == 1);
    assert_true("android tablet client hints are tablet", checkIsTablet("mozilla/5.0", "\"Pixel Tablet\"", "\"Android\"", "?0") == 1);
    assert_true("touch regex matches android", checkIsTouch("mozilla/5.0 android") == 1);
    assert_true("tv regex matches smarttv", checkIsTV("mozilla/5.0 smarttv") == 1);

    assert_string("mobile os from client hint", getOperativeSystem(pool, "mozilla/5.0", "\"Android\""), "android");
    assert_string("desktop os from client hint", getOperativeSystemDesktop(pool, "mozilla/5.0", "\"macOS\""), "mac");
    assert_string("platform version from client hint", getOperativeSystemVersion(pool, "mozilla/5.0", "android", "\"14.0.0\""), "14.0.0");
    assert_string("ios os from user-agent", getOperativeSystem(pool, "mozilla/5.0 iphone os 17_1", NULL), "ios");

    apr_pool_destroy(pool);
    apr_terminate();

    return failures == 0 ? 0 : 1;
}
