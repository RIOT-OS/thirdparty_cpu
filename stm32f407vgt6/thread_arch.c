#include "stm32f407vgt6.h"
#include "attributes.h"

NORETURN void reboot(void)
{
    while (1) {
        NVIC_SystemReset();
    }
}
