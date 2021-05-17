#ifndef __Math_PlasticNumbers_h__
#define __Math_PlasticNumbers_h__

namespace Math
{
    static float PlasticNumber1(uint32_t index)
    {
        double g = 1.6180339887498948482;
        double a1 = 1.0 / g;
        float i;
        return modf((float)(0.5 + a1 * index), &i);
    }

    static cVec2 PlasticNumber2(uint32_t index)
    {
        const double p1 = 0.7548776662466927; // inverse of plastic number
        const double p2 = 0.5698402909980532;

        float i;
        return cVec2(modf((float)(0.5 + p1 * index), &i), modf((float)(0.5 + p2 * index), &i));
    }
    static cVec3 PlasticNumber3(uint32_t index)
    {
        double g = 1.22074408460575947536;
        double a1 = 1.0 / g;
        double a2 = 1.0 / (g * g);
        double a3 = 1.0 / (g * g * g);
        double x = (0.5 + a1 * (double)index);
        double y = (0.5 + a2 * (double)index);
        double z = (0.5 + a3 * (double)index);

        float i;
        return cVec3(modf((float)x, &i), modf((float)y, &i), modf((float)z, &i));
    }
}

#endif
