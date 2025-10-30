#ifndef NET_H
#define NET_H

/* Libfabric libraries. */
#include <rdma/fabric.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>

/* Sockets and socket-related libraries. */
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>

/* Base class for fabric configurations, designed to be inherited. */
class Fabric {
	public:
		/* Gather networking information to configure the network. */
		int fabric_init();

		/* Gracefully shutdown the network. */
		int fabric_finalize();

		void set_comm_mode(const bool mode) {
			comm_mode = mode;
		}

	protected:
		/* Perform error checking for libfabric functions. */
		void check_libfabric(int code, const char* message);

		/* Perform error checking for specific event queues. */
		void check_eq_error(fid_eq* event_queue);

		/* Point to the network config. structure. */
		fi_info* fabric = nullptr;

		/* Point towards the network provider. */
		fid_fabric* fabric = nullptr;

		/* Point towards the options for the completion queue. */
		fi_cq_attr completion_queue_attr = {};

		/* Point towards the event queue. */
		fid_eq* event_queue = nullptr;

		/* Point to the domain for endpoints. */
		fid_domain* domain = nullptr;

		/* Point to the active endpoint. */
		fid_ep* endpoint = nullptr;

		/* Point to the receiving and sending completion queues. */
		fid_cq* recv_queue = nullptr;
		fid_cq* transmit_queue = nullptr;

		/* The communication mode the user specified. */
		bool comm_mode = false;
};

/* Initialize and manage a libfabric server. */
class FabricServer : public Fabric {
	public:
		/* One class instance that lives for the duration of the program. */
		static FabricServer& instance() {
			static FabricServer instance;
			return instance;
		}

		/* Initialize and listen on a libfabric server. */
		int fabric_server(int port);

	private:
		/* Prevent anyone outside the class from created another instance. */
		FabricServer() = default;

		/* Point to the passive endpoint. */
		fid_pep* passive_endpoint = nullptr;
};

/* Initialize and manage a libfabric client. */
class FabricClient : public Fabric {
	public:
		/* One class instance that lives for the duration of the program. */
		static FabricClient& instance() {
			static FabricClient instance;
			return instance;
		}

		/* Initialize and use a libfabric client. */
		int fabric_client(const char* dest_addr, int dest_port);

	private:
		/* Prevent anyone outside the class from creating another instance. */
		FabricClient() = default;
};

#endif /* NET_H */	
