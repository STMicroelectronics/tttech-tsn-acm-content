/**
 * @file demo.c
 *
 * ACM demo main
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <sched.h>
#include <sys/stat.h>

#include "parser.h"
#include "configuration.h"
#include "messagebuffer.h"
#include "logging.h"
#include "worker.h"
#include "monitor.h"
#include "tracer.h"
#include "monitor_server.h"
#include "dump.h"
#include "sps_demo.h"

static struct argv_param acm_demo_params;
static int stopping = 0;

void force_stop(void)
{
	if (!__sync_fetch_and_add(&stopping, 1)) {

		stop_workers();
		stop_diag_workers();
		tracing(0);

		/* do a final dump to stderr */
		monitor_dump2(NULL);
		monitor_dump_cycles(NULL);

		raise(SIGINT);
	}
}

uint32_t get_param_hw_cycle_us(void)
{
	return acm_demo_params.hw_cycle_us;
}

uint32_t get_param_max_frame_counter(void)
{
	return acm_demo_params.dump_counter;
}

#ifdef FTRACE_SUPPORT_ENABLED
uint32_t get_param_tracelimit()
{
	return acm_demo_params.tracelimit;
}

uint32_t get_param_lat_break()
{
	return acm_demo_params.lat_break;
}

int get_param_break_on_loss(void)
{
	return acm_demo_params.break_on_loss;
}

int get_param_break_on_double(void)
{
	return acm_demo_params.break_on_double;
}

int get_param_break_on_invalid(void)
{
	return acm_demo_params.break_on_invalid;
}

int get_param_break_on_missed(void)
{
	return acm_demo_params.break_on_missed;
}
#endif

uint32_t get_param_portno(void)
{
	return acm_demo_params.portno;
}

uint16_t get_param_pc_port(void)
{
	return acm_demo_params.port;
}

const char *get_param_pc_host(void)
{
	return acm_demo_params.host;
}

const cpu_set_t *get_param_cpuset(void)
{
	return &acm_demo_params.cpuset;
}

const bool get_param_distribute(void)
{
	return acm_demo_params.distribute;
}

int get_param_rxoffset(void)
{
	return acm_demo_params.rxoffset;
}

static int disable_rt_throttling(void)
{
	int fd, ret = 0;

	/* disable RT throttling */
	fd = open("/proc/sys/kernel/sched_rt_runtime_us", O_WRONLY);
	if (fd < 0) {
		LOGGING_ERR("Opening sched_rt_runtime_us failed: %s",
			strerror(errno));
		ret = -errno;
		goto out;
	}
	write(fd, "-1\n", 3);
	close(fd);
out:
	return ret;
}

static int get_pid_by_name(const char *name, pid_t **pidlist)
{
	int ret;
	DIR *dir;
	struct dirent *next;
	pid_t* list;
	int i;

	dir = opendir("/proc");
	if (!dir) {
		LOGGING_ERR("Cannot open /proc: %s", strerror(errno));
		return -errno;
	}

	for (i = 0, next = readdir(dir), list = NULL;
	     next;
	     next = readdir(dir)) {
		int fd;
		char *filename;
		char comm[17];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		ret = asprintf(&filename, "/proc/%s/comm", next->d_name);
		if (ret < 0) {
			ret = -ENOMEM;
			goto end_loop;
		}

		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			ret = 0;
			goto free_filename;
		}

		ret = read(fd, comm, sizeof(comm) - 1);
		if (ret <= 0) {
			ret = 0;
			goto close;
		}

		comm[ret - 1] = '\0';
		ret = 0;

		if (!strncmp(name, comm, strlen(name))) {
			pid_t* new_list;

			LOGGING_DEBUG("%s: Found %s", __func__, comm);
			new_list = realloc(list, (i + 1) * sizeof(pid_t));
			if (!new_list) {
				ret = -ENOMEM;
				goto close;
			}
			list = new_list;
			list[i++] = strtol(next->d_name, NULL, 0);
		}

close:
		close(fd);
free_filename:
		free(filename);

end_loop:
		if (ret) {
			free(list);
			goto out;
		}

	}

	*pidlist = list;
	ret = i;
out:
	closedir(dir);
	return ret;
}

