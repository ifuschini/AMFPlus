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

static void load_detection_rules(void)
{
    AMFLog = 0;
    set_mobile(NULL, NULL, readFile("rules/litemobiledetectionPlus.config", "mobile"));
    set_tablet(NULL, NULL, readFile("rules/litetabletdetectionPlus.config", "tablet"));
    set_touch(NULL, NULL, readFile("rules/litetouchdetectionPlus.config", "touch"));
    set_tv(NULL, NULL, readFile("rules/litetvdetectionPlus.config", "tv"));
    set_console(NULL, NULL, readFile("rules/liteconsoledetectionPlus.config", "console"));
    set_settopbox(NULL, NULL, readFile("rules/litesettopboxdetectionPlus.config", "set-top box"));
    set_ereader(NULL, NULL, readFile("rules/liteereaderdetectionPlus.config", "e-reader"));
    set_automotive(NULL, NULL, readFile("rules/liteautomotivedetectionPlus.config", "automotive"));
    set_wearable(NULL, NULL, readFile("rules/litewearabledetectionPlus.config", "wearable"));
    set_bot(NULL, NULL, readFile("rules/litebotdetectionPlus.config", "bot"));
}

static void lowercase(char *value)
{
    char *cursor;

    for (cursor = value; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((unsigned char)*cursor);
    }
}

static int contains(const char *value, const char *needle)
{
    return value != NULL && strstr(value, needle) != NULL;
}

static int is_fixture_user_agent_line(const char *line, int section)
{
    if (section < 1 || section > 31) {
        return 0;
    }
    if (line[0] == '\0' || line[0] == '#' || line[0] == '=' || strncmp(line, "SECTION ", 8) == 0) {
        return 0;
    }
    return section < 32;
}

static int fixture_expects_bot(int section)
{
    return section == 31;
}

static int fixture_expects_console(int section)
{
    return section >= 25 && section <= 28;
}

static int fixture_expects_set_top_box(int section, const char *ua)
{
    if (section == 22) {
        return !contains(ua, "bravia");
    }
    return section == 23 || section == 24;
}

static int fixture_expects_e_reader(int section, const char *ua)
{
    return section == 16 && (contains(ua, "kindle/") || contains(ua, "x11"));
}

static int fixture_expects_automotive(int section, const char *ua)
{
    return section == 29 && contains(ua, "automotive");
}

static int fixture_expects_wearable(int section, const char *ua)
{
    return section == 29 && (contains(ua, "wear os") || contains(ua, "watchos") || contains(ua, "watch") || contains(ua, "sm-r"));
}

static int fixture_expects_tablet(int section, const char *ua)
{
    if (section == 4 || section == 9 || section == 16) {
        return 1;
    }
    if (section == 10) {
        return contains(ua, "matepad") || contains(ua, "mediapad") || contains(ua, "bah4");
    }
    if (section == 11 || section == 12 || section == 13) {
        return contains(ua, " pad") || contains(ua, "tablet");
    }
    if (section == 14) {
        return contains(ua, "lenovo tb") || contains(ua, " tb");
    }
    if (section == 15) {
        return contains(ua, "tablet");
    }
    return 0;
}

static int fixture_expects_mobile(int section, const char *ua)
{
    if ((section >= 1 && section <= 3) ||
        (section >= 6 && section <= 8) ||
        section == 30) {
        return 1;
    }
    if (section >= 10 && section <= 15 && fixture_expects_tablet(section, ua) == 0) {
        return 1;
    }
    return 0;
}

static int fixture_expects_tv_like(int section)
{
    return section >= 20 && section <= 28;
}

static int fixture_expects_desktop(int section)
{
    return section == 5 || (section >= 17 && section <= 19);
}

