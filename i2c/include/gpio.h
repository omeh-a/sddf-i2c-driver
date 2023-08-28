


#ifndef __GPIO_H__
#define __GPIO_H__
#define BIT(n)  (1U << (n))
#define GPIO_OFFSET 1024

#define GPIO_PINMUX_5 0xb5
#define GPIO_X16_I2C 
#define GPIO_PM5_X16 BIT(0) | BIT(1) | BIT(2) | BIT(3)
#define GPIO_PM5_X17 BIT(4) | BIT(5) | BIT(6) | BIT(7)
#define GPIO_PM5_X_I2C 1



#define GPIO_PINMUX_E 0xbe
#define GPIO_PE_A14 BIT(27) | BIT(26) | BIT(25) | BIT(24)
#define GPIO_PE_A15 BIT(31) | BIT(30) | BIT(29) | BIT(28)
#define GPIO_PE_A_I2C 3


#endif


