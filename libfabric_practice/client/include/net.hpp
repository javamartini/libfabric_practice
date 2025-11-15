#ifndef NET_HPP
#define NET_HPP

/* Libfabric libraries. */
#include <rdma/fabric.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_cm.h>

/* Socket and socket-related libraries. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <vector>

/* Initialize and use a libfabric client. */
int client(const char* dest_addr, int dest_port);

#endif /* NET_HPP */
