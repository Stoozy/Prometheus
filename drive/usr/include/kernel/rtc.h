#define CMOS_ADDR   0x70
#define CMOS_DATA   0x71    
#define CURRENT_CENTURY 20
#include "typedefs.h"
#include  <stdbool.h>

typedef struct datetime_t datetime_t;
struct datetime_t{
    uint8_t year;
    uint8_t month;
    uint8_t date;
	uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
};

bool updating();
uint8_t _read_rtc_reg(uint8_t reg);
void _set_rtc_reg(uint8_t reg, uint8_t val);
char * datetime_to_str(datetime_t * d);
void read_rtc();

