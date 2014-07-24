#include "test_env.h"

#if defined(TARGET_K64F)
AnalogIn in(A0);
AnalogOut out(DAC0_OUT);

#elif defined(TARGET_KL25Z)
AnalogIn in(PTC2);
AnalogOut out(PTE30);

#elif defined(TARGET_KL05Z)
AnalogIn in(PTB11);  // D9
AnalogOut out(PTB1); // D1

#elif defined(TARGET_KL46Z)
AnalogIn in(PTB0);
AnalogOut out(PTE30);

#elif defined(TARGET_EFM32LG_STK3600) \
    ||defined(TARGET_EFM32GG_STK3700) \
    ||defined(TARGET_EFM32TG_STK3300) \
    ||defined(TARGET_EFM32_G8XX_STK) \
    ||defined(TARGET_EFM32WG_STK3800)
AnalogIn in(PD5);
AnalogOut out(PB12);

#elif defined(TARGET_LPC1549)
AnalogIn in(A0);
AnalogOut out(D12); //D12 is P0_12, the DAC output pin

#else
AnalogIn in(p17);
AnalogOut out(p18);

#endif

#define ERROR_TOLLERANCE    0.05

int main() {
    bool check = true;

    for (float out_value=0.0; out_value<1.1; out_value+=0.1) {
        out.write(out_value);
        wait(0.1);

        float in_value = in.read();
        float diff = fabs(out_value - in_value);
        if (diff > ERROR_TOLLERANCE) {
            check = false;
            printf("ERROR (out:%.4f) - (in:%.4f) = (%.4f)"NL, out_value, in_value, diff);
        } else {
            printf("OK    (out:%.4f) - (in:%.4f) = (%.4f)"NL, out_value, in_value, diff);
        }
    }

    notify_completion(check);
}
