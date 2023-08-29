


#ifndef __GPIO_H__
#define __GPIO_H__
#define BIT(n)  (1U << (n))
#define GPIO_OFFSET 1024

#define GPIO_PINMUX_5 0xb5
#define GPIO_X16_I2C 
#define GPIO_PM5_X17 BIT(4) | BIT(5) | BIT(6) | BIT(7)
#define GPIO_PM5_X18 BIT(8) | BIT(9) | BIT(10) | BIT(11)
#define GPIO_PM5_X_I2C 1



#define GPIO_PINMUX_E 0xbe
#define GPIO_PE_A14 BIT(27) | BIT(26) | BIT(25) | BIT(24)
#define GPIO_PE_A15 BIT(31) | BIT(30) | BIT(29) | BIT(28)
#define GPIO_PE_A_I2C 3


#endif


