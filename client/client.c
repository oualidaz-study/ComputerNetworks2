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
#include "asp.h"

#define BIND_PORT 1235
#define BUFFER_SIZE 1024*1024


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

uint8_t* extract_package(uint8_t *received_packet){
    struct RFC768Header* header = (struct RFC768Header*)received_packet;
    uint16_t source_port = ntohs(header->source_port);                 // Convert to host byte order
    uint16_t destination_port = ntohs(header->destination_port);       // Convert to host byte order
    uint16_t length = ntohs(header->length);                           // Convert to host byte order
    uint16_t checksum = ntohs(header->checksum);                       // Convert to host byte order
    uint16_t expected_data_length = length - sizeof(struct RFC768Header);
    uint8_t* data = received_packet + sizeof(struct RFC768Header);
    
    return data;
}


void receive_message(int socket_desc, struct sockaddr_in6 sender_addr, void** data){
    int sender_struct_length = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];
    if (recvfrom(socket_desc, buffer, BUFFER_SIZE, 0,
         (struct sockaddr*)&sender_addr, &sender_struct_length) < 0){
        printf("Couldn't receive\n");
        exit(EXIT_FAILURE);
    }
    char str1[INET_ADDRSTRLEN];
    printf("Received message from IP: %s and port: %i\n",
           inet_ntop(AF_INET, &sender_addr.sin6_addr, str1, INET_ADDRSTRLEN), ntohs(sender_addr.sin6_port));
    
    *data = extract_package(buffer);
}

