
__kernel void task1() {
    printf("I am from %d block, %d thread (global index: %d)\n", get_group_id(0), get_local_id(0), get_global_id(0));
}
