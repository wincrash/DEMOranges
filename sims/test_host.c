#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else

#include <CL/cl.h>

#endif

#include "../util/clUtils.h"
#include "../util/particleUtils.h"
#include "../util/collisionUtils.h"
#include "../structures/particle.h"
#include "../structures/collision.h"

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

particle *hparticles;
cl_mem gparticles;
cl_ulong NUMPART = 1000;

pp_collision *hpp_cols;
cl_mem gpp_cols;
cl_ulong MAXCOLS;
cl_ulong NUMCOLS;

cl_float timestep = 1e-4;
cl_float sim_length = 10;
cl_float last_write = 0;
cl_float log_step = 0.0333;

cl_platform_id *platforms;
cl_device_id *devices;
cl_int ret;

int main() {

    // Initializing OpenCL.
    setDevices(&platforms, &devices, TRUE);
    cl_context context = getContext(&devices, TRUE);
    cl_kernel iterate_particle = getKernel(&devices, &context, "../kernels/iterate_particle.cl",
                                           "iterate_particle", TRUE);
    cl_kernel calculate_pp_collision = getKernel(&devices, &context, "../kernels/calculate_pp_collision.cl",
                                                 "calculate_pp_collision", TRUE);
    cl_command_queue queue = getCommandQueue(&context, &devices, TRUE);

    hparticles = malloc(sizeof(particle) * NUMPART);
    cl_float density = 2000;
    cl_float particle_diameter = 0.01;
    cl_float fluid_viscosity = 0.0000193;

    cl_float3 *positions = malloc(sizeof(cl_float3) * NUMPART);
    cl_ulong pos_len = 0;
    auto cubert_NUMPART = (cl_ulong) ceil(pow(NUMPART, 0.334));
    for (int x = 0; x < cubert_NUMPART; x++) {
        for (int y = 0; y < cubert_NUMPART; y++) {
            for (int z = 0; z < cubert_NUMPART; z++) {
                if (pos_len < NUMPART) {
                    cl_float xf = 0.15 * (-0.5 + ((float) x / cubert_NUMPART) + 0.01);
                    cl_float yf = 0.15 * (-0.5 + ((float) y / cubert_NUMPART) + 0.01);
                    cl_float zf = 0.15 * (-0.5 + ((float) z / cubert_NUMPART) + 0.01);
                    cl_float xf = 1.2 * cubert_NUMPART * particle_diameter * (-0.5 + ((float) x / cubert_NUMPART));
                    cl_float yf = 1.2 * cubert_NUMPART * particle_diameter * (-0.5 + ((float) y / cubert_NUMPART));
                    cl_float zf = 1.2 * cubert_NUMPART * particle_diameter * (-0.5 + ((float) z / cubert_NUMPART));
                    positions[pos_len] = (cl_float3) {xf, yf, zf};
                }
                pos_len++;
            }
        }
    }

    for (cl_ulong i = 0; i < NUMPART; i++) {
        hparticles[i].id = i;
        hparticles[i].density = density;
        hparticles[i].fluid_viscosity = fluid_viscosity;
        hparticles[i].diameter = particle_diameter;
        hparticles[i].pos = positions[i];
        hparticles[i].vel = (cl_float3) {0.0, 0.0, 0.0};
        hparticles[i].forces = (cl_float3) {0.0, 0.0, 0.0};
    }

    MAXCOLS = (cl_ulong) ceil(NUMPART * (NUMPART + 1) / 2);
    hpp_cols = malloc(sizeof(pp_collision) * MAXCOLS);

    cl_float stiffness = 5e4;
    cl_float friction_coefficient = 0.6;
    cl_float friction_stiffness = 1e5;

    NUMCOLS = 0;
    for (cl_ulong i = 0; i < NUMPART; i++) {
        for (cl_ulong j = i + 1; j < NUMPART; j++) {
            hpp_cols[NUMCOLS].p1_id = i;
            hpp_cols[NUMCOLS].p2_id = j;
            hpp_cols[NUMCOLS].stiffness = stiffness;
            hpp_cols[NUMCOLS].friction_coefficient = friction_coefficient;
            hpp_cols[NUMCOLS].friction_stiffness = friction_stiffness;
            hpp_cols[NUMCOLS].damping_coefficient = get_damping_coefficient(0.1, 5e4, get_reduced_particle_mass(
                    &hparticles[i], &hparticles[j]));
            NUMCOLS++;
        }
    }

    writeParticles(hparticles, 0, "TEST", "", NUMPART);

    gparticles = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(particle) * NUMPART, NULL, &ret);
    ret = clSetKernelArg(iterate_particle, 0, sizeof(cl_mem), &gparticles);
    ret = clSetKernelArg(iterate_particle, 1, sizeof(cl_float), &timestep);

    gpp_cols = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(pp_collision) * NUMCOLS, NULL, &ret);
    ret = clSetKernelArg(calculate_pp_collision, 0, sizeof(cl_mem), &gpp_cols);
    ret = clSetKernelArg(calculate_pp_collision, 1, sizeof(cl_mem), &gparticles);
    ret = clSetKernelArg(calculate_pp_collision, 2, sizeof(cl_float), &timestep);

    ret = particlesToDevice(queue, gparticles, &hparticles, NUMPART);
    ret = pp_collisionsToDevice(queue, gpp_cols, &hpp_cols, NUMCOLS);

    for (cl_float time = timestep; time <= sim_length; time += timestep) {
        ret = clEnqueueNDRangeKernel(queue, calculate_pp_collision, 1, NULL, (size_t *) &NUMCOLS, 0, NULL, NULL, NULL);
        ret = clFinish(queue);
        ret = clEnqueueNDRangeKernel(queue, iterate_particle, 1, NULL, (size_t *) &NUMPART, 0, NULL, NULL, NULL);
        ret = clFinish(queue);
        if (time - last_write > log_step) {
            ret = particlesToHost(queue, gparticles, &hparticles, NUMPART);
            printf("Logging at time: %f\n", time);
            writeParticles(hparticles, time, "TEST", "", NUMPART);
            last_write = time;
        }
    }


}