#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <kernel/typedefs.h>


char buf[12];
char * convert(uint32_t i, uint32_t base);

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
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
		} else if  (*format == 'd'){
            format++;
            int i = va_arg(parameters, int);

            char buf[12]= {0};
            if(i>0) {

                int counter = 0;
                while(i!=0){
                    buf[counter] = (i%10)+48;
                    i/=10;
                    counter++;
                }
                    buf[counter] = '\0';

                for(int j=counter-1; j>=0; j--){
                    putchar(buf[j]);
                }
            }else if(i<0){
                putchar('-');
                i= i * -1;

                int counter = 0;
                while(i!=0){
                    buf[counter] = (i%10)+48;
                    i/=10;
                    counter++;
                }
                buf[counter] = '\0';

                for(int j=counter-1; j>=0; j--){
                    putchar(buf[j]);
                }
            }

            
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



