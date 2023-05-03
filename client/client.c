/* Skeleton code for Computer Networks 2022 @ LIACS */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include "player.h"

#define BIND_PORT 1235

int main(int argc, char** argv) {
    int buffer_size = 1024*1024;  // TODO: Get buffersize using commandline args.
    int bind_port = BIND_PORT;

    (void) argc;
    (void) argv;
    (void) bind_port;

    /*
     * Make sure the channel count and sample rate is correct!
     * Optional: support arbitrary parameters through your protocol.
     */
    struct player player;
    if (player_open(&player, 2, 44100) != 0) {
        return EXIT_FAILURE;
    }

    // TODO: Parse command-line options.

    // TODO: Set up network connection.


    // set up buffer/queue
    uint8_t* recvbuffer = malloc(buffer_size);

    // TODO: fill the buffer initially, before playing anything

    // Play
    printf("playing...\n");

    while (true) { //TODO: Stop requesting music when all music is already here.
        // TODO: Get ourselves some data, but only if our recvbuffer has room for the data that will be sent back.

        // ...

        //TODO: Send the data to SDL.
        player_queue(&player, recvbuffer, 42); // TODO: Make sure we have some nice music data in our recvbuffer. Also: Replace `42` by the number of bytes we want to hand over to SDL.
        // Now in the next while iteration, we need to reuse the buffer. If we go and free & re-malloc() the buffer every iteration, our implementation becomes inefficient.
    }

    // clean up
    free(recvbuffer);
    player_close(&player);

    return 0;
}
