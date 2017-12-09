//
// Created by Elijah on 30/11/2017.
//

#ifndef DEMORANGES_PARTICLE_H
#define DEMORANGES_PARTICLE_H

#include <CL/cl_platform.h>

typedef struct {
    cl_float3 pos;
    cl_float3 vel;
    cl_float3 forces;
    cl_ulong particle_id;
    cl_float particle_diameter;
    cl_float density;
    cl_float fluid_viscosity;
    cl_char padding[56];
// Structure memory alignment for Visual Studio and GCC compilers.
#if defined(_MSC_VER)
} __declspec(align(128)) particle;
#elif defined(__GNUC) || defined(__GNUG__)
} __attribute__((aligned (128))) particle;
#endif

// TODO: Pack or align this correctly. https://stackoverflow.com/questions/35167229/how-to-use-user-defined-structure-in-opencl

#endif //DEMORANGES_PARTICLE_H
