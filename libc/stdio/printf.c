#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <kernel/typedefs.h>
#include <kernel/util.h>


bool is_digit(char c){
    return (c>47 && c<58);
}

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}


int printf(const char* restrict format, ...) {
    char buf[50];
    bool wide = 0;
    char pad_with = ' ';
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
            pad_with = '0';
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
                long int i = va_arg(parameters, long int);
                if(i==0) printf("0");
			    else{
                    if(padding){
                        int p;
                        char * pad_str;
                        for(p = 0; p<padding; p++)
                            pad_str[p] = pad_with;
                        pad_str[p] = '\0';
                        char * str = itoa(i, &buf[p], 10);
			            printf("%s", strcat(pad_str, str));

                    }else printf("%s", itoa(i, &buf[0], 10));
                }
            }else{
                int i = va_arg(parameters, int);
                if(i==0) printf("0");
                else{
                    if(padding){
                        int p;
                        char * pad_str;
                        for(p = 0; p<padding; p++)
                            pad_str[p] = pad_with;
                        pad_str[p] = '\0';
                        char * str = itoa(i, &buf[p], 10);
			            printf("%s", strcat(pad_str, str));
                    }else {
                        printf("%s", itoa(i, &buf[0], 10));
                    }
                }
            }

        }else if(*format=='u'){
            format++;
            if(wide){
                unsigned long long i = va_arg(parameters, unsigned long long);
                printf("%s", itoa(i, &buf[0], 10));
            }else{
                unsigned int i = va_arg(parameters, unsigned int);
                printf("%s", itoa(i, &buf[0], 10));
            }
    
        }else if(*format == 'x'){
            format++;
            uint64_t hex = va_arg(parameters, uint64_t);

			printf("0x%s", itoa(hex, &buf[0], 16));

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

//char *convert(uint32_t num, uint32_t base) 
//{ 
//	static char Representation[]= "0123456789ABCDEF";
//	static char buffer[50]; 
//	char *ptr; 
//	
//	//ptr = &buffer[49]; 
//	//*ptr = '\0'; 
//    buffer[49] = '\0';
//	
//	do 
//	{ 
//		*--ptr = Representation[num%base]; 
//		num /= base; 
//	}while(num != 0); 
//	
//	return ptr; 
//}



