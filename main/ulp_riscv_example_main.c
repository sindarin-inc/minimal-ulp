/* ULP riscv DS18B20 1wire temperature sensor example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include <esp_adc/adc_oneshot.h>
#include "soc/rtc_periph.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "ulp_riscv.h"
#include "ulp_main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void);

void app_main(void)
{
    // If you configure the ADC before sleeping, you'll burn ~1.3mA of power while in deep sleep.
//     adc_oneshot_unit_init_cfg_t adcOneshotCfg;
//     adc_oneshot_unit_handle_t adcOneshotHandle;
//     adcOneshotCfg.unit_id = ADC_UNIT_1;
// #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
//     adcOneshotCfg.clk_src = TIMER_SRC_CLK_APB;
// #endif
//     adcOneshotCfg.ulp_mode = ADC_ULP_MODE_DISABLE;
//     ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcOneshotCfg, &adcOneshotHandle));

    // But if you release the ADC before sleeping, then you'll be back to the expected ~25uA during deep sleep.
    // ESP_ERROR_CHECK(adc_oneshot_del_unit(adcOneshotHandle));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    /* not a wakeup from ULP, load the firmware */
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not a ULP-RISC-V wakeup, initializing it! \n");
        init_ulp_program();
    }

    /* ULP Risc-V read and detected a change in GPIO_0, prints */
    if (cause == ESP_SLEEP_WAKEUP_ULP) {
        printf("ULP-RISC-V woke up the main CPU! \n");
    }

    /* Go back to sleep, only the ULP Risc-V will run */
    printf("Entering in deep sleep\n\n");

    /* Small delay to ensure the messages are printed */
    vTaskDelay(100);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    esp_deep_sleep_start();
}

static void init_ulp_program(void)
{
    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
     * The second argument is the period in microseconds, which gives a wakeup time period of: 20ms
     */
    ulp_set_wakeup_period(0, 1000000);

    /* Start the program */
    err = ulp_riscv_run();
    ESP_ERROR_CHECK(err);
}
