#ifndef ERR_HPP
#define ERR_HPP

/* Libfabric libraries. */
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>

#include <iostream>
#include <cstdlib>

/* Perform error checking for libfabric functions. */
void check_libfabric(int code, const char* message);

/* Perform error checking for specific event queues. */
void check_eq_error(fid_eq* event_queue);

#endif /* ERR_HPP */
