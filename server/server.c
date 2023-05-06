/* Skeleton code for Computer Networks 2022 @ LIACS */
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "asp.h"
#include "wave.h"

#define BUFFER_SIZE 2000

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
    int socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    return socket_desc;
}


struct sockaddr_in set_addr(){
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1235);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    return addr;
}

void bind_addr_to_port(int socket_desc, struct sockaddr_in addr){
    printf("%d", socket_desc);

    if(bind(socket_desc, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}

struct sockaddr_in receive_message(int socket_desc, struct sockaddr_in sender_addr, char* sender_msg){
    int sender_struct_length = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if (recvfrom(socket_desc, buffer, BUFFER_SIZE, 0,
         (struct sockaddr*)&sender_addr, &sender_struct_length) < 0){
        printf("Couldn't receive\n");
        exit(EXIT_FAILURE);
    }
    printf("Received message from IP: %s and port: %i\n",
           inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));
    strncpy(sender_msg, buffer, BUFFER_SIZE);
    return sender_addr;
}

void send_message(int socket_desc, struct sockaddr_in receiver_addr, char* msg){
    int receiver_struct_length = sizeof(receiver_addr);
    if (sendto(socket_desc, msg, strlen(msg), 0,
         (struct sockaddr*)&receiver_addr, receiver_struct_length) < 0){
        perror("Error: ");
        return -1;
    }
}

void close_socket(int socket_desc){
    close(socket_desc);
}


int main(int argc, char** argv) {
    //Setup UDP
    int socket_desc = setup_udp_socket(socket_desc); // Create UDP socket:
    struct sockaddr_in server_addr = set_addr(server_addr); // Set port and IP:
    bind_addr_to_port(socket_desc, server_addr); // Bind to the set port and IP:
    
    //listen


    //printf("Listening for incoming messages...\n\n\0");


    do{
        printf("Listening...\n");
        // Receive client's message:
    
        char client_message[2000];
        clean_buffer(client_message);
        struct sockaddr_in client_addr;
        client_addr = receive_message(socket_desc, client_addr, client_message);
        printf("Msg from client: %s\n", client_message);

        
        // Respond to client:
        const char* filename = argv[1];
        send_message(socket_desc, client_addr, filename);
    }while(1);
    
    
    
    // Close the socket:
    close_socket(socket_desc);

    return EXIT_SUCCESS;
}
