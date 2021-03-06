float get_particle_overlap(particle p1, particle p2) {
    return p1.diameter / 2 + p2.diameter / 2 - distance(p1.pos, p2.pos);
}

float get_particle_effect_overlap(particle p1, particle p2) {
    return p1.effect_diameter / 2 + p2.effect_diameter / 2 - distance(p1.pos, p2.pos);
}

float3 get_collision_normal(particle p1, particle p2) {
    return normalize(p2.pos - p1.pos);
}

float3 get_normal_velocity(particle p1, particle p2) {
    float3 normal = get_collision_normal(p1, p2);
    return dot(p2.vel - p1.vel, normal) * normal;
}

float3 get_tangent_velocity(particle p1, particle p2) {
    return p2.vel - p1.vel - get_normal_velocity(p1, p1);
}

float3 get_collision_tangent(particle p1, particle p2) {
    return p2.vel - p1.vel - get_normal_velocity(p1, p2);
}

float3 calculate_collision_normal_force(pp_collision col, particle p1, particle p2, float stiffness,
                                        float damping_coefficient, float overlap) {
    float3 force = stiffness * overlap * get_collision_normal(p1, p2)
                    - damping_coefficient * get_normal_velocity(p1, p2);
    return force;
}

float3 calculate_friction_tangent_force(pp_collision col, particle p1, particle p2, float3 normal_force, float delta_t,
                                        float friction_coefficient, float friction_stiffness, float stiffness) {
    float3 f_dyn = - friction_coefficient * length(normal_force) * get_collision_tangent(p1, p2);

    float tang_displacement = length(get_tangent_velocity(p1, p2) * M_PI_F * sqrt(get_reduced_particle_mass(get_particle_mass(p1), get_particle_mass(p2)) / stiffness));
    float3 f_static = - friction_stiffness * tang_displacement * get_collision_tangent(p1, p2);

    if (fast_length(f_dyn) < fast_length(f_static)) {
        return f_dyn;
    } else {
        return f_static;
    }
}

float3 calculate_cohesion_force(particle p1, particle p2, float cohesion_stiffness, float effect_overlap) {
    return cohesion_stiffness * effect_overlap * get_collision_normal(p1, p2);
}

/* Kernel to calculate particle-particle collisions. */

__kernel void calculate_pp_collision(__global pp_collision *collisions, __global particle *particles, float delta_t,
                                        float stiffness, float restitution_coefficient,
                                        float friction_coefficient, float friction_stiffness,
                                        float cohesion_stiffness, float domain_length) {
    int gid = get_global_id(0);
    ulong p1_id = collisions[gid].p1_id;
    ulong p2_id = collisions[gid].p2_id;

    particle p1 = particles[p1_id];
    particle p2 = particles[p2_id];

    p1.pos.x -= domain_length * collisions[gid].cross_boundary_x;
    p1.pos.y -= domain_length * collisions[gid].cross_boundary_y;
    p1.pos.z -= domain_length * collisions[gid].cross_boundary_z;

    float overlap = get_particle_overlap(p1, p2);
    float effect_overlap = get_particle_effect_overlap(p1, p2);
    float3 forces = (float3) {0, 0, 0};
    if (overlap > 0) {
        float reduced_particle_mass = get_reduced_particle_mass(get_particle_mass(p1),
                                                                get_particle_mass(p2));
        float damping_coefficient = get_damping_coefficient(restitution_coefficient, stiffness, reduced_particle_mass);
        float3 normal_force = calculate_collision_normal_force(collisions[gid], p1, p2,
                                                                stiffness, damping_coefficient, overlap);
        float nf = fast_length(normal_force);
        if (nf > 10000) {
            printf("force = %f, stiff = %f, overlap = %f, damping_coeff = %f, vel = %f\n", nf, stiffness, overlap, damping_coefficient, fast_length(get_normal_velocity(p1, p2)));
        }
        float3 tangent_force = calculate_friction_tangent_force(collisions[gid], p1, p2,
                                                                normal_force, delta_t, friction_coefficient,
                                                                friction_stiffness, stiffness);
        forces += normal_force + tangent_force;
    }
    if (effect_overlap > 0) {
        float3 cohesion_force = calculate_cohesion_force(p1, p2, cohesion_stiffness, effect_overlap);
        forces -= cohesion_force;  // Pulling rather than pushing.
    }
    if (overlap > 0 || effect_overlap > 0){
        if (overlap > 3 * p1.diameter / 4 || overlap > 3 * p2.diameter / 4) {
            printf("Warning: Excessive particle overlap (%f), p1: (%f, %f, %f), p2: (%f, %f, %f)\n", overlap,
                    p1.pos.x, p1.pos.y, p1.pos.z,
                    p2.pos.x, p2.pos.y, p2.pos.z);
        }
        atomicAdd_float3(&particles[p1_id].forces, - forces);
        atomicAdd_float3(&particles[p2_id].forces, forces);
    }
}