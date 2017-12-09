#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else

#include <CL/cl.h>

#endif

#include "../util/clUtils.h"
#include "../structures/particle.h"

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

particle *hparticles;
cl_mem gparticles;
int NUMPART = 10;

cl_platform_id *platforms;
cl_device_id *devices;
cl_int ret;

int main() {

    // Initializing OpenCL.
    setDevices(&platforms, &devices);
    cl_context context = getContext(&devices);
    cl_kernel kernel = getKernel(&devices, &context, "../kernels/iterate_particle.cl", "iterate_particle");
    cl_command_queue queue = getCommandQueue(&context, &devices);

    hparticles = malloc(sizeof(particle) * NUMPART);
    cl_float density = 2000;
    cl_float particle_diameter = 0.1;
    cl_float fluid_viscosity = 0.0000193;

    for (cl_ulong i = 0; i < NUMPART; i++) {
        hparticles[i].particle_id = i;
        hparticles[i].density = density;
        hparticles[i].fluid_viscosity = fluid_viscosity;
        hparticles[i].particle_diameter = particle_diameter;
        hparticles[i].pos = (cl_float3) {0.0, i, 0.0};
        hparticles[i].vel = (cl_float3) {0.0, 0.0, 0.0};
        hparticles[i].forces = (cl_float3) {0.0, 0.0, 0.0};
    }

    gparticles = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(particle) * NUMPART, NULL, &ret);

    ret = clEnqueueWriteBuffer(queue, gparticles, CL_TRUE, 0, sizeof(particle) * NUMPART, hparticles, 0, NULL, NULL);
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gparticles);
    ret = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, (size_t *) &NUMPART, 0, NULL, NULL, NULL);
    ret = clEnqueueReadBuffer(queue, gparticles, CL_TRUE, 0, sizeof(particle) * NUMPART, hparticles, 0, NULL, NULL);
    ret = clFinish(queue);

    for (cl_ulong i = 0; i < NUMPART; i++) {
        printf("%f\n", hparticles[i].pos.y);
    }
//    particle p = hparticles[0];
//    printf("%i\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n", (int) p.particle_id, p.particle_diameter, p.density,
//           p.fluid_viscosity, p.pos.x, p.pos.y, p.pos.z, p.vel.x, p.vel.y, p.vel.z, p.forces.x, p.forces.y, p.forces.z);
}