void send_message(int socket_desc, struct sockaddr_in6 receiver_addr, char* msg){
    
    int receiver_struct_length = sizeof(receiver_addr);
    if (sendto(socket_desc, msg, strlen(msg), 0,
         (struct sockaddr*)&receiver_addr, receiver_struct_length) < 0){
        perror("Error: ");
        return -1;
    }
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

#include <unistd.h> // for usleep

void receive_and_play_audio(int socket_desc, struct player* player, size_t expected_song_size, double duration) {
    int buffer_size = 1048576;  // 1 megabyte buffer size

    // Allocate buffer for receiving audio data dynamically
    uint8_t* recv_buffer = malloc(buffer_size);
    if (recv_buffer == NULL) {
        printf("Failed to allocate receive buffer\n");
        return;
    }

    printf("Playing audio...\n");
    const double minutes = floor(duration / 60.);
    const double seconds = round(fmod(duration, 60.));
    const int minutes_digits = (minutes == 0 ? 0 : floor(log10(minutes))) + 1;

    uint8_t* cur = recv_buffer; // Initialize cur with recv_buffer
    size_t played = 0;
    ssize_t recv_len;
    int endTransmission = 0;
    /* 1 frame is 1 sample in each channel, so for stereo 16-bit this is 4 bytes */
    const size_t frames_to_play = 4096;
    const size_t bytes_per_frame = 2 * sizeof(int16_t);
    size_t bytes_to_play = frames_to_play * bytes_per_frame;
    // Receive audio data until the entire song is received or end of transmission is received
    int count_package = 0;
    while (played < expected_song_size && !endTransmission && stop == 0) {
        struct sockaddr_in6 sender_addr;
        socklen_t sender_struct_length = sizeof(sender_addr);

        // Adjust the buffer size if remaining size is smaller
        size_t remaining_size = expected_song_size - played;
        size_t receive_size = (remaining_size < buffer_size) ? remaining_size : buffer_size;

        // Receive audio data into the dynamically allocated buffer
        recv_len = recvfrom(socket_desc, recv_buffer, receive_size, 0,
                            (struct sockaddr*)&sender_addr, &sender_struct_length);
        if (recv_len < 0) {
            printf("Failed to receive audio data\n");
            break;
        }

        // Check if the received packet indicates end of transmission
        if (recv_len == 1 && recv_buffer[0] == 0xFF) {
            endTransmission = 1;
            continue;
        }

        // Send an acknowledgment to the server
        uint8_t ack = 0xFF;
        ssize_t bytesSent = sendto(socket_desc, &ack, sizeof(ack), 0,
                                   (struct sockaddr*)&sender_addr, sender_struct_length);
        if (bytesSent == -1) {
            printf("Failed to send acknowledgment\n");
            break;
        }
        count_package++;

        // Play the received audio data
        player_queue(player, recv_buffer, recv_len);
        /* Bookkeeping */
        played += recv_len;

        /* Print progress */
        {
            double percent = ((double)played) / expected_song_size;
            double passed = duration * percent;
            double m_passed = floor(passed / 60.);
            double s_passed = round(fmod(passed, 60.));

            char progressBar[1024];  // Adjust the size according to your needs
            int printed = snprintf(progressBar, sizeof(progressBar),
                "\r| %d | %*.f:%02.f / %.f:%.f | %3.f%% | ", count_package, minutes_digits,
                m_passed, s_passed, minutes, seconds, percent * 100.);

            // Calculate the number of '#' characters to fill in the progress bar
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            int bar_width = (int)w.ws_col - 4 - printed;
            int cols_filled = round(bar_width * percent);

            // Build the progress bar string
            for (int i = 0; i < cols_filled; ++i) {
                strncat(progressBar, "#", sizeof(progressBar) - strlen(progressBar) - 1);
            }

            for (int i = 0; i < (bar_width - cols_filled); ++i) {
                strncat(progressBar, " ", sizeof(progressBar) - strlen(progressBar) - 1);
            }

            strncat(progressBar, " |", sizeof(progressBar) - strlen(progressBar) - 1);

            // Print the progress bar
            printf("%s", progressBar);

            /* Ensure our progress bar gets updated nice and fast */
            fflush(stdout);
            // Introduce a delay to allow the player to catch up
            usleep(100000); // Delay for 100 milliseconds (adjust as needed)
        }

        /* 
         * Wait until there's 1/8th second of audio remaining in the buffer,
         * this is effectively our ctrl+c latency
         */
        player_wait_for_queue_remaining(&player, 0.125);
    }
    // Clean up
    free(recv_buffer);
}





struct wave_info* join_server(struct asp_socket_info* asp){
    

    //connect
    inet_pton(AF_INET6, "::1", &asp->remote_addr.sin6_addr);
    asp->remote_addr.sin6_port = htons(1235);
    asp->remote_addr.sin6_family = AF_INET6;
    asp->remote_addr.sin6_flowinfo = 26;
    asp->remote_addr.sin6_scope_id = 2;

    connect_to_server(asp->sockfd, asp->remote_addr);
    printf("connected\n");
    // Get input from the user:
    char client_message[2000] = "Hi Server\0";
    // Send the message to server:
    printf("talking to server\n");
    send_message(asp->sockfd, asp->remote_addr, client_message);


    // Receive the server's response:
    void* data = NULL; // Initialize data to NULL

    printf("requesting server\n");
    receive_message(asp->sockfd, asp->remote_addr, &data);

    if (data != NULL) {
        struct wave_info* info = malloc(sizeof(struct wave_info));
        if (info != NULL) {
            memcpy(info, data, sizeof(struct wave_info));
            return info;
        } else {
            printf("Memory allocation failed\n");
        }
        // Handle the data as needed
    } else {
        printf("Invalid data received\n");
        // Handle the case when data is NULL or not enough data is received
    }

    return NULL;
}

int main(int argc, char** argv) {
    
    // Setup UDP
    struct asp_socket_info asp;
    asp.sockfd = setup_udp_socket(); // Create UDP socket

    // Join the server and retrieve wave_info
    struct wave_info* info = join_server(&asp);
    if (info != NULL) {
        printf("Server's response: %d\n", info->channels);
        // Handle the wave_info as needed

        // Clean up the allocated wave_info
        
    } else {
        printf("Failed to receive wave_info\n");
        // Handle the case when joining the server fails
    }

    //______________________________________________________________________________


    const double duration = info->duration;
    const double minutes = floor(duration / 60.);
    const double seconds = round(fmod(duration, 60.));
    const int minutes_digits = (minutes == 0 ? 0 : floor(log10(minutes))) + 1;


    struct player player;

    printf("channels: %d, sample rate: %d\n", info->channels, info->sample_rate);
    if (player_open(&player, info->channels, info->sample_rate) != 0) {
        return EXIT_FAILURE;
    }

    // Connect to the server and receive audio data
    receive_and_play_audio(asp.sockfd, &player, info->max, info->duration);

    player_close(&player);

    //Close the socket:
    close(asp.sockfd);

    return EXIT_SUCCESS;
}
