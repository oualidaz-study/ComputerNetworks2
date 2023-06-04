/* Skeleton code for Computer Networks 2022 @ LIACS */
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "asp.h"
#include "wave.h"

#define BUFFER_SIZE 2000
#define MAX_PACKET_SIZE 65507

static int asp_socket_fd = -1;


void wave_play(int argc, char** argv){
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
    return socket_desc;
}


struct sockaddr_in6 set_addr(){
    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(1235);
    inet_pton(AF_INET6, "::1", &addr.sin6_addr);
    return addr;
}

void bind_addr_to_port(int socket_desc, struct sockaddr_in6 addr){
    printf("%d", socket_desc);

    if(bind(socket_desc, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}

struct sockaddr_in6 receive_message(int socket_desc, struct sockaddr_in6 sender_addr, char* sender_msg){
    int sender_struct_length = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    printf("0\n");
    if (recvfrom(socket_desc, buffer, BUFFER_SIZE, 0,
         (struct sockaddr*)&sender_addr, &sender_struct_length) < 0){
        printf("Couldn't receive\n");
        exit(EXIT_FAILURE);
    }
    printf("1\n");
    char str1[INET_ADDRSTRLEN];
    printf("2\n");
    printf("Received message from IP: %s and port: %i\n",
           inet_ntop(AF_INET, &sender_addr.sin6_addr, str1, INET_ADDRSTRLEN), ntohs(sender_addr.sin6_port));
    strncpy(sender_msg, buffer, BUFFER_SIZE - 1);
    sender_msg[BUFFER_SIZE - 1] = '\0';  // Ensure the destination string is null-terminated
    return sender_addr;
}


uint8_t* get_package(void* data, uint16_t data_length, in_port_t source_port, in_port_t destination_port){
    void* package = malloc(sizeof(struct RFC768Header) + data_length);

    struct RFC768Header* header = (struct RFC768Header*)package;
    header->source_port = source_port;                 // Convert to network byte order
    header->destination_port = destination_port;       // Convert to network byte order
    header->length = htons(sizeof(struct RFC768Header) + data_length);
    header->checksum = 0;                                      // Calculate and set checksum later

    memcpy(package + sizeof(struct RFC768Header), data, data_length);

    //header->checksum = calculateChecksum(header, sizeof(struct RFC768Header) + data_length);

    return (uint8_t*)package;
}

void send_message(struct asp_socket_info asp, void* data, uint16_t data_length){
    printf("a\n");
    uint8_t* package = get_package(data, data_length, asp.local_addr.sin6_port, asp.remote_addr.sin6_port);
    printf("b\n");
    size_t package_size = sizeof(struct RFC768Header) + data_length;
    printf("%d\n", package_size);
    if (sendto(asp.sockfd, package, package_size, 0,
         (struct sockaddr*)&asp.remote_addr, asp.remote_addrlen) < 0){
        perror("Error: ");
    }
    printf("d\n");
}

void close_socket(int socket_desc){
    close(socket_desc);
}


struct wave_info get_wave_data(char* filename, struct wave_file wave){
    struct wave_info w;
    w.channels = wave.fmt->channels;
    w.sample_rate = wave.fmt->sample_rate;
    w.duration = wave_duration(&wave);
    w.cur = wave.data->data;
    w.max = wave.data->riff.chunk_size;
    return w;
}

int send_audio_over_udp(struct wave_file* wave, const struct sockaddr_in6* destAddr, int udpSocket) {
    // Calculate the total number of packets
    size_t totalPackets = (wave->data->riff.chunk_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;

    // Allocate memory for the packet buffer
    uint8_t* packet = malloc(MAX_PACKET_SIZE);
    if (packet == NULL) {
        printf("Failed to allocate memory for packet buffer\n");
        return 1;
    }

    // Split audio data into packets and send them over UDP
    size_t offset = 0;
    for (size_t packetIndex = 0; packetIndex < totalPackets; ++packetIndex) {
        // Calculate the size of the current packet
        size_t packetSize = (packetIndex == totalPackets - 1) ? (wave->data->riff.chunk_size - offset) : MAX_PACKET_SIZE;
        packetSize = (packetSize < MAX_PACKET_SIZE) ? packetSize : MAX_PACKET_SIZE;

        // Copy audio data into the packet buffer
        memcpy(packet, wave->data->data + offset, packetSize);

        // Send the packet over UDP
        ssize_t bytesSent = sendto(udpSocket, packet, packetSize, 0, (struct sockaddr*)destAddr, sizeof(*destAddr));
        if (bytesSent == -1) {
            printf("Failed to send UDP packet\n");
            free(packet);
            return 1;
        }
        printf("Sending package: %zu/%zu.\n", packetIndex + 1, totalPackets);

        // Wait for acknowledgment from the client
        uint8_t ack;
        ssize_t bytesReceived = recvfrom(udpSocket, &ack, sizeof(ack), 0, NULL, NULL);
        if (bytesReceived == -1) {
            printf("Failed to receive acknowledgment\n");
            free(packet);
            return 1;
        }

        // Update the offset
        offset += packetSize;
    }

    // Send the "end of transmission" packet
    uint8_t endPacket = 0xFF;
    ssize_t bytesSent = sendto(udpSocket, &endPacket, sizeof(endPacket), 0, (struct sockaddr*)destAddr, sizeof(*destAddr));
    if (bytesSent == -1) {
        printf("Failed to send end of transmission packet\n");
        free(packet);
        return 1;
    }

    // Free the packet buffer
    free(packet);

    return 0;
}



int main(int argc, char** argv) {
    const char* filename = argv[1];
    struct asp_socket_info asp;


    //Setup UDP
    asp.sockfd = setup_udp_socket(); // Create UDP socket:
    asp.local_addr = set_addr();
    bind_addr_to_port(asp.sockfd, asp.local_addr); // Bind to the set port and IP:
    
    struct wave_file wave;
    if (wave_open(&wave, filename) != 0) {
        return EXIT_FAILURE;
    }

    const double duration = wave_duration(&wave);
    const double minutes = floor(duration / 60.);
    const double seconds = round(fmod(duration, 60.));
    const int minutes_digits = (minutes == 0 ? 0 : floor(log10(minutes))) + 1;

    /* Print some info */
    {
        printf("File:        %s\n",     filename);
        printf("Channels:    %hu\n",    wave.fmt->channels);
        printf("Sample rate: %u\n",     wave.fmt->sample_rate);
        printf("Duration:    %.1f s\n", duration);
    }

    
    //listen
    do{
        // Receive client's message:
        char client_message[2000];
        clean_buffer(client_message);
        asp.remote_addr = receive_message(asp.sockfd, asp.remote_addr, client_message);
        asp.remote_addrlen = sizeof(asp.remote_addr);
        printf("Msg from client: %s\n", client_message);

        //Initial Message:
        printf("1\n");
        struct wave_info info = get_wave_data(filename, wave);
        printf("2\n");
        printf("Server's response: %d\n", info.channels);
        printf("1\n");
        send_message(asp, &info, sizeof(info));

        // Call the function to send audio over UDP
        if (send_audio_over_udp(&wave, &asp.remote_addr, asp.sockfd) != 0) {
            printf("Failed to send audio over UDP\n");
            close_socket(asp.sockfd);;
            wave_close(&wave);
            return 1;
        }
    }while(1);
    
    // Close the socket:
    close_socket(asp.sockfd);

    return EXIT_SUCCESS;
}
