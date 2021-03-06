//
// Created by Elijah on 19/12/2017.
//
#include "test_kernels.h"

boolean test_kernels(cl_device_id device, cl_context context, boolean verbose) {
    cl_int ret;

    if (verbose) printf("\nTesting compiling kernels.\n");

    // Source file variables
    char *fileNames[] = {
            "../kernels/iterate_particle.cl",
            "../kernels/assign_particles.cl",
            "../kernels/assign_particles.cl",
            "../kernels/make_pp_collisions.cl",
            "../kernels/make_pp_collisions.cl",
//        "../kernels/make_pw_collisions.cl",
            "../kernels/calculate_pp_collision.cl",
            "../kernels/calculate_pw_collision.cl",
    };

    char *kernelNames[] = {
            "iterate_particle",
            "assign_particle_count",
            "assign_particles",
            "count_pp_collisions",
            "make_pp_collisions",
//        "make_pw_collisions",
            "calculate_pp_collision",
            "calculate_pw_collision",
    };
    int files = 7;

    for (int i = 0; i < files; i++) {

        if (verbose) {
            printf("\nKernel file: %s\n", fileNames[i]);
            printf("Kernel name: %s\n", kernelNames[i]);
        }

        cl_kernel kernel = getKernelWithUtils(device, context, fileNames[i], kernelNames[i], verbose);
        if (kernel == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}
