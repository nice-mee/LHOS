#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

float pow(float base, int exponent) {
    if (exponent == 0) {
        return 1; 
    }
    int i;
    float result = 1;
    if (exponent > 0) {
        for (i = 0; i < exponent; i++) {
            result *= base;
        }
    } else {
        for (i = 0; i < -exponent; i++) {
            result /= base;
        }
    }
    return result;
}

float factorial(int n) {
    float result = 1;
    int i;
    for (i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

float sin(float x) {
    float result = 0;
    int i;
    for (i = 0; i < 10; i++) {
        float term = (i % 2 == 0 ? 1 : -1) * pow(x, 2 * i + 1) / factorial(2 * i + 1);
        result += term;
    }
    return result;
}

float cos(float x) {
    float result = 0;
    int i;
    for (i = 0; i < 10; i++) {
        float term = (i % 2 == 0 ? 1 : -1) * pow(x, 2 * i) / factorial(2 * i);
        result += term;
    }
    return result;
}

int main ()
{
    float A = 0, B = 0;
    float j;
    float i;
    int k;
    float z[1760];
    char b[1760];
    ece391_ioctl(1, 1);
    ece391_fdputs(1, (uint8_t *)"\x1b[2J");
    for(;;) {
        int idx;
        for (idx = 0; idx < 1760; idx++) {
            b[idx] = 32;
        }
        for (idx = 0; idx < 7040; idx++) {
            z[idx] = 0;
        }
        for(j=0; j < 6.28; j += 0.07) {
            for(i=0; i < 6.28; i += 0.02) {
                float c = sin(i);
                float d = cos(j);
                float e = sin(A);
                float f = sin(j);
                float g = cos(A);
                float h = d + 2;
                float D = 1 / (c * h * e + f * g + 5);
                float l = cos(i);
                float m = cos(B);
                float n = sin(B);
                float t = c * h * g - f * e;
                int x = 40 + 30 * D * (l * h * m - t * n);
                int y= 12 + 15 * D * (l * h * n + t * m);
                int o = x + 80 * y;
                int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);
                if(22 > y && y > 0 && x > 0 && 80 > x && D > z[o]) {
                    z[o] = D;
                    b[o] = ".,-~:;=!*#$@"[N > 0 ? N : 0];
                }
            }
        }
        ece391_fdputs(1, (uint8_t *)"\x1b[H");
        for(k = 0; k < 1761; k++) {
            char ch = k % 80 ? b[k] : 10;
            ece391_write(1, &ch, 1);
            A += 0.00004;
            B += 0.00002;
        }
    }
    ece391_ioctl(1, 0);
    return 0;
}