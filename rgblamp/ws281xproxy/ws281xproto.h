struct rgbmsg {
    unsigned int type;
    union {
        double rgb[3];
    };
};

