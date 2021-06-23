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