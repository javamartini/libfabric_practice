#include "net.hpp"

/* Gather network information to configure the network. */
int Fabric::fabric_init(bool is_server) {
	/* Create a structure that holds the libfabric config. that
	 * is being requested. This structure will be used to request
	 * an actual structure to begin making the network. */
	fi_info* hints = fi_allocinfo();

	/* Request the type of network based upon the user input. */
	if (this-> comm_mode == true) {
		/* Request the ability to use the send/recv form of message passing. */
		hints->caps = FI_MSG | FI_SEND | FI_RECV;
	} /* TODO. Add esle block that specifies a one-way net. */

	/* Request a connection-oriented endpoint (TCP). */
	hints->ep_attr->type = FI_EP_MSG;

	/* Use the hinting structure to capture the real config.
	 * for the network.*/
	if (is_server == true) {
		const char* node = nullptr;
		const char* service = "8080";

		check_libfabric(fi_getinfo(FI_VERSION(1, 15), node, service, 
					FI_SOURCE, hints, &info), "fi_getinfo()");
	} else {
		check_libfabric(fi_getinfo(FI_VERSION(1, 15), 0, 0, 0, hints, &info),
				"fi_getinfo()");
	}

	/* Now that there is an actual config. from libfabric,
	 * free the 'hints' info struct. */
	fi_freeinfo(hints);

	/* Use the info gathered to create a libfabric network using the 
	 * available providers available to the OS. This represents
	 * a collection of resources such as domains, event queues,
	 * completion queues, endpoints, etc. */
	check_libfabric(fi_fabric(info->fabric_attr, &fabric, nullptr),
			"fi_fabric()");

	/* Set event queue attributes. An event queue stores the 
	 * asynchronous events made on the network. The wait setting is
	 * a mechanism that holds multiple waitable objects such as event queues,
	 * completion queues, counters, etc. */
	fi_eq_attr event_queue_attr = {
		.size = 10, /* The maximum number of events that can be queued. */
		.wait_obj = FI_WAIT_UNSPEC /* Use whatever wait obj. deemed needed. */
	};

	/* Create the event queue using the settings structure. */
	check_libfabric(fi_eq_open(fabric, &event_queue_attr, &event_queue, 0),
			"fi_eq_open()");

	/* Configure attributes of the completion queue. */
	completion_queue_attr = {
		.flags = 0,

		/* When completing operations, like 'fi_send' for instance, you
		 * might poll the CQ to get a completion entry. This option ensures
		 * that the returned info is the context pointer. */
		.format = FI_CQ_FORMAT_CONTEXT,
		.wait_obj = FI_WAIT_UNSPEC,
		.signaling_vector = 0,
		.wait_cond = FI_CQ_COND_NONE, /* No condition needs to be met. */
		.wait_set = 0
	};
	
	return 0;
}

/* Gracefully shut down the network. */
int Fabric::fabric_finalize() {
	/* Endpoints must be closed before any objects bound to them can be. */
	check_libfabric(fi_close(&endpoint->fid),
			"fi_close(), endpoint");

	/* Objects inside a domain have to be closed before the domain can. */
	check_libfabric(fi_close(&recv_queue->fid), "fi_close(), recv_queue");
	check_libfabric(fi_close(&transmit_queue->fid), 
			"fi_close(), transmit_queue");

	/* The domain and the event queue must be closed. The passive endpoint
	 * is closed by it's own server-side. */
	check_libfabric(fi_close(&domain->fid), "fi_close(), domain");
	check_libfabric(fi_close(&event_queue->fid),
			"fi_close(), event_queue");

	/* Now close the fabric network. */
	check_libfabric(fi_close(&fabric->fid), "fi_close(), fabric");

	/* Free the fabric info structure last. */
	fi_freeinfo(info);

	return 0;
}

