#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static int is_digit(char c) { return (c > 47 && c < 58); }

static int print(const char *data, size_t length) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < length; i++) {
    putchar(bytes[i]);
  }
  return 1;
}

int _printf(const char *restrict format, ...) {
  char buf[50];
  int wide = 0;
  int padding = 0;
  int precision = 0;

  va_list parameters;
  va_start(parameters, format);

  int written = 0;

  while (*format != '\0') {
    size_t maxrem = INT64_MAX - written;
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

    const char *format_begun_at = format++;

    if (*format == 'c') {
      format++;
      char c = (char)va_arg(parameters, int);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(&c, sizeof(c)))
        return -1;
      written++;
    } else if (*format == 's') {
      format++;
      const char *str = va_arg(parameters, const char *);
      size_t len = strlen(str);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(str, len))
        return -1;
      written += len;
    } else if (*format == 'd') {
      // increment
      format++;
      if (wide) {
        int64_t i = va_arg(parameters, int64_t);
        if (i == 0)
          _printf("0");
        else {
          if (padding) {
            // TODO: handle padding
          } else
            _printf("%s", lltoa(i, &buf[0], 10));
        }
      } else {
        int32_t i = va_arg(parameters, int32_t);
        if (i == 0)
          _printf("0");
        else {
          if (padding) {
            /* TODO: Handle padding */
          } else {
            _printf("%s", itoa(i, &buf[0], 10));
          }
        }
      }

    } else if (*format == 'u') {
      format++;
      if (wide) {
        uint64_t i = va_arg(parameters, uint64_t);
        _printf("%s", ulltoa(i, &buf[0], 10));
      } else {
        unsigned int i = va_arg(parameters, unsigned int);
        _printf("%s", ulltoa(i, &buf[0], 10));
      }

    } else if (*format == 'x') {
      format++;
      uint64_t hex = va_arg(parameters, uint64_t);

      _printf("%s", ulltoa(hex, &buf[0], 16));

    } else {
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
