#pragma once

#include <stdlib.h>

struct InterconnectParameters
{
    TaskHandle_t *interconnectTask;
};

void vInterconnectTask(void *godParameters);