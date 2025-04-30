#include <unity.h>
#include "../trail_Capture_cam"

void setUp(void) {
    // Setup code before each test
}

void tearDown(void) {
    // Cleanup code after each test
}

void test_radar_packet_validation(void) {
    uint8_t valid_packet[24] = {0xAA, 0xFF, /* ... */};
    uint8_t invalid_packet[24] = {0x00, 0x00, /* ... */};
    
    TEST_ASSERT_TRUE(validateRadarPacket(valid_packet));
    TEST_ASSERT_FALSE(validateRadarPacket(invalid_packet));
}

void test_system_diagnostics(void) {
    SystemDiagnostics diag = getDiagnostics();
    TEST_ASSERT_GREATER_THAN(0, diag.freeHeap);
    TEST_ASSERT_GREATER_THAN(0, diag.sdCardSpace);
    TEST_ASSERT_LESS_THAN(100, diag.cpuTemperature);
}

void test_error_logging(void) {
    logError(ERR_CAMERA_INIT, "Test error message");
    File errorLog = SD_MMC.open("/error_log.txt", FILE_READ);
    TEST_ASSERT_NOT_NULL(errorLog);
    String lastLine = errorLog.readStringUntil('\n');
    TEST_ASSERT_NOT_EQUAL(0, lastLine.length());
    errorLog.close();
}

void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_radar_packet_validation);
    RUN_TEST(test_system_diagnostics);
    RUN_TEST(test_error_logging);
    UNITY_END();
}

void setup() {
    delay(2000);
    RUN_UNITY_TESTS();
}

void loop() {
    // Empty
}