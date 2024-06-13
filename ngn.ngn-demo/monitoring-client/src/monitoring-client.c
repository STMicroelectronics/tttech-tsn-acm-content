#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "args_parser.h"

static int write_cmd(int fd, const char *cmd)
{
	int ret;

	ret = write(fd, cmd, strlen(cmd));
	if (ret == strlen(cmd))
		return 0;
	if (ret == -1)
		return -errno;

	return -EIO;
}

#define BUFLEN	0x4000

static int receive_response(int fd, char **response)
{
	int ret;
	int len;
	char *data;
	unsigned int idx, endcount;

	for (idx = 0, endcount = 0, data = NULL, len = 0;
	     endcount < 3; ++idx) {

		if (idx >= len) {
			char *newdata;

			len += BUFLEN;
			newdata = realloc(data, len);
			if (!newdata) {
				ret = -ENOMEM;
				free(data);
				data = NULL;
				break;
			}
			data = newdata;
		}

		ret = recv(fd, &data[idx], 1, 0);
		if (ret <= 0) {
			free(data);
			data = NULL;
			ret = ret == 0 ? -ENODATA : ret;
			break;
		}

		if (data[idx] == '\n') {
			endcount++;
			/* terminate message string */
			if (endcount == 3) {
				data[idx] = '\0';
				ret = 0;
			}
		} else
			endcount = 0;
	}

	*response = data;
	return ret;
}

int main(int argc, char* argv[])
{
	int ret;
	struct argv_param args;
	int fd;
	struct sockaddr_in server = { 0 };
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *p;

	parse_args(argc, argv, &args);

retry:
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "Cannot create socket: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(args.host, NULL, &hints, &result);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo(%s): %s\n", args.host,
			gai_strerror(ret));
		return EXIT_FAILURE;
	}

	for (p = result; p; p->ai_next) {
		struct in_addr sin_addr;

		sin_addr = ((struct sockaddr_in *)(p->ai_addr))->sin_addr;

		if (sin_addr.s_addr != INADDR_NONE) {
			server.sin_addr = sin_addr;
			break;
		}
	}
	freeaddrinfo(result);

	server.sin_family = AF_INET;
	server.sin_port = htons(args.port);

	do {
		ret = connect(fd, (struct sockaddr *)&server, sizeof(server));

		if (ret == 0)
			/* connected successfully */
			break;

		ret = -errno;

		if ((args.reconnect) != 0 && (ret == -ECONNREFUSED)) {
			ret = usleep(args.reconnect * 1000);
			if (ret == -EINVAL)
				sleep(args.reconnect / 1000);
		} else
			/* no connect retry or error */
			break;

	} while (true);

	switch (ret) {
	case 0:
		/* nothing to do */
		break;
	case -EINTR:
		/* reconnect aborted */
		if (args.reconnect != 0) {
			ret =  EXIT_SUCCESS;
			goto close;
		}
		/* fallthrough */
	default:
		fprintf(stderr, "Connecting to %s:%d failed: %s\n",
			args.host, args.port,
			strerror(ret));
		ret = EXIT_FAILURE;
		goto close;
	}


	if (args.diagnostics.enabled) {
		char *cmd;
		char *diag_data;

		asprintf(&cmd, "DIAGNOSTICS %d %d %d\n",
			args.diagnostics.offset, args.diagnostics.mult,
			args.diagnostics.count);

		ret = write_cmd(fd, cmd);

		if (ret) {
			fprintf(stderr, "Writing command \"%s\" to %s:%d failed: %s",
				cmd, args.host, args.port, strerror(ret));
			free(cmd);
			goto close;
		}
		free(cmd);

		ret = receive_response(fd, &diag_data);
		if (ret)
			goto close;
		fprintf(stdout, "%s", diag_data);
		fflush(stdout);

		free(diag_data);
		goto close;
	}


	do {
		char *dump_cmd;
		char *dump_data;

		if (args.missed) {
			dump_cmd = "DUMP2\n";
		} else {
			dump_cmd = "DUMP\n";
		}

		ret = write_cmd(fd, dump_cmd);
		if (ret)
			break;
		ret = receive_response(fd, &dump_data);
		if (ret)
			goto close;
		fprintf(stdout, "%s", dump_data);
		fflush(stdout);

		free(dump_data);

		if (args.cycles) {
			const char *dump_cycles_cmd = "DUMPCYC\n";

			ret = write_cmd(fd, dump_cycles_cmd);
			if (ret)
				break;
			ret = receive_response(fd, &dump_data);
			if (ret)
				goto close;
			fprintf(stdout, "%s", dump_data);
			fflush(stdout);

			free(dump_data);
		}

		if (args.dump_counter >= 0)
			args.dump_counter--;
		if (args.dump_counter == 0)
			break;

		usleep(args.interval * 1000);
	} while (ret >= 0 || ret == -ECONNRESET);

close:
	close(fd);

	if ((ret < 0) && (args.reconnect != 0)) {
		switch (ret) {
		case -EINTR:
			ret = EXIT_SUCCESS;
			break;
		default:
			goto retry;
		}
	}

	return ret;
}
