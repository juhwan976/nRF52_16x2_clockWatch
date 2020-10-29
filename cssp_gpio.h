#ifndef CSSP_GPIO_H
#define CSSP_GPIO_H

#include "nrf52.h"
#include "nrf52_bitfields.h"

/* Redefine */
#define GPIO_DIR_INPUT         GPIO_PIN_CNF_DIR_Input         //0
#define GPIO_DIR_OUTPUT        GPIO_PIN_CNF_DIR_Output        //1

#define GPIO_INPUT_CONNECT     GPIO_PIN_CNF_INPUT_Connect     //0
#define GPIO_INPUT_DISCONNECT  GPIO_PIN_CNF_INPUT_Disconnect  //1

#define GPIO_PULL_DISABLED     GPIO_PIN_CNF_PULL_Disabled     //0
#define GPIO_PULL_PULLDOWN     GPIO_PIN_CNF_PULL_Pulldown     //1
#define GPIO_PULL_PULLUP       GPIO_PIN_CNF_PULL_Pullup       //3

#define GPIO_DRIVE_S0S1        GPIO_PIN_CNF_DRIVE_S0S1        //0
#define GPIO_DRIVE_H0S1        GPIO_PIN_CNF_DRIVE_H0S1        //1
#define GPIO_DRIVE_S0H1        GPIO_PIN_CNF_DRIVE_S0J1        //2
#define GPIO_DRIVE_H0H1        GPIO_PIN_CNF_DRIVE_H0H1        //3
#define GPIO_DRIVE_D0S1        GPIO_PIN_CNF_DRIVE_D0S1        //4
#define GPIO_DRIVE_D0H1        GPIO_PIN_CNF_DRIVE_D0H1        //5
#define GPIO_DRIVE_S0D1        GPIO_PIN_CNF_DRIVE_S0D1        //6
#define GPIO_DRIVE_H0D1        GPIO_PIN_CNF_DRIVE_H0D1        //7

#define GPIO_SENSE_DISABLED    GPIO_PIN_CNF_SENSE_Disabled    //0
#define GPIO_SENSE_HIGH        GPIO_PIN_CNF_SENSE_High        //2
#define GPIO_SENSE_LOW         GPIO_PIN_CNF_SENSE_Low         //3

#define GPIO_OUT_LOW           GPIO_OUT_PIN0_Low              //0
#define GPIO_OUT_HIGH          GPIO_OUT_PIN0_High             //1

#define GPIO_OUTSET_LOW        GPIO_OUTSET_PIN0_Low           //0
#define GPIO_OUTSET_HIGH       GPIO_OUTSET_PIN0_High          //1
#define GPIO_OUTSET_SET        GPIO_OUTSET_PIN0_Set           //1

#define GPIO_OUTCLR_LOW        GPIO_OUTCLR_PIN0_Low           //0
#define GPIO_OUTCLR_HIGH       GPIO_OUTCLR_PIN0_High          //1
#define GPIO_OUTCLR_CLEAR      GPIO_OUTCLR_PIN0_Clear         //1

/* functions */
__STATIC_INLINE void cssp_gpio_config(uint8_t p_num, uint8_t dir, uint8_t input, uint8_t pull, uint8_t drive, uint8_t sense) {
    NRF_P0->PIN_CNF[p_num] = ((dir << GPIO_PIN_CNF_DIR_Pos)
                        | (input << GPIO_PIN_CNF_INPUT_Pos)
                        | (pull << GPIO_PIN_CNF_PULL_Pos)
                        | (drive << GPIO_PIN_CNF_DRIVE_Pos)
                        | (sense << GPIO_PIN_CNF_SENSE_Pos));

}

__STATIC_INLINE uint8_t cssp_gpio_read(uint8_t p_num) {
    return (((NRF_P0->IN)>>p_num) & 1UL);
}

//__STATIC_INLINE void csp_gpio_out(uint8_t p_num, uint8_t condition) {
//    if(((((reg->OUT) << (31 -p_num)) >> 31) & 1UL) && !condition)
//        reg->OUT -= (1UL << (GPIO_OUT_PIN0_Pos +p_num));
//    else if(!((((reg->OUT) << (31 -p_num)) >> 31) & 1UL))
//        reg->OUT += (condition << (GPIO_OUT_PIN0_Pos +p_num));
//}

__STATIC_INLINE void cssp_gpio_outset(uint8_t p_num) {
    NRF_P0->OUTSET = 1UL << (GPIO_OUTSET_PIN0_Pos +p_num);
}

__STATIC_INLINE void cssp_gpio_outclr(uint8_t p_num) {
    NRF_P0->OUTCLR = 1UL << (GPIO_OUTCLR_PIN0_Pos +p_num);
}

__STATIC_INLINE void cssp_gpio_invert(uint8_t p_num) {
    if((NRF_P0->OUT)>>p_num)
        cssp_gpio_outclr(p_num);
    else
        cssp_gpio_outset(p_num);
}

#endif /* CSSP_GPIO_H */