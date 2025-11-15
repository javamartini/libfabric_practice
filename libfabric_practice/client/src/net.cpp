#include "net.hpp"
#include "err.hpp"
#include "debugger.hpp" /* Thanks again Riley! :D */

/* Initialize and use a libfabric client. */
int client(const char* dest_addr, int dest_port) {
	/* Create a structure that holds the libfabric config. that
	 * is being requested. This structure will be used to request
	 * an actual structure to begin making the network. */
	fi_info* hints = fi_allocinfo();

	/* Request the ability to use the send/recv form of message passing. */
	hints->caps = FI_SEND | FI_RECV;

	/* Request a connection-oriented endpoint (TCP). */
	hints->ep_attr->type = FI_EP_MSG;

	fi_info* info = nullptr;
	check_libfabric(fi_getinfo(FI_VERSION(1, 15), 0, 0, 0, hints, &info),
				"fi_getinfo()");

	/* Now that there is an actual configuration from libfabric
	 * free the 'hints' info structure. */
	fi_freeinfo(hints);

	/* Use the info gathered to create a libfabric network using the
	 * available providers available to the OS. This represents
	 * a collection of resources such as domains, event queues,
	 * completion queues, endpoints, etc. */
	fid_fabric* fabric = nullptr;
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

	sockaddr_in dest = {};
	dest.sin_family = AF_INET;
	dest.sin_port = htons(dest_port);
	inet_aton(dest_addr, &dest.sin_addr);

	/* Create a domain for endpoints to be created on top of. */
	fid_domain* domain = nullptr;
	check_libfabric(fi_domain(fabric, info, &domain, 0),
			"fi_domain(), client");

	/* Create an endpoint that is responsible for initiating
	 * communication. */
	fid_ep* endpoint = nullptr;
	check_libfabric(fi_endpoint(domain, info, &endpoint, nullptr),
			"fi_endpoint(), client");

	Debugger debug;
	debug.print_info(info);

	/* Open a receiving and transmission completion queue. */
	fid_cq* transmit_queue = nullptr;
	fid_cq* recv_queue = nullptr;
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
				sizeof(fi_eq_entry), -1, 0);
		if (return_code < 0) {
			if (-FI_EAVAIL == return_code) {
				check_eq_error(event_queue);
			} else {
				std::fprintf(stderr,
						"fi_eq_sread(), client, FI_CONNECTED: %s\n",
						fi_strerror(-return_code));
			}
		}
	} while (event_type != FI_CONNECTED);

	/* Post a send buffer and send it to the now-connected server. This is an
	 * asynchronous call. We are just sending a floating point number here. */
	float send_buffer = 678.90;
	check_libfabric(fi_send(endpoint, &send_buffer, sizeof(float), 0, 0, 0),
			"fi_send(), client)");

	/* We read the transmission completion queue, and it will let us know when
	 * the message has been transmitted. This essentially turns an asynchronous
	 * call and make it synchronous. */
	struct fi_cq_data_entry transmit_cq_entry = {};
	int read = -1;
	do {
		read = fi_cq_sread(transmit_queue, &transmit_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);
	/* check_libfabric(read); */

	/* We read the receiving completion queue, and it will let us know when
	 * a message has been received. Even though we do not explicitely ask to
	 * grab that data by using fi_recv(), we do get the data from the connected
	 * server. So, there is still an entry in the completion queue. */
	struct fi_cq_data_entry recv_cq_entry = {};
	do {
		read = fi_cq_sread(recv_queue, &recv_cq_entry, 1, 0, -1);
	} while (read == -FI_EAGAIN);

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
