#pragma once

#include <stdlib.h>

struct InterconnectParameters
{
    struct CommsParameters *commsParameters;
};

void vInterconnectTask(void *interconnectParameters);