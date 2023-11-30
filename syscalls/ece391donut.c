#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

double pow(double base, int exponent) {
    if (exponent == 0) {
        return 1; 
    }
    int i;
    double result = 1;
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

double factorial(int n) {
    double result = 1;
    int i;
    for (i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double sin_approx(double x) {
    double result = 0;
    int i;
    for (i = 0; i < 10; i++) {
        double term = (i % 2 == 0 ? 1 : -1) * pow(x, 2 * i + 1) / factorial(2 * i + 1);
        result += term;
    }
    return result;
}

double cos_approx(double x) {
    double result = 0;
    int i;
    for (i = 0; i < 10; i++) {
        double term = (i % 2 == 0 ? 1 : -1) * pow(x, 2 * i) / factorial(2 * i);
        result += term;
    }
    return result;
}

int main ()
{
    double A = 0, B = 0;
    double j;
    int i;
    int k;
    double z[1760];
    char b[1760];
    char buffer[1024];
    *buffer = "\x1b[2J";
    ece391_fdputs(1, (uint8_t *)buffer);
    for(;;) {
        for (i = 0; i < 1760; i++) {
            b[i] = 32;
        }
        for (i = 0; i < 1760; i++) {
            z[i] = 0;
        }
        for(j=0; j < 6.28; j += 0.07) {
            for(i=0; i < 6.28; i += 0.02) {
                double c = sin_approx(i);
                double d = cos_approx(j);
                double e = sin_approx(A);
                double f = sin_approx(j);
                double g = cos_approx(A);
                double h = d + 2;
                double D = 1 / (c * h * e + f * g + 5);
                double l = cos_approx(i);
                double m = cos_approx(B);
                double n = sin_approx(B);
                double t = c * h * g - f * e;
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
        *buffer = "\x1b[H";
        ece391_fdputs(1, (uint8_t *)buffer);
        for(k = 0; k < 1761; k++) {
            char ch = k % 80 ? b[k] : 10;
            ece391_write(1, &ch, 1);
            A += 0.00004;
            B += 0.00002;
        }
    }
    return 0;
}