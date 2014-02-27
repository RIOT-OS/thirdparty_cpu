#include "stm32f10x.h"
#include "attributes.h"

NORETURN void reboot(void)
{
    while (1) {
        NVIC_SystemReset();
    }
}
