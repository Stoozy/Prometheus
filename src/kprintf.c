#include "kprintf.h"
#include "typedefs.h"

#include "misc/ssfn.h"
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#include "drivers/serial.h"
#include "util.h"

static bool is_digit(char c){
    return (c>47 && c<58);
}

static bool print(const char* data, size_t length) {
    const unsigned char* bytes = (const unsigned char*) data;
    turn_color_on();
    for (size_t i = 0; i < length; i++)
        write_serial(bytes[i]);
        //ssfn_putc(bytes[i]);
    turn_color_off();
    return true;
}


int kprintf(const char* restrict format, ...) {
    char buf[50];
    bool wide = 0;
    int padding = 0;
    int precision = 0;

    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    while (*format != '\0') {
        size_t maxrem = INT_MAX - written;
        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%')
                format++;
            size_t amount = 1;
            while (format[amount] && format[amount] != '%')
                amount++;
            if (maxrem < amount) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(format, amount))
                return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if(*format == '0'){
            format++;
        }

        // next character is the precision
        if (*format == '.') {
            format++;
            if (*format == '*') {
                precision = va_arg(parameters, int);
                format++;
            } else while (is_digit(*format)) {
                precision *= 10;
                precision += *format++ - '0';
            }
        } else {
            precision = -1;
        }

        if (*format == '*') {
            padding = va_arg(parameters, int);
            format++;
        } else while (is_digit(*format)) {
            padding *= 10;
            padding += *format++ - '0';
        }


        while(*format == 'l'){
            wide = 1; 
            format++;
        }



        if (*format == 'c') {
            format++;
            char c = (char) va_arg(parameters, int /* char promotes to int */);
            if (!maxrem) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(&c, sizeof(c)))
                return -1;
            written++;
        } else if (*format == 's') {
            format++;
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(str, len))
                return -1;
            written += len;
        } else if  (*format == 'd' ){
            // increment
            format++;
            if(wide){
                i64 i = va_arg(parameters, i64);
                if(i==0) kprintf("0");
                else{
                    if(padding){
                        // TODO: handle padding 
                    }else kprintf("%s", lltoa(i, &buf[0], 10));
                }
            }else{
                i32 i = va_arg(parameters, i32);
                if(i==0) kprintf("0");
                else{
                    if(padding){
                        /* TODO: Handle padding */
                    }else {
                        kprintf("%s", itoa(i, &buf[0], 10));
                    }
                }
            }

        }else if(*format=='u'){
            format++;
            if(wide){
                u64 i = va_arg(parameters, u64);
                kprintf("%s", ulltoa(i, &buf[0], 10));
            }else{
                unsigned int i = va_arg(parameters, unsigned int);
                kprintf("%s", ulltoa(i, &buf[0], 10));
            }
    
        }else if(*format == 'x'){
            format++;
            u32 hex = va_arg(parameters, u32);

            kprintf("%s", ulltoa(hex, &buf[0], 16));

        }
        else {
            format = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(format, len))
                return -1;
            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
    }

