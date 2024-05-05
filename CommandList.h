#ifndef COMMANDLIST_H
#define COMMANDLIST_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "HC05.h"
#include "hardware/rtc.h"
#include <time.h>
#include "pico/util/datetime.h"
#include "PressureSensor.h"

// Globals
extern uint16_t Scale_Threshold;
extern uint8_t Scale_Sensitivity;
extern bool In_Alarm_Window;
extern datetime_t Alarm_Window_Start;
extern datetime_t Alarm_Window_Stop;


// Defines 
#define NUMBER_OF_COMMANDS          10
#define COMMAND_LENGTH              8              // in bytes 

// ==================== Application specific stuff ==================== 

#define LOG2_WEIGHT_SAMPLES         4              // So we sample 16 times per measurement
#define WEIGHT_SAMPLES              0x01 << LOG2_WEIGHT_SAMPLES
#define MAX_ALARM_WINDOW            86400          // In seconds, so one day in this case
#define MAX_ALARM_MESSAGE           "Alarm window cannot be greater than 24 hours\n"

typedef struct SetClockStruct{
    uint8_t     SKIP0;
    uint8_t     year[4];
    uint8_t     SKIP1;
    uint8_t     month[2];
    uint8_t     SKIP2;
    uint8_t     day[2];
    uint8_t     SKIP3;
    uint8_t     dotw;
    uint8_t     SKIP4;
    uint8_t     hour[2];
    uint8_t     SKIP5;
    uint8_t     min[2];
    uint8_t     SKIP6;
    uint8_t     sec[2];
} SetClockType;

typedef struct WindowStruct{
    SetClockType    StartTime;
    SetClockType    EndTime;
} WindowType;

