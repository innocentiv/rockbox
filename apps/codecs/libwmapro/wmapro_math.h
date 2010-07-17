#ifndef _WMAPRO_MATH_H_
#define _WMAPRO_MATH_H_

#include <inttypes.h>

#define fixtof16(x)       (float)((float)(x) / (float)(1 << 16))
#define fixtof31(x)       (float)((float)(x) / (float)(1 << 31))
#define ftofix16(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5:0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5:0.5)))

static inline int32_t fixmulshift(int32_t x, int32_t y, int shamt)
{
    int64_t temp;
    temp = x;
    temp *= y;

    temp >>= shamt;

    return (int32_t)temp;
}


static inline void vector_fixmul_window(int32_t *dst, const int32_t *src0, 
                                   const int32_t *src1, const int32_t *win, 
                                   int32_t add_bias, int len)
{
    int i, j;
    dst += len;
    win += len;
    src0+= len;
        for(i=-len, j=len-1; i<0; i++, j--) {
        int32_t s0 = src0[i];
        int32_t s1 = src1[j];
        int32_t wi = win[i];
        int32_t wj = win[j];
        dst[i] = fixmulshift(s0,-1*wj,31) - fixmulshift(s1,-1*wi,31) + (add_bias<<16);
        dst[j] = fixmulshift(s0,-1*wi,31) + fixmulshift(s1,-1*wj,31) + (add_bias<<16);
    }   
    
}

static inline void vector_fixmul_scalar(int32_t *dst, const int32_t *src, int32_t mul,
                                        int len, int shift)
{
    int i;
    for(i=0; i<len; i++)
        dst[i] = fixmulshift(src[i],mul,shift);   
}

static inline int av_clip(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}
#endif /* _WMAPRO_MATH_H_ */