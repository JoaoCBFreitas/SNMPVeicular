#include "auxFunc.h"

errorDescrList *readErrorDescr(errorDescrList *samples)
{
    samples = (errorDescrList *)malloc(sizeof(errorDescrList));
    samples->capacity = 1;
    samples->current = 0;
    samples->errorList = malloc(sizeof(errorDescrList));
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("./Files/errorDescription.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        errorDescrStruct *aux = (errorDescrStruct *)malloc(sizeof(errorDescrStruct));
        aux->errorDescrID = i;
        aux->errorDescr = malloc(sizeof(char) * 65535);
        strtok(line, "\n");
        strcpy(aux->errorDescr, line);
        if (samples->capacity == samples->current)
        {
            errorDescrList *samples2 = (errorDescrList *)malloc(sizeof(errorDescrList));
            samples2->capacity = samples->capacity * 2;
            samples2->current = samples->current;
            samples2->errorList = malloc(sizeof(errorDescrStruct) * samples2->capacity);
            for (int j = 0; j < samples->current; j++)
            {
                samples2->errorList[j] = samples->errorList[j];
            }
            samples->capacity = samples2->capacity;
            samples->current = samples2->current;
            samples->errorList = samples2->errorList;
            free(samples2);
        }
        i++;
        samples->errorList[samples->current] = *aux;
        samples->current++;
    }
    fclose(fp);
    if (line)
        free(line);
    return samples;
}
int addToSampleUnits(sampleUnitsList *samples, long message, char* unit)
{
    for(int i=0;i<samples->current;i++){
		if(strcmp(unit,"")==0)
			unit="N/A";
        if(strcmp(samples->list[i].unit,unit)==0)
            return i;
    }

    sampleUnitsStruct *aux = (sampleUnitsStruct *)malloc(sizeof(sampleUnitsStruct));
    aux->id=samples->current;
	aux->message=message;
    aux->unit = malloc(sizeof(char) * 1024);
	if(strcmp("",unit)==0){
		strcpy(aux->unit,"N/A");
	}else{
		strcpy(aux->unit,unit);
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
    return samples->current-1;
}
int addToGenericTypes(genericTypeList *samples,long message,char* description)
{
	for(int i=0;i<samples->current;i++){
		if(strcmp(description,"")==0)
			description="N/A";
        if(strcmp(samples->genericList[i].typeDescription,description)==0)
            return i;
    }
	genericTypeStruct *aux = (genericTypeStruct *)malloc(sizeof(genericTypeStruct));
	aux->genericTypeID = samples->current;
	aux->message=message;
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
    return samples->current-1;
}
