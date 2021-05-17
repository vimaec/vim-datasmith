#ifndef __CEULER_H__
#define __CEULER_H__

class 
    cEuler
{
public:
    union {
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            float pitch;
            float yaw;
            float roll;
        };
        struct {
            float attitude, heading, bank;
        };
    };
    inline cEuler()                                : x(0.0), y(0.0), z(0.0) {}
    inline cEuler(const cEuler &e)                 : x(e.x), y(e.y), z(e.z) {}
    inline cEuler(float inx, float iny, float inz) : x(inx), y(iny), z(inz) {}
    inline ~cEuler() {}

    inline void Set (float inx, float iny, float inz) { x = inx; y = iny; z = inz; }

};

#endif