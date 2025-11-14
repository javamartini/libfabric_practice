#include "err.hpp"

/* Perform error checking for libfabric functions. */
void check_libfabric(int code, const char* message) {
	if (code != 0) {
		/* Error constants (or macros) in their library are positive. and then
		 * are returned as negative whenever they are returned through their
		 * various functions (i.e. -FI_EADDRINUSE). However, fi_strerror
		 * expects the actual error constant before it was inverted, so we must
		 * pass it with '-'. */
		std::cerr << message << ": " << fi_strerror(-code) << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

/* Perform error checking for specific event queues. */
void check_eq_error(fid_eq* event_queue) {
	struct fi_eq_err_entry error_entry = {};
	fi_eq_readerr(event_queue, &error_entry, 0); /* Read the EQ. */

	std::cerr << "Event queue error: " << fi_strerror(error_entry.err) <<
		", Data size: " << error_entry.err_data_size << std::endl;
}
