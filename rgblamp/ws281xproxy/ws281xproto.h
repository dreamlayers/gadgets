#define WSCMD_SRGB      1
#define WSCMD_PWM       2

struct rgbmsg {
    unsigned int type;
    union {
        double rgb[3];
        unsigned short pwm[3];
    };
};