static void assert_fixture_user_agent(int section, int line_no, char *ua)
{
    char name[160];
    int is_mobile;
    int is_tablet;
    int is_tv;
    int is_console;
    int is_set_top_box;
    int is_e_reader;
    int is_automotive;
    int is_wearable;
    int is_bot;

    lowercase(ua);
    is_mobile = checkIsMobile(ua, NULL);
    is_tablet = checkIsTablet(ua, NULL, NULL, NULL);
    is_tv = checkIsTV(ua);
    is_console = checkIsConsole(ua);
    is_set_top_box = checkIsSetTopBox(ua);
    is_e_reader = checkIsEReader(ua);
    is_automotive = checkIsAutomotive(ua);
    is_wearable = checkIsWearable(ua);
    is_bot = checkIsBot(ua);

    if (fixture_expects_bot(section)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is bot", section, line_no);
        assert_true(name, is_bot == 1);
        return;
    }

    if (fixture_expects_console(section)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is console", section, line_no);
        assert_true(name, is_console == 1 && is_tv == 1);
        return;
    }

    if (fixture_expects_set_top_box(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is set-top box", section, line_no);
        assert_true(name, is_set_top_box == 1 && is_tv == 1);
        return;
    }

    if (fixture_expects_e_reader(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is e-reader", section, line_no);
        assert_true(name, is_e_reader == 1);
        return;
    }

    if (fixture_expects_automotive(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is automotive", section, line_no);
        assert_true(name, is_automotive == 1);
        return;
    }

    if (fixture_expects_wearable(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is wearable", section, line_no);
        assert_true(name, is_wearable == 1);
        return;
    }

    if (fixture_expects_tv_like(section)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is tv-like", section, line_no);
        assert_true(name, is_tv == 1);
        return;
    }

    if (fixture_expects_tablet(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is tablet", section, line_no);
        assert_true(name, is_tablet == 1);
        return;
    }

    if (fixture_expects_mobile(section, ua)) {
        snprintf(name, sizeof(name), "fixture section %d line %d is mobile", section, line_no);
        assert_true(name, is_mobile == 1);
        return;
    }

    if (fixture_expects_desktop(section)) {
        snprintf(name, sizeof(name), "fixture section %d line %d stays desktop-like", section, line_no);
        assert_true(name, is_mobile == 0 && is_tablet == 0 && is_tv == 0);
    }
}

static void assert_complete_user_agent_fixture(void)
{
    FILE *fp;
    char line[4096];
    int section = 0;
    int line_no = 0;
    int tested = 0;

    fp = fopen("tests/fixtures/amfplus_useragents_complete.txt", "r");
    if (fp == NULL) {
        fprintf(stderr, "FAIL: cannot open complete user-agent fixture\n");
        failures++;
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len;

        line_no++;
        len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (strncmp(line, "SECTION ", 8) == 0) {
            section = atoi(line + 8);
            continue;
        }

        if (is_fixture_user_agent_line(line, section)) {
            assert_fixture_user_agent(section, line_no, line);
            tested++;
        }
    }

    fclose(fp);
    assert_true("complete user-agent fixture has samples", tested > 100);
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

    load_detection_rules();

    assert_true("samsung galaxy s chrome is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 15; sm-s938b) applewebkit/537.36 (khtml, like gecko) chrome/137.0.7151.68 mobile safari/537.36", NULL) == 1);
    assert_true("samsung internet phone is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 15; samsung sm-s938b) applewebkit/537.36 (khtml, like gecko) samsungbrowser/27.0 chrome/125.0.0.0 mobile safari/537.36", NULL) == 1);
    assert_true("samsung foldable is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 15; sm-f956b) applewebkit/537.36 (khtml, like gecko) chrome/137.0.7151.68 mobile safari/537.36", NULL) == 1);
    assert_true("pixel phone is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 15; pixel 9 pro) applewebkit/537.36 (khtml, like gecko) chrome/137.0.7151.68 mobile safari/537.36", NULL) == 1);
    assert_true("xiaomi phone is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 14; xiaomi 14) applewebkit/537.36 (khtml, like gecko) chrome/128.0.6613.99 mobile safari/537.36", NULL) == 1);
    assert_true("realme phone is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 14; rmx3851) applewebkit/537.36 (khtml, like gecko) chrome/128.0.6613.99 mobile safari/537.36", NULL) == 1);
    assert_true("motorola phone is mobile",
                checkIsMobile("mozilla/5.0 (linux; android 14; motorola edge 50 pro) applewebkit/537.36 (khtml, like gecko) chrome/128.0.6613.99 mobile safari/537.36", NULL) == 1);
    assert_true("samsung galaxy tab is tablet",
                checkIsTablet("mozilla/5.0 (linux; android 14; sm-x910) applewebkit/537.36 (khtml, like gecko) chrome/128.0.6613.99 safari/537.36", NULL, NULL, NULL) == 1);
    assert_true("samsung galaxy tab a is tablet",
                checkIsTablet("mozilla/5.0 (linux; android 13; sm-t636b) applewebkit/537.36 (khtml, like gecko) chrome/126.0.6478.110 safari/537.36", NULL, NULL, NULL) == 1);
    assert_true("amazon fire silk tablet is tablet",
                checkIsTablet("mozilla/5.0 (linux; android 11; kftrwi) applewebkit/537.36 (khtml, like gecko) silk/112.4.1 like chrome/112.0.0.0 safari/537.36", NULL, NULL, NULL) == 1);
    assert_true("samsung smart tv tizen is tv",
                checkIsTV("mozilla/5.0 (smart-tv; linux; tizen 8.0) applewebkit/537.36 (khtml, like gecko) samsungbrowser/7.0 tv safari/537.36") == 1);
    assert_true("android tv is tv",
                checkIsTV("mozilla/5.0 (linux; android 12; android tv; shield android tv) applewebkit/537.36 (khtml, like gecko) chrome/120.0.0.0 safari/537.36") == 1);
    assert_true("playstation is tv-like",
                checkIsTV("mozilla/5.0 (playstation 5 3.20) applewebkit/605.1.15 (khtml, like gecko)") == 1);
    assert_true("playstation has console flag",
                checkIsConsole("mozilla/5.0 (playstation 5 3.20) applewebkit/605.1.15 (khtml, like gecko)") == 1);
    assert_true("roku has set-top box flag",
                checkIsSetTopBox("mozilla/5.0 (roku/dvp-12.5) applewebkit/537.36 (khtml, like gecko) rokubrowser/1.0 safari/537.36") == 1);
    assert_true("kindle e-reader has e-reader flag",
                checkIsEReader("mozilla/5.0 (x11; u; linux armv7l like android; en-us) applewebkit/531.2+ kindle/3.0+ safari/531.2+") == 1);
    assert_true("android automotive has automotive flag",
                checkIsAutomotive("mozilla/5.0 (linux; android 12; android automotive) applewebkit/537.36 (khtml, like gecko) chrome/110.0.5481.65 safari/537.36") == 1);
    assert_true("watch has wearable flag",
                checkIsWearable("mozilla/5.0 (watch; cpu watchos 10_0 like mac os x) applewebkit/605.1.15 (khtml, like gecko) version/10.0 mobile/15e148 safari/604.1") == 1);
    assert_true("googlebot has bot flag",
                checkIsBot("mozilla/5.0 applewebkit/537.36 (khtml, like gecko; compatible; googlebot/2.1; +http://www.google.com/bot.html) chrome/w.x.y.z safari/537.36") == 1);

    assert_complete_user_agent_fixture();

    apr_pool_destroy(pool);
    apr_terminate();

    return failures == 0 ? 0 : 1;
}
