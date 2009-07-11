#include "pqiindic.h"

Indicator::Indicator(uint16_t n = 1) :
        num(n),
        changeFlags(n)
{
    IndicateChanged();
}

void Indicator::IndicateChanged()
{
    for (uint16_t i = 0; i < num; i++)
        changeFlags[i]=true;
}

bool Indicator::Changed(uint16_t idx = 0)
{
    /* catch overflow */
    if (idx > num - 1)
        return false;

    bool ans = changeFlags[idx];
    changeFlags[idx] = false;
    return ans;
}

