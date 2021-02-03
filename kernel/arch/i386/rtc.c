#include <string.h>
#include <stdio.h>

#include <kernel/io.h>
#include <kernel/rtc.h>
#include <kernel/util.h>

// TODO: Add local TZs
datetime_t *current_time;

char ret[50];
char  month_map[13][4] = {"\0","Jan\0","Feb\0", "Mar\0", "Apr\0", 
                                "May\0", "Jun\0", "Jul\0", "Aug\0", 
                                "Sep\0", "Oct\0", "Nov\0", "Dec\0"
                            };
char day_map[8][4] = { "\0", "Sun\0", "Mon\0", "Tue\0", "Wed\0", "Thu\0", "Fri\0", "Sat\0"};

bool updating(){
    outb(CMOS_ADDR, 0x0A);
    uint32_t status = inb(CMOS_DATA);
    return (status & 0x80);
}

uint8_t _read_rtc_reg(uint8_t reg){
	outb(CMOS_ADDR, reg);
	uint8_t in  = inb(CMOS_DATA);
	return in;
}

void _set_rtc_register(uint16_t reg_num, uint8_t val) {
    outb(CMOS_ADDR, reg_num);
    outb(CMOS_DATA, val);
}


void compare_and_pad(int x, char * hms){
    if(x<10) {
        hms[0] = '0';
        hms[1] = (char)48+x;
        hms[2] = '\0';
    }
}

char * datetime_to_str(datetime_t * dt){	
	ret[49] = '\0';
	char day[4];

	char date[3];
	char month[3];
	char year[3];

    char hour[3];
    char min[3];
    char sec[3];

    current_time = dt;
    date[2]     = '\0';
    month[2]    = '\0';
    year[2]     = '\0';
	hour[2]     = '\0';
	min[2]      = '\0';
	sec[2]      = '\0';
	char * ret_ptr = &ret[0];

    itoa(current_time->month, month, 10);
    itoa(current_time->date, date, 10);
    itoa(current_time->year, year, 10);

	itoa(current_time->hour, hour, 10);
	itoa(current_time->min, min, 10);
	itoa(current_time->sec, sec, 10);

    // incase number is less than 10, pad with one 0
    compare_and_pad(current_time->date, date);
    compare_and_pad(current_time->month, month);
    compare_and_pad(current_time->year, year);


    compare_and_pad(current_time->hour, hour);
    compare_and_pad(current_time->min, min);
    compare_and_pad(current_time->sec, sec);

  
    strcat(ret_ptr, date);
    strcat(ret_ptr, ".");
    strcat(ret_ptr, month);
    strcat(ret_ptr, ".");
    strcat(ret_ptr, year);
    strcat(ret_ptr, " ");

    strcat(ret_ptr, day_map[current_time->day]);
    strcat(ret_ptr," ");


	char * ptr = &hour[0];
	strcat(ret_ptr, ptr);
	strcat(ret_ptr, ":");

	ptr = &min[0];
	strcat(ret_ptr, ptr);
	strcat(ret_ptr, ":");

	ptr = &sec[0];
	strcat(ret_ptr, ptr);


	printf("%s UTC \n", ret);
	for(int i=0; i<49; i++){
		ret[i] = 0;
	}
	return &ret[0];
}

void read_rtc(){
    while(!updating()){
        asm("nop");
    }

	current_time->sec = _read_rtc_reg(0x00);
	current_time->min = _read_rtc_reg(0x02);
	current_time->hour = _read_rtc_reg(0x04);
	current_time->day = _read_rtc_reg(0x06);	// weekday
	current_time->date = _read_rtc_reg(0x07);
	current_time->month = _read_rtc_reg(0x08);
	current_time->year = _read_rtc_reg(0x09);

	uint8_t registerB = _read_rtc_reg(0x0B);
    if (!(registerB & 0x04)) {
        current_time->sec = (current_time->sec & 0x0F) + ((current_time->sec / 16) * 10);

        current_time->min = (current_time->min & 0x0F) + ((current_time->min / 16) * 10);

        current_time->hour = ( (current_time->hour & 0x0F) + (((current_time->hour & 0x70) / 16) * 10) ) | (current_time->hour & 0x80);

        current_time->day = (current_time->day & 0x0F) + ((current_time->day / 16) * 10);
        current_time->date = (current_time->date & 0x0F) + ((current_time->date / 16) * 10);
        current_time->month = (current_time->month & 0x0F) + ((current_time->month / 16) * 10);
        current_time->year = (current_time->year & 0x0F) + ((current_time->year / 16) * 10);
    }

	datetime_to_str(current_time);
}

datetime_t get_time(){	
	read_rtc();
	return *current_time;
}