static int set_ktimersoftd_priority(int prio)
{
	int ret = 0;
	int no;
	int i;

	pid_t *pidlist = NULL;

	/* find ktimersoftd threads */
	no = get_pid_by_name("ktimersoftd", &pidlist);
	if (no < 0)
		return no;
	if (no == 0)
		/* no ktimersoftd found, nothing to do */
		return 0;

	LOGGING_DEBUG("%s: Found %d ktimersoftd threads", __func__, no);
	for (i = 0; (i < no) && pidlist; ++i)  {
		const struct sched_param param = {
			.sched_priority = prio,
		};

		ret = sched_setscheduler(pidlist[i], SCHED_FIFO, &param);
		if (ret < 0) {
			LOGGING_ERR("sched_setscheduler(%d) failed: %s",
				strerror(errno));
			ret = -errno;
			goto out;
		}
	}

out:
	free(pidlist);
	return ret;
}

/**
 * @brief main entry
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	struct configuration_list configurations;
	struct configuration_entry *config;
	int config_num;
	int sig;

	STAILQ_INIT(&configurations);

	parse_args(argc, argv, &acm_demo_params);
	set_loglevel(acm_demo_params.verbosity);
	set_logger(LOGGER_STDERR);

	if (acm_demo_params.daemon) {
		// daemonize
		pid_t pid, sid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0) {
			LOGGING_ERR("fork() failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		/* exit the parent process */
		if (pid > 0) {
			exit(EXIT_SUCCESS);
		}
		umask(0);
		set_logger(LOGGER_SYSLOG);
		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0) {
			LOGGING_ERR("setsid() failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Change the current working directory */
		if ((chdir("/")) < 0) {
			LOGGING_ERR("chdir() failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Close out the standard file descriptors */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	LOGGING_INFO("Starting %s", acm_demo_params.version);
	log_args(LOGLEVEL_DEBUG, argv[0], &acm_demo_params);

	ret = sps_demo_init();
	if (ret)
		LOGGING_ERR("sps_demo_init() failed: %s, ignoring", strerror(-ret));

	LOGGING_DEBUG("Parsing configuration ...");
	ret = parse_configuration(acm_demo_params.conf_name, &configurations);
	if (ret < 0) {
		LOGGING_ERR("Parsing configuration failed: %s", strerror(-ret));
		goto out;
	}

	config_num = ret;
	if (config_num == 0) {
		LOGGING_WARN("No configurations found, nothing to do.");
		ret = 0;
		goto out;
	}
	log_configurations(LOGLEVEL_INFO, &configurations);

	LOGGING_DEBUG("Reading messagebuffers ...");
	ret = read_messagebuffers();
	if (ret) {
		LOGGING_ERR("Reading messagebuffers failed: %d", ret);
		goto free_configs;
	}
	log_messagebuffers(LOGLEVEL_INFO);

	/* map messagebuffer to config */
	LOGGING_DEBUG("Mapping message buffers to configuration ...");
	STAILQ_FOREACH(config, &configurations, entries) {
		struct messagebuffer *msgbuf;

		msgbuf = get_messagebuffer_by_name(config->buffer_name);
		if (!msgbuf) {
			LOGGING_ERR("Messagebuffer %s not found.",
				config->buffer_name);
			ret = -ENODEV;
			goto free_configs;
		}

		config->msgbuf = msgbuf;
		LOGGING_DEBUG("%s: message buffer %d maps to configuration %d",
			msgbuf->alias.alias, msgbuf->alias.idx, config->index);
	}

	LOGGING_DEBUG("Creating workers ...");
	ret = create_workers(&configurations);
	if (ret) {
		LOGGING_DEBUG("Creating workers failed: %d", ret);
		goto free_msgbuf;
	}
	LOGGING_DEBUG("Created %d workers", get_num_workers());

	setup_tracer();

	mlockall(MCL_CURRENT | MCL_FUTURE);

	ret = disable_rt_throttling();
	if (ret)
		goto free_workers;

	ret = set_ktimersoftd_priority(10);
	if (ret)
		goto free_workers;

	LOGGING_DEBUG("Starting workers ...");
	ret = start_workers();
	if (ret) {
		LOGGING_DEBUG("Starting workers failed: %d", ret);
		goto free_workers;
	}

	ret = monitor_server_start();
	if (ret == 0) {
		sigset_t sigset;

		/* wait for any kind of external process termination */
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGINT);
		sigaddset(&sigset, SIGQUIT);
		sigaddset(&sigset, SIGTSTP);
		sigaddset(&sigset, SIGTERM);
		sigaddset(&sigset, SIGKILL);

		sigwait(&sigset, &sig);

		monitor_server_stop();
	}

	stop_workers();
	stop_diag_workers();

free_workers:
	free_workers();
free_msgbuf:
	empty_messagebuffers();
free_configs:
	empty_configurations(&configurations);
out:
	sps_demo_exit();
	free_args(&acm_demo_params);
	return ret;
}