/* Initialize and listen on a libfabric server. */
int FabricServer::fabric_server(int port) {
	/* Initialize the network somewhat. */
	fabric_init(true);

	/* Create a passive endpoint for the server. It will be used for listening 
	 * for incoming connections. */
	check_libfabric(fi_passive_ep(fabric, info, &passive_endpoint, nullptr),
			"fi_passive_ep()");

	/* Bind the passive endpoint to the event queue. */
	check_libfabric(fi_pep_bind(passive_endpoint, &event_queue->fid, 0), 
			"fi_pep_bind()");

	/* Listen on the passive endpoint. */
	check_libfabric(fi_listen(passive_endpoint), "fi_listen()");


	/* Obtain the address information of the passive endpoint. */
	sockaddr_in addr = {};
	size_t addr_length = sizeof(sockaddr_in);
	check_libfabric(fi_getname(&passive_endpoint->fid, &addr, &addr_length),
			"fi_getname()");

	std::print("Server address: {}:{}\n", inet_ntoa(addr.sin_addr), 
			ntohs(addr.sin_port));

	/* A struct. for reporting connection management events in an event queue.
	 * When a remote peer calls "fi_connect()", the peer that uses 
	 * 'fi_listen()' will receive a connection request in the form of
	 * 'fi_eq_cm_entry'. 'conn_req' contains a pointer to the client's
	 * endpoint info., of which we can use to establish this connection. */
	fi_eq_cm_entry conn_req = {};
	uint32_t event_type;
	do {
		/* Read from the event queue (synchronous (blocking)). */
		int return_code = fi_eq_sread(event_queue, &event_type, &conn_req, 
				sizeof(fi_eq_cm_entry), -1, 0);
		if (return_code < 0) {
			if (-FI_EAVAIL == return_code) {
				check_eq_error(event_queue);
			} else {
				std::print(stderr, "fi_eq_sread(): {}\n", 
						fi_strerror(-return_code));
			}
		}
	} while (event_type != FI_CONNREQ);

	/* Create a domain for the client based off of their provider info. */
	check_libfabric(fi_domain(fabric, conn_req.info, &domain, 0),
			"fi_domain(), server, conn_req");

	/* Create an endpoint for the client using their provider info. */
	check_libfabric(fi_endpoint(domain, conn_req.info, &endpoint, 0),
			"fi_domain(), server, conn_req");

	/* Open a completion queue bound to the requestor's domain and endpoint. */
	check_libfabric(fi_cq_open(domain, &completion_queue_attr, 
				&transmit_queue, nullptr), "fi_cq_open(), server");
	check_libfabric(fi_ep_bind(endpoint, &transmit_queue->fid, FI_TRANSMIT),
			"fi_ep_bind(), server, transmit_queue");

	/* Bind the new active endpoint to the event queue and enable it.*/
	check_libfabric(fi_ep_bind(endpoint, &event_queue->fid, 0),
			"fi_ep_bind(), conn_req endpoint, event_queue");
	check_libfabric(fi_enable(endpoint), "fi_enable(), conn_req, endpoint");

	/* This is asynchronous, so post a receive buffer so when data is sent, 
	 * there is a place for it to go. */
	float recv_buffer = 0.0;
	check_libfabric(fi_recv(endpoint, &recv_buffer, sizeof(float), 0, 0, 0),
			"fi_recv(), conn_req");

	/* Send an acceptance response back to the requestor. */
	check_libfabric(fi_accept(endpoint, 0, 0), "fi_accept, conn_req");

	/* Use the actice endpoint (the connection requestor) to post an
	 * 'FI_CONNECTED' event into the event queue. */
	fi_eq_entry conn = {};
	do {
		int return_code = fi_eq_sread(event_queue, &event_type, &conn,
				sizeof(fi_eq_entry), 500, 0);
		if (return_code < 0) {
			if (-FI_EAVAIL == return_code) {
				check_eq_error(event_queue);
			} else {
				std::print(stderr, "fi_eq_sread(), FI_CONNECTED: {}\n",
						fi_strerror(-return_code));
			}
		}
	} while (event_type != FI_CONNECTED);

	float send_buffer = 123.45;
	check_libfabric(fi_send(endpoint, &send_buffer, sizeof(float), 0, 0, 0),
			"fi_send, conn_req, endpoint");

	fi_cq_data_entry transmit_cq_entry = {};
	int read = -1;
	do {
		read = fi_cq_sread(transmit_queue, &transmit_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);

	fi_cq_data_entry recv_cq_entry = {};
	do {
		read = fi_cq_sread(recv_queue, &recv_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);

	/* Now that we are done, release the conn. to the client. */
	check_libfabric(fi_shutdown(endpoint, 0),
			"fi_shutdown(), server, conn_req");

	/* Close the passive endpoint. */
	check_libfabric(fi_close(&passive_endpoint->fid),
			"fi_close(), passive_endpoint");

	/* The server will properly shutdown the rest of it's
	 * components in function 'fabric_finalize()'. */

	return 0;
}

/* Initialize and use a libfabric client. */
int FabricClient::fabric_client(const char* dest_addr, int dest_port) {
	/* Initialize the network somewhat. */
	fabric_init(false);

	sockaddr_in dest = {};
	dest.sin_family = AF_INET;
	dest.sin_port = htons(dest_port);
	inet_aton(dest_addr, &dest.sin_addr);

	/* Create a domain for endpoints to be created on top of. */
	check_libfabric(fi_domain(fabric, info, &domain, 0),
			"fi_domain(), client");

	/* Create an endpoint that is responsible for initiating 
	 * communication. */
	check_libfabric(fi_endpoint(domain, info, &endpoint, nullptr),
			"fi_endpoint(), client");

	/* Open a receiving and transmission completion queue. */
	check_libfabric(fi_cq_open(domain, &completion_queue_attr,
				&recv_queue, nullptr), "fi_cq_open(), recv_queue, client");
	check_libfabric(fi_cq_open(domain, &completion_queue_attr, &transmit_queue,
				nullptr), "fi_cq_open(), transmit_queue, client");

	/* Bind and endpoint to both of the new completion queues. */
	check_libfabric(fi_ep_bind(endpoint, &(recv_queue->fid), FI_RECV),
			"fi_ep_bind, recv_queue, client");
	check_libfabric(fi_ep_bind(endpoint, &(transmit_queue->fid), FI_TRANSMIT),
			"fi_ep_bind, transmit_queue, client");

	/* Bind the endpoint to the event queue and enable it. */
	check_libfabric(fi_ep_bind(endpoint, &event_queue->fid, 0), 
			"fi_ep_bind, event_queue, client");
	check_libfabric(fi_enable(endpoint), "fi_enable, endpoint, client");

	/* Communication is asynchronous for the most part in libfabric, so the 
	 * provider only completes the operation when the matching work on
	 * the other size actually exists. */
	float recv_buffer = 0.0;
	check_libfabric(fi_recv(endpoint, &recv_buffer, sizeof(float), 0, 0, 0),
			"fi_recv(), client");

	/* Send the server the connection request. */
	check_libfabric(fi_connect(endpoint, &dest, 0, 0), "fi_connect(), client");

	/* Use the active endpoint to post a "FI_CONNECTED" event into the
	 * event queue. */
	fi_eq_entry event = {};
	uint32_t event_type;
	do {
		int return_code = fi_eq_sread(event_queue, &event_type, &event,
				sizeof(fi_eq_entry), 500, 0);
		if (return_code < 0) {
			if (-FI_EAVAIL == return_code) {
				check_eq_error(event_queue);
			} else {
				std::print(stderr, "fi_eq_sread(), client, FI_CONNECTED: {}\n",
						fi_strerror(-return_code));
			}
		}
	} while (event_type != FI_CONNECTED);

	/* Post a send buffer to the endpoint. At this point, we are connected. */
	float send_buffer = 678.90;
	check_libfabric(fi_send(endpoint, &send_buffer, sizeof(float), 0, 0, 0),
			"fi_send(), client)");

	struct fi_cq_data_entry transmit_cq_entry = {};
	int read = -1;
	do {
		read = fi_cq_sread(transmit_queue, &transmit_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);
	/* check_libfabric(read); */

	struct fi_cq_data_entry recv_cq_entry = {};
	do {
		read = fi_cq_sread(recv_queue, &recv_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);

	/* Gracefully shutdown the client. */
	fabric_finalize();
	
	return 0;
}

/* Perform error checking for libfabric functions. */
void Fabric::check_libfabric(int code, const char* message) {
	if (code != 0) {
		/* Error constants (or macros) in their library are positive. and then
		 * are returned as negative whenever they are returned through their
		 * various functions (i.e. -FI_EADDRINUSE). However, fi_strerror
		 * expects the actual error constant before it was inverted, so we must
		 * pass it with '-'. */
		std::print(stderr, "{}: {}\n", message, fi_strerror(-code));
		exit(EXIT_FAILURE);
	}
}

/* Perform error checking for specific event queues. */
void Fabric::check_eq_error(fid_eq* event_queue) {
	struct fi_eq_err_entry error_entry = {};
	fi_eq_readerr(event_queue, &error_entry, 0); /* Read the EQ. */

	std::print(stderr, "Event Queue Error: {}, Data Size: {}\n", 
			fi_strerror(error_entry.err), error_entry.err_data_size);
}
