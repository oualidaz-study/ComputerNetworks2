/* Skeleton code for Computer Networks 2022 @ LIACS */
#include <stdlib.h>

#include "asp.h"
#include "wave.h"

static int asp_socket_fd = -1;

int main(int argc, char** argv) {
    // TODO: Parse command-line options 
    (void) argc;
    const char* filename = argv[1];

    // Open the WAVE file
    struct wave_file wave;
    if (wave_open(&wave, filename) < 0) {
        return EXIT_FAILURE;
    }

    // TODO: Read and send audio data
    (void) asp_socket_fd;

    // Clean up
    wave_close(&wave);

    return EXIT_SUCCESS;
}
