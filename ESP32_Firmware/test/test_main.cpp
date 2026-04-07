/**
 * @file test_main.cpp
 * @brief Centralized Hardware Validation Runner.
 * * This file uses preprocessor directives to selectively compile and run
 * specific hardware tests. To run a test, change its corresponding
 * ENABLE macro from 0 to 1.
 * * NOTE: It is recommended to enable only one test at a time to avoid
 * peripheral or Serial monitor resource conflicts.
 */

#include <Arduino.h>

// --- Test Selection Flags ---
// Set to 1 to enable the specific test, 0 to skip it during compilation
#define ENABLE_CONTROL_NTC_FANS_TEST 0
#define ENABLE_ENVIRONMENTAL_SENSORS_TEST 0
#define ENABLE_I2C_SCANNER_TEST 0
#define ENABLE_LED_ARRAY_TEST 0
#define ENABLE_PUMPS_LOGIC_TEST 0

/**
 * @brief Main Setup function.
 * Dispatches to the initialization function of the enabled test module.
 */
void setup(void)
{
    // Initialize Serial communication for all tests
    Serial.begin(115200);

#if (ENABLE_CONTROL_NTC_FANS_TEST == 1)
    setup_control_ntc_fans();
#endif

#if (ENABLE_ENVIRONMENTAL_SENSORS_TEST == 1)
    setup_environmental_sensors();
#endif

#if (ENABLE_I2C_SCANNER_TEST == 1)
    setup_i2c_scanner();
#endif

#if (ENABLE_LED_ARRAY_TEST == 1)
    setup_led_array();
#endif

#if (ENABLE_PUMPS_LOGIC_TEST == 1)
    setup_pumps_logic();
#endif
}

/**
 * @brief Main Loop function.
 * Dispatches to the repetitive logic of the enabled test module.
 */
void loop(void)
{
#if (ENABLE_CONTROL_NTC_FANS_TEST == 1)
    loop_control_ntc_fans();
#endif

#if (ENABLE_ENVIRONMENTAL_SENSORS_TEST == 1)
    loop_environmental_sensors();
#endif

#if (ENABLE_I2C_SCANNER_TEST == 1)
    loop_i2c_scanner();
#endif

#if (ENABLE_LED_ARRAY_TEST == 1)
    loop_led_array();
#endif

#if (ENABLE_PUMPS_LOGIC_TEST == 1)
    loop_pumps_logic();
#endif
}