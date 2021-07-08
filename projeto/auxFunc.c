#include "auxFunc.h"

int addToSampleUnits(sampleUnitsList *samples, long message, char *unit)
{
    for (int i = 0; i < samples->current; i++)
    {
        if (strcmp(unit, "") == 0)
            unit = "N/A";
        if (strcmp(samples->list[i].unit, unit) == 0)
            return i;
    }

    sampleUnitsStruct *aux = (sampleUnitsStruct *)malloc(sizeof(sampleUnitsStruct));
    aux->id = samples->current;
    aux->message = message;
    aux->unit = malloc(sizeof(char) * 1024);
    if (strcmp("", unit) == 0)
    {
        strcpy(aux->unit, "N/A");
    }
    else
    {
        strcpy(aux->unit, unit);
    }
    if (samples->capacity == samples->current)
    {
        sampleUnitsList *samples2 = (sampleUnitsList *)malloc(sizeof(sampleUnitsList));
        samples2->capacity = samples->capacity * 2;
        samples2->current = samples->current;
        samples2->list = malloc(sizeof(sampleUnitsStruct) * samples2->capacity);
        for (int j = 0; j < samples->current; j++)
        {
            samples2->list[j] = samples->list[j];
        }
        samples->capacity = samples2->capacity;
        samples->current = samples2->current;
        samples->list = samples2->list;
        free(samples2);
    }
    samples->list[samples->current] = *aux;
    samples->current++;
    return samples->current - 1;
}
int addToGenericTypes(genericTypeList *samples, long message, char *description)
{
    for (int i = 0; i < samples->current; i++)
    {
        if (strcmp(description, "") == 0)
            description = "N/A";
        if (strcmp(samples->genericList[i].typeDescription, description) == 0)
            return i;
    }
    genericTypeStruct *aux = (genericTypeStruct *)malloc(sizeof(genericTypeStruct));
    aux->genericTypeID = samples->current;
    aux->message = message;
    aux->typeDescription = malloc(sizeof(char) * 65535);
    strcpy(aux->typeDescription, description);
    if (samples->capacity == samples->current)
    {
        genericTypeList *samples2 = (genericTypeList *)malloc(sizeof(genericTypeList));
        samples2->capacity = samples->capacity * 2;
        samples2->current = samples->current;
        samples2->genericList = malloc(sizeof(genericTypeStruct) * samples2->capacity);
        for (int j = 0; j < samples->current; j++)
        {
            samples2->genericList[j] = samples->genericList[j];
        }
        samples->capacity = samples2->capacity;
        samples->current = samples2->current;
        samples->genericList = samples2->genericList;
        free(samples2);
    }
    samples->genericList[samples->current] = *aux;
    samples->current++;
    return samples->current - 1;
}
char *encode_int(int i)
{
    char *c = (char *)malloc(sizeof(char) * 9);
    char *hextable = "0123456789ABCDEF";
    for (int j = 0; j < 4; j++)
    {
        c[(j << 1)] = hextable[((i % 256) >> 4)];
        c[(j << 1) + 1] = hextable[((i % 256) % 16)];
        i = (i >> 8);
    }
    c[8] = 0;
    return c;
}
int checksum(char *str)
{
    int i;
    int chk = 0x12345678;
    for (i = 0; str[i] != '\0'; i++)
    {
        chk += ((int)(str[i]) * (i + 1));
    }
    return chk;
}
char *createChecksum(char *input)
{
    return encode_int(checksum(input));
}
struct tm *convertTime(struct tm *tm, char *timestamp)
{
    char *day = malloc(sizeof(char) * 3);
    char *mon = malloc(sizeof(char) * 3);
    char *year = malloc(sizeof(char) * 5);
    char *hour = malloc(sizeof(char) * 3);
    char *min = malloc(sizeof(char) * 3);
    char *sec = malloc(sizeof(char) * 3);

    strncpy(day, timestamp, 2);
    strncpy(mon, timestamp + 3, 2);
    strncpy(year, timestamp + 6, 4);
    strncpy(hour, timestamp + 11, 2);
    strncpy(min, timestamp + 14, 2);
    strncpy(sec, timestamp + 17, 2);

