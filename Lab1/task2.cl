__kernel void task2(__global int *input, __global int *output) {
    int id = get_global_id(0);
    output[id] = input[id] + id;
}