int str2int(const char* str, int len){
    int i;
    int ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

 long gregorian_calendar_to_jd(int y, int m, int d){
	y+=8000;
	if(m<3) { y--; m+=12; }
	return (y*365) +(y/4) -(y/100) +(y/400) -1200820
              +(m*153+3)/5-92
              +d-1
	;
}

// returns true iff t1 is less than t2
bool TimeCompare(datetime_t t1, datetime_t t2){
    if(t2.year < t1.year) return 0;
    if(t2.year == t1.year){
        if(t2.month < t1.month) return 0;
        if(t2.month == t1.month){
            if(t2.day < t1.day) return 0;
            if(t2.day == t1.day){
                if(t2.hour < t1.hour) return 0;
                if(t2.hour == t1.hour){
                    if(t2.min < t1.min) return 0;
                    if(t2.min == t1.min){
                        if(t2.sec <= t1.sec) return 0;
                    }
                }
            }
        }
    }
    return 1;
}

long TimeDifferenceSec(datetime_t t1, datetime_t t2){
    // First find julian day numbers from the times provided
    long jd1 = gregorian_calendar_to_jd(t1.year,t1.month,t1.day);
    long jd2 = gregorian_calendar_to_jd(t2.year,t2.month,t2.day);
    // Compute the number of days between the two provided dates
    long day_difference  = jd2 - jd1;
    // Compute the difference between the time hours
    int16_t hour_difference = t2.hour - t1.hour;
    // Compute the difference between the time minutes
    int16_t minute_difference = t2.min - t1.min;
    // Compute the difference between the time seconds
    int16_t second_difference = t2.sec - t1.sec;
    // Return the total diffrence in seconds
    return (day_difference*86400 + hour_difference*3600 + minute_difference*60 + second_difference);
}
void TimeDifference(datetime_t t1, datetime_t t2, long* days_hours_minutes_seconds){
    // Grab the total ellapsed seconds between the dates
    long ellapsed_seconds = TimeDifferenceSec(t1,t2);
    long days = (long) (ellapsed_seconds / 86400);
    long hours = (long) ((ellapsed_seconds - days*86400)/3600);
    long minutes = (long) ((ellapsed_seconds - days*86400 - hours*3600)/60);
    long seconds = (long) ((ellapsed_seconds - days*86400 - hours*3600 - minutes*60));
    *days_hours_minutes_seconds++ = days;
    *days_hours_minutes_seconds++ = hours;
    *days_hours_minutes_seconds++ = minutes;
    *days_hours_minutes_seconds++ = seconds;
    return;
}

// ====================================================================

// Types
typedef struct CMD_Struct {
    char* CmdName;
    void (*Callback)(void);
    char* Usage;
} CMD_Type;

// Callback Function Declerations
void Help_Callback(void);
void Set_Clock_Callback(void);
void Get_Clock_Callback(void);
void Zero_Scale_Callback(void);
void Get_Weight_Callback(void);
void Set_Scale_Threshold(void);
void Set_Scale_Sensitivity(void);
void Set_Alarm_Callback(void);
void Get_Alarm_Callback(void);
void Clear_Alarm_Callback(void);
void Enter_Alarm_Window(void);
void Exit_Alarm_Window(void);

// Command definitions 
CMD_Type CommandLookup[NUMBER_OF_COMMANDS] = {
    // Command Name     // Callback Function             //Usage Information
    {"HelpInfo",        &Help_Callback,                  "HelpInfo <Parameter_1>\n\nCalling HelpInfo with no parameters will list all available commands\n\nCalling HelpInfo with <Parameter_1> equal to the name of another function will give the usagae information for that function\n"},
    {"SetClock",        &Set_Clock_Callback,             "SetClock <Year> <Month> <Day> <Day of Week> <Hour> <Min> <Sec>\n\nEx: “SetClock 2023 01 14 6 15 45 00” sets the time to 3:45:00pm on Sat 14, Jan 2023\n"},
    {"GetClock",        &Get_Clock_Callback,             "GetClock\n\nReturns the current time the pi is set to.\n"},
    //{"LoadZero",        &Zero_Scale_Callback,            "LoadZero\n\nZeros the scale by setting current value to a global offset.\n"},
    {"SetUpper",        &Set_Scale_Threshold,            "SetUpper\n\nSets the upper bound for weight allowed during alarm period.\n"},
    {"SetTolTo",        &Set_Scale_Sensitivity,          "SetTolTo\n\nSets the sensitivity of the weight detection during alarm period. The alarm will trigger when the current weight equals tol % of the weight set by SetUpper.\n\n <Tol> = a percentage between 0 and 99.\n"},
    {"WeighNow",        &Get_Weight_Callback,            "WeighNow\n\n Measures and returns the current weight being read by the load cells.\n"},
    {"SetAlarm",        &Set_Alarm_Callback,             "SetAlarm <Year1> <Month1> <Day1> <Day of Week 1> <Hour1> <Min1> <Sec1> <Year2> <Month2> <Day2> <Day of Week 2> <Hour2> <Min2> <Sec2>\n\nEx: “SetAlarm 2023 01 14 6 15 45 00 2023 01 14 6 15 30” sets an alarm to start at 3:45:00pm on Sat 14, Jan 2023 and end 30 seconds later\n"},
    {"GetAlarm",        &Get_Alarm_Callback,             "GetAlarm\n\nReturns information about any alarms that are set.\n"},
    {"ClrAlarm",        &Clear_Alarm_Callback,           "ClrAlarm\n\nClears any alarms that may be set\n"}
};

// Callback Functions
void Help_Callback(void){
    uint8_t bufferarray[COMMAND_LENGTH];
    uint8_t* readbuffer = &bufferarray[0];
    // Read BT_CMD_LENGTH bytes from the FIFO
    uart_read_blocking_within_us(BLUETOOTH, readbuffer, 1+COMMAND_LENGTH, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);
    //Scrap first byte, assumed to be a space between the words
    readbuffer++;
    // See if first (COMMAND_LENGTH) bytes of the RX data correspond
    // to a command in the CommandLookup table and send the cmd info text
    for (uint8_t i; i < NUMBER_OF_COMMANDS; i++){
        if (strncmp((char*) readbuffer, CommandLookup[i].CmdName, COMMAND_LENGTH) == 0){
            BLUETOOTH_SEND(CommandLookup[i].Usage);
            return;
        }
    }
    // If none of the commands matched print out all commands
    BLUETOOTH_SEND("Available Commands:\n");
    for (uint8_t i; i < NUMBER_OF_COMMANDS; i++){
        BLUETOOTH_SEND("    ");
        BLUETOOTH_SEND(CommandLookup[i].CmdName);
        BLUETOOTH_SEND("\n");
    }

    return;
}

void Set_Clock_Callback(void){
    // First make sure we're not currently in an alarm window
    if (In_Alarm_Window){
        BLUETOOTH_SEND("Unable to change clock time while in alarm window\n");
        return;
    }

    uint16_t buffer_size = 24;
    uint8_t bufferarray[buffer_size];
    uint8_t* readbuffer = &bufferarray[0];
    // Read from FIFO until we hit the first new line char or time out or fill buffer
    size_t last_index = uart_read_until_within_us(BLUETOOTH, readbuffer, 10, 1, buffer_size, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);

    // Use a struture to split up the bytes into meaningfull chunks
    SetClockType* extracted_bytes = (SetClockType*) (readbuffer);

    // Next build a proper datetime_t struct from the structure of bytes
    datetime_t time_struct = {
            .year  = str2int((char*) &(extracted_bytes->year),4),
            .month = str2int((char*) &(extracted_bytes->month),2),
            .day   = str2int((char*) &(extracted_bytes->day),2),
            .dotw  = str2int((char*) &(extracted_bytes->dotw),1),
            .hour  = str2int((char*) &(extracted_bytes->hour),2),
            .min   = str2int((char*) &(extracted_bytes->min),2),
            .sec   = str2int((char*) &(extracted_bytes->sec),2)
    };

    // Finally set the RTC using the info in the time structure
    rtc_set_datetime(&time_struct);
    // and give the clock time to update
    busy_wait_us(64);
    // Send back clock value
    BLUETOOTH_SEND("Clock time is now: ");
    Get_Clock_Callback();

    return;
}

void Get_Clock_Callback(void){
    // Declare a time structure and string
    datetime_t time_struct = {
            .year  = 0,
            .month = 0,
            .day   = 0,
            .dotw  = 0,
            .hour  = 0,
            .min   = 0,
            .sec   = 0
    };
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    // Populate the time struture with the current time
    rtc_get_datetime(&time_struct);
    // Populate the string from the time_struct
    datetime_to_str(datetime_str, sizeof(datetime_buf), &time_struct);
    // Finally send back the string
    BLUETOOTH_SEND(datetime_str);
    BLUETOOTH_SEND("\n");
    return;
}

void Zero_Scale_Callback(void){
    return;
}

// Sends raw scale value back over bluetooth
void Get_Weight_Callback(void){
    BLUETOOTH_SEND("Measuring weight, please wait...\n");
    // Grab the current value on the ADC
    uint16_t Weight = adc_read();

    // Convert the output back to a string we can send over BT
    char sendbuffer[12];
    sprintf(sendbuffer, "%d", Weight);

    // Send value back over BT
    BLUETOOTH_SEND("The current weight value is: ");
    BLUETOOTH_SEND(sendbuffer);
    BLUETOOTH_SEND(" out of 4096\n");
    return;
}

void Set_Scale_Threshold(void){
    // First make sure we're not currently in an alarm window
    if (In_Alarm_Window){
        BLUETOOTH_SEND("Unable to change alarm weight threshold while in alarm window\n");
        return;
    }

    BLUETOOTH_SEND("Measuring weight, please wait...\n");
    uint16_t Weight = adc_read();

    Threshold = Weight;
    // Tell user everything went fine (We're optimists here)
    BLUETOOTH_SEND("Threshold set\n");
    return;
}

void Set_Scale_Sensitivity(void){
    // First make sure we're not currently in an alarm window
    if (In_Alarm_Window){
        BLUETOOTH_SEND("Unable to change alarm tolerance in alarm window\n");
        return;
    }

    uint16_t buffer_size = 4;
    uint8_t readbuffer[buffer_size];
    // Read from FIFO until we hit the first \n char or time out or fill buffer
    size_t last_index = uart_read_until_within_us(BLUETOOTH,(char*) &readbuffer, 10, 1, buffer_size, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);
    // Scrap first value in read buffer as it's assumed to be a space 
    // and set tolerance global to value we read
    Scale_Sensitivity = 100 - str2int((char*) (&readbuffer + 1), last_index - 2);

    BLUETOOTH_SEND("Tolerance set\n");

    return;
}

void Enter_Alarm_Window(void){
    // Set alarm window global
    In_Alarm_Window = true;
    // Set up new alarm to close window later
    rtc_set_alarm(&Alarm_Window_Stop, &Exit_Alarm_Window);
    return;
}
void Exit_Alarm_Window(void){
    // Clear alarm window global
    In_Alarm_Window = false;
    Clear_Alarm_Callback();
    return;
}

void Set_Alarm_Callback(void){
    // First make sure we're not currently in an alarm window
    if (In_Alarm_Window){
        BLUETOOTH_SEND("Unable to change alarm while in alarm window\n");
        return;
    }

    // Store old alarm values incase we need to revert
    datetime_t Alarm_Window_Start_old = Alarm_Window_Start;
    datetime_t Alarm_Window_Stop_old = Alarm_Window_Stop;

    uint16_t buffer_size = 48;
    uint8_t bufferarray[buffer_size];
    uint8_t* readbuffer = &bufferarray[0];
    // Read from FIFO until we hit the first new line char or time out or fill buffer
    size_t last_index = uart_read_until_within_us(BLUETOOTH, readbuffer, 10, 1, buffer_size, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);

    // Use a struture to split up the bytes into meaningfull chunks
    WindowType* extracted_bytes = (WindowType*) (readbuffer);

    // Next build a proper datetime_t structs from the structures of bytes
    Alarm_Window_Start.year  = str2int((char*) &(extracted_bytes->StartTime.year),4);
    Alarm_Window_Start.month = str2int((char*) &(extracted_bytes->StartTime.month),2);
    Alarm_Window_Start.day   = str2int((char*) &(extracted_bytes->StartTime.day),2);
    Alarm_Window_Start.dotw  = str2int((char*) &(extracted_bytes->StartTime.dotw),1);
    Alarm_Window_Start.hour  = str2int((char*) &(extracted_bytes->StartTime.hour),2);
    Alarm_Window_Start.min   = str2int((char*) &(extracted_bytes->StartTime.min),2);
    Alarm_Window_Start.sec   = str2int((char*) &(extracted_bytes->StartTime.sec),2);
    
    Alarm_Window_Stop.year  = str2int((char*) &(extracted_bytes->EndTime.year),4);
    Alarm_Window_Stop.month = str2int((char*) &(extracted_bytes->EndTime.month),2);
    Alarm_Window_Stop.day   = str2int((char*) &(extracted_bytes->EndTime.day),2);
    Alarm_Window_Stop.dotw  = str2int((char*) &(extracted_bytes->EndTime.dotw),1);
    Alarm_Window_Stop.hour  = str2int((char*) &(extracted_bytes->EndTime.hour),2);
    Alarm_Window_Stop.min   = str2int((char*) &(extracted_bytes->EndTime.min),2);
    Alarm_Window_Stop.sec   = str2int((char*) &(extracted_bytes->EndTime.sec),2);

    // Make sure start time is after the current time
    datetime_t current_time = {
            .year  = 0,
            .month = 0,
            .day   = 0,
            .dotw  = 0,
            .hour  = 0,
            .min   = 0,
            .sec   = 0
    };
    rtc_get_datetime(&current_time);
    if(!TimeCompare(current_time,Alarm_Window_Start)){
        // If not then revert times back, alert, and return
        Alarm_Window_Start = Alarm_Window_Start_old;
        Alarm_Window_Stop = Alarm_Window_Stop_old;
        BLUETOOTH_SEND("Alarm not set\n");
        BLUETOOTH_SEND("Start time must be after current time\n");
        return;
    }
    // Make sure end time is greater than start time
     if(!TimeCompare(Alarm_Window_Start,Alarm_Window_Stop)){
        // If not then revert times back, alert, and return
        Alarm_Window_Start = Alarm_Window_Start_old;
        Alarm_Window_Stop = Alarm_Window_Stop_old;
        BLUETOOTH_SEND("Alarm not set\n");
        BLUETOOTH_SEND("End time must be after start time\n");
        return;
    }
    // Prevent setting a window that is greater than MAX_ALARM_WINDOW
    if(TimeDifferenceSec(Alarm_Window_Start,Alarm_Window_Stop) > MAX_ALARM_WINDOW){
        // If not then revert times back, alert, and return
        Alarm_Window_Start = Alarm_Window_Start_old;
        Alarm_Window_Stop = Alarm_Window_Stop_old;
        BLUETOOTH_SEND("Alarm not set\n");
        BLUETOOTH_SEND(MAX_ALARM_MESSAGE);
        return;
    }

    // Set an alarm to fire when we hit start time
    // the call back will then set a new alarm for stop time when it is called
    rtc_set_alarm(&Alarm_Window_Start, &Enter_Alarm_Window);

    // Tell em it worked 
    BLUETOOTH_SEND("Alarm set successfully\n");

    return;
}

void Get_Alarm_Callback(void){
    // See if an alarm has been set
    if (Alarm_Window_Start.year  == 0 && Alarm_Window_Start.month == 0 && Alarm_Window_Start.day   == 0 && Alarm_Window_Start.dotw  == 0 && Alarm_Window_Start.hour  == 0 && Alarm_Window_Start.min   == 0 && Alarm_Window_Start.sec   == 0){
        BLUETOOTH_SEND("There are currently no alarms set\n");
        return;
    }

    // Get current time
     datetime_t current_time = {
            .year  = 0,
            .month = 0,
            .day   = 0,
            .dotw  = 0,
            .hour  = 0,
            .min   = 0,
            .sec   = 0
    };
    rtc_get_datetime(&current_time);
    // Prep array to hold time difference (4 long's)
    long arraybuffer[4];
    long* days_hours_minutes_seconds = &arraybuffer[0];
    long* days = days_hours_minutes_seconds;
    long* hours = days_hours_minutes_seconds + 1;
    long* minutes = days_hours_minutes_seconds + 2;
    long* seconds = days_hours_minutes_seconds + 3;
    char days_str[12];
    char hours_str[12];
    char minutes_str[12];
    char seconds_str[12];
    // Prep string to hold alarm start or end time
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    // Give different info if we're in the alarm window yet or not
    if(!In_Alarm_Window){
        // Convert alarm start time to string
        datetime_to_str(datetime_str, sizeof(datetime_buf), &Alarm_Window_Start);
        // Find time until alarm goes off
        TimeDifference(current_time,Alarm_Window_Start,days_hours_minutes_seconds);
        // Convert times to strings
        sprintf(days_str,"%ld",*days);
        sprintf(hours_str,"%ld",*hours);
        sprintf(minutes_str,"%ld",*minutes);
        sprintf(seconds_str,"%ld",*seconds);
        // Send info back over BT
        BLUETOOTH_SEND("There is an alarm set for:\n");
        BLUETOOTH_SEND(datetime_str);
        BLUETOOTH_SEND("\n\nThe alarm will go off in:\n");
        BLUETOOTH_SEND(days_str);
        BLUETOOTH_SEND(" days\n");
        BLUETOOTH_SEND(hours_str);
        BLUETOOTH_SEND(" hours\n");
        BLUETOOTH_SEND(minutes_str);
        BLUETOOTH_SEND(" minutes\n");
        BLUETOOTH_SEND(seconds_str);
        BLUETOOTH_SEND(" seconds \n");
    }else{
        // Convert alarm stop time to string
        datetime_to_str(datetime_str, sizeof(datetime_buf), &Alarm_Window_Stop);
        // Find time until alarm goes off
        TimeDifference(current_time,Alarm_Window_Stop,days_hours_minutes_seconds);
        // Convert times to strings
        sprintf(days_str,"%ld",*days);
        sprintf(hours_str,"%ld",*hours);
        sprintf(minutes_str,"%ld",*minutes);
        sprintf(seconds_str,"%ld",*seconds);
        // Send info back over BT
        BLUETOOTH_SEND("The current alarm is set to end at:\n");
        BLUETOOTH_SEND(datetime_str);
        BLUETOOTH_SEND("\n\nThe alarm window will end in:\n");
        BLUETOOTH_SEND(days_str);
        BLUETOOTH_SEND(" days\n");
        BLUETOOTH_SEND(hours_str);
        BLUETOOTH_SEND(" hours\n");
        BLUETOOTH_SEND(minutes_str);
        BLUETOOTH_SEND(" minutes\n");
        BLUETOOTH_SEND(seconds_str);
        BLUETOOTH_SEND(" seconds \n");
    }

    return;
}

void Clear_Alarm_Callback(void){
    // First make sure we're not currently in an alarm window
    if (In_Alarm_Window){
        BLUETOOTH_SEND("Unable to change alarm while in alarm window\n");
        return;
    }

    // Disable the alarm
    rtc_disable_alarm();
    // Set alarm times back to default values
    Alarm_Window_Start.year  = 0;
    Alarm_Window_Start.month = 0;
    Alarm_Window_Start.day   = 0;
    Alarm_Window_Start.dotw  = 0;
    Alarm_Window_Start.hour  = 0;
    Alarm_Window_Start.min   = 0;
    Alarm_Window_Start.sec   = 0;

    Alarm_Window_Stop.year  = 0;
    Alarm_Window_Stop.month = 0;
    Alarm_Window_Stop.day   = 0;
    Alarm_Window_Stop.dotw  = 0;
    Alarm_Window_Stop.hour  = 0;
    Alarm_Window_Stop.min   = 0;
    Alarm_Window_Stop.sec   = 0;

    BLUETOOTH_SEND("Alarm was cleared\n");

    return;
}

#endif