    day[2] = '\0';
    mon[2] = '\0';
    year[4] = '\0';
    hour[2] = '\0';
    min[2] = '\0';
    sec[2] = '\0';

    tm->tm_sec = atoi(sec);
    tm->tm_min = atoi(min);
    tm->tm_hour = atoi(hour);
    tm->tm_mday = atoi(day);
    tm->tm_mon = atoi(mon) - 1;
    tm->tm_year = atoi(year) - 1900;

    free(day);
    free(mon);
    free(year);
    free(hour);
    free(min);
    free(sec);
    return tm;
}
struct tm *deepCopyTM(struct tm *tm, struct tm *time)
{
    time->tm_zone = tm->tm_zone;
    time->tm_year = tm->tm_year;
    time->tm_yday = tm->tm_yday;
    time->tm_wday = tm->tm_wday;
    time->tm_sec = tm->tm_sec;
    time->tm_mon = tm->tm_mon;
    time->tm_min = tm->tm_min;
    time->tm_mday = tm->tm_mday;
    time->tm_isdst = tm->tm_isdst;
    time->tm_hour = tm->tm_hour;
    time->tm_gmtoff = tm->tm_gmtoff;
    return time;
}
int compareTimeStamp(struct tm *time1, struct tm *time2)
{
    if (time1->tm_year + 1900 == time2->tm_year + 1900)
        if (time1->tm_mon + 1 == time2->tm_mon + 1)
            if (time1->tm_mday == time2->tm_mday)
                if (time1->tm_hour == time2->tm_hour)
                    if (time1->tm_min == time2->tm_min)
                        if (time1->tm_sec == time2->tm_sec)
                            return 0;
                        else if (time1->tm_sec > time2->tm_sec)
                            return 2;
                        else
                            return 1;
                    else if (time1->tm_min > time2->tm_min)
                        return 2;
                    else
                        return 1;
                else if (time1->tm_hour > time2->tm_hour)
                    return 2;
                else
                    return 1;
            else if (time1->tm_mday > time2->tm_mday)
                return 2;
            else
                return 1;
        else if (time1->tm_mon + 1 > time2->tm_mon + 1)
            return 2;
        else
            return 1;
    else if (time1->tm_year + 1900 > time2->tm_year + 1900)
        return 2;
    else
        return 1;
}
struct tm *addToTime(struct tm *time, int hour, int minutes)
{
    if (hour > 23 || hour < 0 || minutes < 0 || minutes > 59)
        exit(EXIT_FAILURE);
    time->tm_min += minutes;
    if (time->tm_min > 59)
    {
        time->tm_min -= 60;
        hour += 1;
    }
    time->tm_hour += hour;
    if (time->tm_hour > 23)
    {
        time->tm_hour -= 24;
        time->tm_mday += 1;
        if (time->tm_mon % 2 != 0 && time->tm_mday > 31)
        {
            time->tm_mday -= 31;
            time->tm_mon += 1;
        }
        else if (time->tm_mon == 2 && time->tm_mday > 28 && ((time->tm_year + 1900) % 400 == 0) || ((time->tm_year + 1900) % 4 == 0))
        {
            time->tm_mday -= 29;
            time->tm_mon += 1;
        }
        else if (time->tm_mon == 2 && time->tm_mday > 27)
        {
            time->tm_mday -= 28;
            time->tm_mon += 1;
        }
        else
        {
            time->tm_mday -= 30;
            time->tm_mon += 1;
        }
    }
    if (time->tm_mon > 11)
    {
        time->tm_mon -= 12;
        time->tm_year += 1;
    }
    return time;
}
int validateTime(char *time)
{
    if (strlen(time) != 8)
        return 1;
    char *hour = malloc(sizeof(char) * 3);
    char *min = malloc(sizeof(char) * 3);
    strncpy(hour, time, 2);
    strncpy(min, time + 3, 2);
    min[2] = '\0';
    hour[2] = '\0';
    int m = atoi(min);
    int h = atoi(hour);
    free(min);
    free(hour);
    if (m < 0 || m > 59)
        return 1;
    if (h < 0 || h > 23)
        return 1;
    return 0;
}