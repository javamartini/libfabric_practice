#include "net.hpp"

/* Gather network information to configure the network. */
int Fabric::fabric_init() {
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
	check_libfabric(fi_getinfo(FI_VERSION(1, 15), 0, 0, 0, hints, &info),
			"fi_getinfo()");

	/* Now that there is an actual config. from libfabric,
	 * free the 'hints' info struct. */
	fi_freeinfo(hints);

	/* Use the info gathered to create a libfabric network using the 
	 * available providers available to the OS. This represents
	 * a collection of resources such as domains, event queues,
	 * completion queues, endpoints, etc. */
	check_libfabric(fi_fabric(info->fabric_attr, &fabric, 0),
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
	check_libfabric(fi_close(&endpoints->fid),
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
