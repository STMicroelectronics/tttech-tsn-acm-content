/**
 * @file configuration.c
 *
 * Configuration file access
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <dlfcn.h>

#include "logging.h"
#include "configuration.h"
#include "demo.h"

/**
 * @brief Remove leading and trailing whitespaces
 */
static char *trim(char *str)
{
	char *end;

/*
 * suppress false positive scan-build:
 * Out of bound memory access (index is tainted)
 */
#ifndef __clang_analyzer__
	/* Trim leading space */
	while (isspace((unsigned char)*str))
		str++;
#endif
	if (*str == 0)  /* All spaces? */
		return str;

	/* Trim trailing space */
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	/* Write new null terminator character */
	end[1] = '\0';

	return str;
}


int endsWith(const char *str, const char *tail)
{
	size_t lenstr;
	size_t lentail;

	if (!str || !tail)
		return 0;

	lenstr = strlen(str);
	lentail = strlen(tail);
	if (lentail > lenstr)
		return 0;
	return strncmp(str + lenstr - lentail, tail, lentail) == 0;
}

static int parse_config_line(const char *line,
			     struct configuration_entry *entry)
{
	int ret = 0;
	char *trimmed_line;
	char *funcname;
	char *prepare_funcname;
	char *buffername;
	void *handle;
	const char *delimit = ",\n#";
	char *line2 = strdup(line);

	trimmed_line = trim(line2);
	/* check for empty or comment line */
	if ((strlen(trimmed_line) < 1) ||
	    (trimmed_line[0] == '#') ||
	    ((strlen(trimmed_line) >= 2) && (trimmed_line[0] == '/') &&
	     ((trimmed_line[1] == '/') || (trimmed_line[1] == '*'))) ||
	    (trimmed_line[0] == '\n')
	   ) {
		ret = 1;
		goto out;
	}

	ret = -1;
	trimmed_line = strtok(trimmed_line, delimit);
	if (!trimmed_line)
		goto out;
	buffername = strdup(trimmed_line);
	trimmed_line = strtok(NULL, delimit);
	if (!trimmed_line)
		goto free_buffername;
	trimmed_line = trim(trimmed_line);
	funcname = strdup(trimmed_line);
	trimmed_line = strtok(NULL, delimit);
	if (!trimmed_line)
		goto free_funcname;
	trimmed_line = trim(trimmed_line);
	entry->size = atoi(trimmed_line);
	trimmed_line = strtok(NULL, delimit);
	if (!trimmed_line)
		goto free_funcname;
	trimmed_line = trim(trimmed_line);
	entry->time_offs = atoi(trimmed_line);
	trimmed_line = strtok(NULL, delimit);
	if (!trimmed_line)
		goto free_funcname;
	trimmed_line = trim(trimmed_line);
	entry->period = atoi(trimmed_line);
	if (get_param_hw_cycle_us() % entry->period != 0) {
		LOGGING_ERR("Cycle (%u) is not a multiple of %s's buffer cycle (%u)",
			get_param_hw_cycle_us(), buffername, entry->period);
		goto free_funcname;
	}

	/* now lookup required function */
	handle = dlopen(NULL, RTLD_LAZY | RTLD_NOLOAD);
	if (!handle) {
		LOGGING_ERR("dlopen on main module: %s", dlerror());
		goto free_funcname;
	}

	/* check if we should use the timestamp support */
	entry->use_timestamp = endsWith(funcname, "_t");
	if (entry->use_timestamp)
		/* truncate function */
		funcname[strlen(funcname) - 2] = 0x00;

	entry->function = dlsym(handle, funcname);

	/* eventually bind to respective prepare_ function */
	ret = asprintf(&prepare_funcname, "prepare_%s", funcname);
	if (ret < 0) {
		entry->prepare_function = NULL;
		goto out_dlclose;
	}

	entry->prepare_function = dlsym(handle, prepare_funcname);

out_dlclose:
	dlclose(handle);

	if (entry->function) {
		LOGGING_DEBUG("%s: %s%s: entry=%p, entry->worker=%p", __func__,
			funcname, entry->function ? "_t" : "", entry,
			entry->function);
		entry->buffer_name = strdup(buffername);
		if (entry->prepare_function)
			LOGGING_DEBUG("%s%s: %s: prepare function=%s", __func__,
				funcname, entry->function ? "_t" : "",
				prepare_funcname);
		free(prepare_funcname);
		ret = 0;
	} else
		LOGGING_ERR("%s%s not supported", funcname,
			entry->function ? "_t" : "");

free_funcname:
	free(funcname);
free_buffername:
	free(buffername);
out:
	free(line2);
	return ret;
}

static int insert_entry(struct configuration_list *configurations,
			struct configuration_entry *entry)
{
	STAILQ_INSERT_TAIL(configurations, entry, entries);
	return 0;
}

/**
 * brief Read task configuration from file
 */
int parse_configuration(const char *filename,
			struct configuration_list *configurations)
{
	FILE * fp;
	ssize_t read;
	size_t len = 0;
	char *line = NULL;
	int ret = 0;
	int index = 0;

	fp = fopen(filename, "r");

	if (!fp) {
		LOGGING_ERR("File: %s: %s", filename, strerror(errno));
		ret = -ENOENT;
		goto out;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		struct configuration_entry *entry;

		entry = calloc(1, sizeof(*entry));

		if (!entry) {
			ret = -ENOMEM;
			goto out_close;
		}

		ret = parse_config_line(line, entry);
		if (ret < 0) {
			free(entry);
			goto out_close;
		}
		/* ignore line */
		if (ret > 0) {
			free(entry);
			continue;
		}

		entry->index = index++;
		ret = insert_entry(configurations, entry);
		if (ret) {
			free(entry);
			goto out_close;
		}
	}

	ret = index;

out_close:
	fclose(fp);
	free(line);
out:
	return ret;
}

void empty_configurations(struct configuration_list *configurations)
{
	struct configuration_entry *entry;

	STAILQ_FOREACH(entry, configurations, entries) {
		STAILQ_REMOVE(configurations, entry, configuration_entry,
			      entries);
		free(entry->buffer_name);
		free(entry);
	}
}

void log_configurations(int loglevel, struct configuration_list *configurations)
{
	struct configuration_entry *config;

	STAILQ_FOREACH(config, configurations, entries) {
		Dl_info info;

		dladdr(config->function, &info);

		LOG(loglevel,
		    "configuration %d (%p): buffer=%s, worker=%s (%p), data size=%d, time offset=%d",
		    config->index, config, config->buffer_name, info.dli_sname,
		    config->function, config->size, config->time_offs);
	}
}

