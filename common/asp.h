#include <math.h>


/*
 * An asp socket descriptor for information about the sockets current state.
 * This is just an example, you can make anything you want out of it
 */
struct asp_socket_info {
    int sockfd;

    struct sockaddr_in6 local_addr;
    socklen_t local_addrlen;

    struct sockaddr_in6 remote_addr;
    socklen_t remote_addrlen;

    struct asp_socket_info *next;

    int current_quality_level;
    uint32_t sequence_count;

    size_t packets_received;
    size_t packets_missing;

    struct timeval last_packet_received;
    struct timeval last_problem;

    unsigned int is_connected : 1;
    unsigned int has_accepted : 1;
};

struct asp_packet
{
    
    /* data */
};
