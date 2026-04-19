#include <stdint.h>
#include "ulp_lp_core.h"
#include "ulp_lp_core_gpio.h"
#include "ulp_lp_core_utils.h"

#define WAKE_PIN GPIO_NUM_4

int main(void) {
    // Configure the pin for the LP core
    ulp_lp_core_gpio_init(WAKE_PIN);
    ulp_lp_core_gpio_input_enable(WAKE_PIN);

    while (1) {
        // Check if the pin was grounded
        if (ulp_lp_core_gpio_get_level(WAKE_PIN) == 0) {
            
            // Wake up the Big Brain
            ulp_lp_core_wakeup_main_processor();
            break;
        }
        
        // Sleep for 100ms to save power between checks
        ulp_lp_core_delay_us(100000); 
    }
    return 0;
}