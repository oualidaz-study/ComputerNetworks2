/* Skeleton code for Computer Networks 2022 @ LIACS */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "player.h"
#include "wave.h"
#include "player.h"
#include "asp.h"

#define BIND_PORT 1235
#define BUFFER_SIZE 2000


int wave_play(int argc, char** argv){
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
}

void clean_buffer(char* message){
    memset(message, '\0', sizeof(message));
}

int setup_udp_socket(){
    int socket_desc = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    return socket_desc;
}

struct sockaddr_in6 set_addr(){
    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(1234);
    inet_pton(AF_INET6, "::1", &addr.sin6_addr);
    return addr;
}

void bind_addr_to_port(int socket_desc, struct sockaddr_in addr){
    printf("%d", socket_desc);

    if(bind(socket_desc, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
    printf("Done with binding\n");
}

void receive_message(int socket_desc, struct sockaddr_in6 sender_addr, char* sender_msg){
    int sender_struct_length = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if (recvfrom(socket_desc, buffer, BUFFER_SIZE, 0,
         (struct sockaddr*)&sender_addr, &sender_struct_length) < 0){
        printf("Couldn't receive\n");
        exit(EXIT_FAILURE);
    }
    char str1[INET_ADDRSTRLEN];
    printf("Received message from IP: %s and port: %i\n",
           inet_ntop(AF_INET, &sender_addr.sin6_addr, str1, INET_ADDRSTRLEN), ntohs(sender_addr.sin6_port));
    strncpy(sender_msg, buffer, BUFFER_SIZE);
}

void send_message(int socket_desc, struct sockaddr_in6 receiver_addr, char* msg){
    
    int receiver_struct_length = sizeof(receiver_addr);
    printf("1\n");
    if (sendto(socket_desc, msg, strlen(msg), 0,
         (struct sockaddr*)&receiver_addr, receiver_struct_length) < 0){
        perror("Error: ");
        return -1;
    }
    printf("2\n");
}

void connect_to_server(int socket_desc, struct sockaddr_in6 server_addr){
    if(connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error: ");
        exit(0);
    }
}

void close_socket(int socket_desc){
    close(socket_desc);
}

/* Signal handler for stopping playback */
sig_atomic_t stop = 0;
static void sigint_handler(int sig) {
    (void) sig;
    stop = 1;
}

char* join_server(struct asp_socket_info* asp){
    

    //connect
    inet_pton(AF_INET6, "::1", &asp->remote_addr.sin6_addr);
    asp->remote_addr.sin6_port = htons(1235);
    asp->remote_addr.sin6_family = AF_INET6;

    connect_to_server(asp->sockfd, asp->remote_addr);
    printf("connected\n");
    // Get input from the user:
    char client_message[2000] = "Hi Server\0";
    // Send the message to server:
    printf("talking to server\n");
    send_message(asp->sockfd, asp->remote_addr, client_message);


    // Receive the server's response:
    char* file_name = (char*)malloc(64000 * sizeof(char));
    if (file_name == NULL) {
        printf("Error: Failed to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    memset(file_name, '\0', 64000);

    printf("requesting server\n");
    receive_message(asp->sockfd, asp->remote_addr, file_name);
    
    printf("Server's response: %s\n", file_name);
    return file_name;
}

int main(int argc, char** argv) {
    
    //setup UDP
    struct asp_socket_info asp;
    asp.sockfd = setup_udp_socket(); // Create UDP socket:
    char* file_name = join_server(&asp);

    //______________________________________________________________________________

    struct wave_file wave;
    if (wave_open(&wave, file_name) != 0) {
        return EXIT_FAILURE;
    }

    const double duration = wave_duration(&wave);
    const double minutes = floor(duration / 60.);
    const double seconds = round(fmod(duration, 60.));
    const int minutes_digits = (minutes == 0 ? 0 : floor(log10(minutes))) + 1;

    /* Print some info */
    {
        printf("File:        %s\n",     file_name);
        printf("Channels:    %hu\n",    wave.fmt->channels);
        printf("Sample rate: %u\n",     wave.fmt->sample_rate);
        printf("Duration:    %.1f s\n", duration);
    }

    struct player player;
    if (player_open(&player, wave.fmt->channels, wave.fmt->sample_rate) != 0) {
        wave_close(&wave);
        return EXIT_FAILURE;
    }

    /* Install signal handler to cancel playback */
    struct sigaction sighandler = {
        .sa_handler = sigint_handler,
        .sa_flags = 0
    };
    sigemptyset(&sighandler.sa_mask);
    sigaction(SIGINT, &sighandler, NULL);

    /* Current position we're reading audio data from */
    uint8_t* cur = wave.data->data;

    /* Total number of bytes played */
    size_t played = 0;
    while (played < wave.data->riff.chunk_size && stop == 0) {
        /* 1 frame is 1 sample in each channel, so for stereo 16-bit this is 4 bytes */
        const size_t frames_to_play = 4096;
        size_t bytes_to_play = frames_to_play * wave.fmt->channels * sizeof(int16_t);

        /* Clamp to max size */
        if ((played + bytes_to_play) > wave.data->riff.chunk_size) {
            bytes_to_play = wave.data->riff.chunk_size - played;
        }

        /* Queue the frames */
        player_queue(&player, cur, bytes_to_play);

        /* Bookkeeping */
        played += bytes_to_play;
        cur += bytes_to_play;

        /* Print progress */
        {
            double percent = ((double)played) / wave.data->riff.chunk_size;
            double passed = duration * percent;
            double m_passed = floor(passed / 60.);
            double s_passed = round(fmod(passed, 60.));

            int printed = printf("\r| %*.f:%02.f / %.f:%.f | %3.f%% ", minutes_digits, m_passed, s_passed,
                                 minutes, seconds, percent * 100.);
            /* Ignore the carriage return */
            printed -= 1;

            /* Get terminal size */
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

            /* 2 characters on both sides for borders */
            int bar_width = (int)w.ws_col - 4 - printed;
            int cols_filled = round(bar_width * percent);

            /* Print the progress bar */
            printf("| ");
            for (int i = 0; i < cols_filled; ++i) {
                printf("#");
            }

            for (int i = 0; i < (bar_width - cols_filled); ++i) {
                printf(" ");
            }

            printf(" |");

            /* Ensure our progress bar gets updated nice and fast */
            fflush(stdout);
        }

        /* 
         * Wait until there's 1/8th second of audio remaining in the buffer,
         * this is effectively our ctrl+c latency
         */
        player_wait_for_queue_remaining(&player, 0.125);
    }

    printf("\n");

    player_close(&player);
    wave_close(&wave);

    // Close the socket:
    close(asp.sockfd);

    return EXIT_SUCCESS;
}
