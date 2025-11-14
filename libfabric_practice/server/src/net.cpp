#include "net.hpp"
#include "err.hpp"
#include "debugger.hpp" /* Thank you Riley! :D */

/* Initialize and listen as a libfabric server. */
int server() {

	/* Create a structure that holds the libfabric config. that
	 * is being requested. This structure will be used to request
	 * an actual structure to begin making the network. */
	fi_info* hints = fi_allocinfo();

	/* Request the ability to use the send/recv form of message passing. */
	hints->caps = FI_SEND | FI_RECV;

	/* Request a connection-oriented endpoint (TCP). */
	hints->ep_attr->type = FI_EP_MSG;

	/* Use the hinting structure to capture the real configuration
	 * for the network. */
	fi_info* info = nullptr;
	check_libfabric(fi_getinfo(FI_VERSION(1, 15), 0, 0, 0,
				hints, &info), "fi_getinfo()");

	/* Now that there is an actual config. from libfabric,
	 * free the 'hints' info struct. */
	fi_freeinfo(hints);

	/* Use the info gathered to create a libfabric network using the
	 * available providers available to the OS. This represents
	 * a collection of resources such as domains, event queues,
	 * completion queues, endpoints, etc. */
	fid_fabric* fabric;
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
	fid_eq* event_queue = nullptr;
	check_libfabric(fi_eq_open(fabric, &event_queue_attr, &event_queue, 0),
			"fi_eq_open()");

	/* Configure attributes of the completion queue. */
	fi_cq_attr completion_queue_attr = {
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

	/* Create a passive endpoint for the server. It will be used for listening
	 * for incoming connections. */
	fid_pep* passive_endpoint = nullptr;
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

	std::cout << "Server address: " << inet_ntoa(addr.sin_addr) << ":" <<
		ntohs(addr.sin_port) << std::endl;

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
				std::fprintf(stderr, "fi_eq_sread(): %s\n",
						fi_strerror(-return_code));
			}
		}
	} while (event_type != FI_CONNREQ);

	/* Create a domain for the client based off of their provider info. */
	fid_domain* domain = nullptr;
	check_libfabric(fi_domain(fabric, conn_req.info, &domain, 0),
			"fi_domain(), server, conn_req");

	/* Create an endpoint for the client using their provider info. */
	fid_ep* endpoint = nullptr;
	check_libfabric(fi_endpoint(domain, conn_req.info, &endpoint, 0),
			"fi_domain(), server, conn_req");

	/* Open a completion queue bound to the requestor's domain and endpoint. */
	fid_cq* transmit_queue = nullptr;
	check_libfabric(fi_cq_open(domain, &completion_queue_attr,
				&transmit_queue, nullptr), "fi_cq_open(), server");
	check_libfabric(fi_ep_bind(endpoint, &transmit_queue->fid, FI_TRANSMIT),
			"fi_ep_bind(), server, transmit_queue");

	/* TODO. It is entirely possible the same thing needs to happen with
	 * recv_queue. */

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
				std::fprintf(stderr, "fi_eq_sread(), FI_CONNECTED: %s\n",
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
	fid_cq* recv_queue = nullptr;
	do {
		read = fi_cq_sread(recv_queue, &recv_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);

	/* Now that we are done, release the conn. to the client. */
	check_libfabric(fi_shutdown(endpoint, 0),
			"fi_shutdown(), server, conn_req");

	/* Close the passive endpoint. */
	check_libfabric(fi_close(&passive_endpoint->fid),
			"fi_close(), passive_endpoint");

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
