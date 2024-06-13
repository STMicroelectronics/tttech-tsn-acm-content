/*
 * TTTech acm-yang-module
 * Copyright(c) 2020 TTTech Industrial Automation AG.
 *
 * ALL RIGHTS RESERVED.
 * Usage of this software, including source code, netlists, documentation,
 * is subject to restrictions and conditions of the applicable license
 * agreement with TTTech Industrial Automation AG or its affiliates.
 *
 * All trademarks used are the property of their respective owners.
 *
 * TTTech Industrial Automation AG and its affiliates do not assume any liability
 * arising out of the application or use of any product described or shown
 * herein. TTTech Industrial Automation AG and its affiliates reserve the right to
 * make changes, at any time, in order to improve reliability, function or
 * design.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */

#include <stdlib.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/* common includes */
#include "common_defines.h"
#include "common.h"
/* sysrepo includes */
#include <sysrepo.h>
#include <sysrepo/values.h>
#include <sysrepo/xpath.h>
/* libbase includes */
#include "libbase/base_defines.h"
/* libacm includes */
#include <libacmconfig/libacmconfig.h>
/* module specific */
#include "acm_defines.h"
#include "error_codes.h"
#include "parse_functions.h"

/* structure holding all the subscriptions */
sr_subscription_ctx_t *subscription;

/* Variable to detect is callback triggered for the first time
 * The startup datastore is copied to the running before this plugin is initialized.
 * So, at first time plugin started we need just to copy data to startup and running.
 * It is not necessary to trigger setter function.
 * "plugin_init" will be '0' if plugin is not initialized.
 * Set "plugin_init" to '1' after is initialized. The will be after fill startup datastore function.
 * */
int plugin_init = 0;

/**
 * @ingroup acminternal
 * @brief: Streams list
 */
typedef struct TTStream {
    LIST_ENTRY(TTStream) listEntry; /**< list entry */
    struct acm_stream *stream;      /**< pointer to a stream */
    int stream_id;                  /**< stream ID */
} TTStream;

static LIST_HEAD( ListOfTTStreams, TTStream ) tt_streams;

/**
 * @ingroup acminternal
 * @brief Initialize a list of streams.
 *
 * The function initializes an empty list of streams.
 */
void tt_stream_list_init(void) {
    LIST_INIT(&tt_streams);
}

/**
 * @ingroup acminternal
 * @brief Find a stream in the stream list.
 *
 * The function searches for a stream with the given ID.
 *
 * @param[in]   stream_id stream ID
 *
 * @return pointer to the found stream or NULL if stream not found
 */
int tt_stream_list_add(struct acm_stream *stream, int stream_id) {
    TTStream *entry;

    entry = (TTStream *) malloc(sizeof(TTStream));
    if (entry == NULL) {
        return -ENOMEM;
    }

    entry->stream = stream;
    entry->stream_id = stream_id;
    LIST_INSERT_HEAD(&tt_streams, entry, listEntry);
    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Find a stream in the stream list.
 *
 * The function searches for a stream with the given ID.
 *
 * @param[in]   stream_id stream ID
 *
 * @return pointer to the found stream or NULL if stream not found
 */
struct acm_stream *tt_stream_list_find(int stream_id) {
    TTStream *np;

    for (np = tt_streams.lh_first; np != NULL; np = np->listEntry.le_next) {
        if (np->stream_id == stream_id) {
            return np->stream;
        }
    }
    return NULL;
}

/**
 * @ingroup acminternal
 * @brief Clear the stream list.
 *
 * The function removes all entries in the stream list.
 */
void tt_stream_list_clear(void) {
    while (tt_streams.lh_first != NULL) {
        LIST_REMOVE(tt_streams.lh_first, listEntry);
    }
}

/**
 * @brief Callback to be called by the event of changing any running datastore content within the module.
 *
 * @param[in] session        Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in] module_name        Name of the module where the change has occurred.
 * @param[in] xpath          XPath used when subscribing, NULL if the whole module was subscribed to.
 * @param[in] event          Type of the notification event that has occurred.
 * @param[in] request_id     Request ID unique for the specific module_name. Connected events for one request (SR_EV_CHANGE and SR_EV_DONE, for example) have the same request ID.
 * @param[in] private_data   Private context opaque to sysrepo, as passed to sr_module_change_subscribe call.
 * @return      Error code (SR_ERR_OK on success).
 */
static int
module_change_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath, sr_event_t event, uint32_t request_id, void *private_data)
{
    (void)session;
    (void)module_name;
    (void)xpath;
    (void)event;
    (void)request_id;
    (void)private_data;

    return SR_ERR_OK;
#if 0
    sr_val_t *values = NULL;
    size_t count = 0;
    int rc = SR_ERR_OK;

    SRP_LOG_DBGMSG("Retrieve current configuration.\n");

    rc = sr_get_items(session, "/acm:acm", 0, 0, &values, &count);
    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR("Error by sr_get_items: %s", sr_strerror(rc));
        return rc;
    }
    for (size_t i = 0; i < count; i++){
        sr_print_val(&values[i]);
    }
    sr_free_values(values, count);
    return rc;
#endif
}

/**
 * @brief Creates a startup datastore for acm module. Function is going to be called once, due to sysrepo-plugind startup.
 *
 * @param[in] session     Sysrepo session that can be used for any API calls needed for plugin cleanup (mainly for unsubscribing of subscriptions initialized in sr_plugin_init_cb).
 * @return      EXIT_SUCCESS or EXIT_FAILURE.
 */
static int
acm_fill_startup_datastore(sr_session_ctx_t *session)
{
    const struct ly_ctx *ctx = NULL;
    const struct lys_module *module = NULL;
    struct lyd_node *root = NULL;
    int fd = -1;
    char *path = NULL;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if (0 == sr_path_check_startup_done_file(ACM_MODULE_STR, &path)) {
        ctx = sr_get_context(sr_session_get_connection(session));
        if (NULL == ctx) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_SESSION_CTX_FAILED_STR);
            free(path);
            return EXIT_FAILURE;
        }

        /* create acm root element (container acm) */
        root = lyd_new_path(NULL, ctx, "/acm:acm", NULL, 0, 0);
        if (NULL == root) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_NOT_CREATED_ROOT_ELEMENT_STR);
            free(path);
            return EXIT_FAILURE;
        }

        /* container acm, leaf config-change, default value 'false' */
        if (EXIT_FAILURE == new_node(root, module, ACM_CONFIG_CHANGE_STR, BASE_FALSE)) {
            free(path);
            return EXIT_FAILURE;
        }

        /* container acm, leaf schedule-change, default value 'false' */
        if (EXIT_FAILURE == new_node(root, module, ACM_SCHEDULE_CHANGE_STR, BASE_FALSE)) {
            return EXIT_FAILURE;
        }

        /******** NOTE *******/
        /* The startup to running datastore copy is done before the plugin is started.
         * So, here we will replace the current startup and running datastores with the
         * subtree called 'root' in this function.
         *  */

        /* switch to startup datastore */
        if (SR_ERR_OK != sr_session_switch_ds(session, SR_DS_STARTUP)) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_SWITCH_DATASTORE_FAILED_STR);
            free(path);
            return EXIT_FAILURE;
        }

        /* Replace current running configuration with created 'root' subtree */
        if (SR_ERR_OK != sr_replace_config(session, ACM_MODULE_STR, root, 0, 0)) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_REPLACE_CONFIG_FAILED_STR);
            free(path);
            return EXIT_FAILURE;
        }

        /* switch back to running datastore */
        if (SR_ERR_OK != sr_session_switch_ds(session, SR_DS_RUNNING)) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_SWITCH_DATASTORE_FAILED_STR);
            free(path);
            return EXIT_FAILURE;
        }

        /* copy config from startup to running datastore */
        if (SR_ERR_OK != sr_copy_config(session, ACM_MODULE_STR, SR_DS_STARTUP, 0, 0)) {
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, ERR_COPY_DATASTORE_FAILED_STR);
            free(path);
            return EXIT_FAILURE;
        }

        fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (-1 == fd) {
            SRP_LOG_ERR("Unable to create file %s.", path);
            free(path);
            close(fd);
            return EXIT_FAILURE;
        }
        close(fd);
    }
    if (path) {
        free(path);
        path = NULL;
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Creates a new container bypass1 or bypass2 inside container acm inside acm yang module.
 * This function is for state data from container 'acm'.
 *
 * @param[in]  parent           Pointer to an existing parent of the requested nodes. Is NULL for top-level nodes. Caller is supposed to append the requested nodes to this data subtree and return either the original parent or a top-level node.
 * @param[in]  index            Index of the module of ACM to read the status from.
 * @param[in]  container_name   Name of the container to be created.
 * @return      Error code (SR_ERR_OK on success).
 */
static int new_acm_state(struct lyd_node **parent, int index, char *container_name)
{
    char path[MAX_STR_LEN] = {0};
    char tmp[MAX_STR_LEN] = {0};
    int64_t acm_status;
    enum acm_status_item i;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    for (i = STATUS_HALT_ERROR_OCCURED; i < STATUS_ITEM_NUM; i++) {
        acm_status = acm_read_status_item(index, i);

        if (acm_status < 0) {
            SRP_LOG_ERR(ERR_ACM_STATUS_MODULE_STR, index, i, acm_status);
            return SR_ERR_OPERATION_FAILED;
        }

        switch (i) {
        case STATUS_HALT_ERROR_OCCURED:
            snprintf(tmp, MAX_STR_LEN, "%s", (acm_status ? "true" : "false"));
            fill_xpath(path, ACM_HALT_ON_ERROR_OCCURED_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_HALT_ERR_OCCURED_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_IP_ERROR_FLAGS:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_IP_ERROR_FLAGS_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_IP_ERROR_FLAG_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }

            break;
        case STATUS_POLICING_ERROR_FLAGS:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_POLICING_ERROR_FLAGS_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_POLICING_ERROR_FLAGS_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_RUNT_FRAMES_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_RUNT_FRAMES_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_RUNT_FRAMES_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_MII_ERRORS_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_MII_ERRORS_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_MII_ERRORS_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_GMII_ERROR_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_GMII_ERROR_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_GMII_ERROR_PREV_CYCLE_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_SOF_ERRORS_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_SOF_ERRORS_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_SOF_ERRORS_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_LAYER7_MISSMATCH_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_LAYER7_MISMATCH_CNT_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_LAYER_MISMATCH_CNT_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_DROPPED_FRAMES_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_DROPPED_FRAMES_CNT_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_DROPED_FRAMES_CNT_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_FRAMES_SCATTERED_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_FRAMES_SCATTERED_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_SCATTER_DMAF_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_DISABLE_OVERRUN_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_DISABLE_OVERRUN_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_DISABLE_OVERRRUN_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_TX_FRAMES_CYCLE_CHANGE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_TX_FRAMES_CYCLE_CHANGE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_TX_FRAMES_CYCLE_CHANGE_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_TX_FRAMES_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_TX_FRAMES_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_TX_FRAMES_PREV_CYCLE_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_RX_FRAMES_CYCLE_CHANGE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_RX_FRAMES_CYCLE_CHANGE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_RX_FRAMES_CYCLE_CHANGE_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        case STATUS_RX_FRAMES_PREV_CYCLE:
            snprintf(tmp, MAX_STR_LEN, "%lld", acm_status);
            fill_xpath(path, ACM_RX_FRAMES_PREV_CYCLE_XPATH, container_name);
            if (NULL == lyd_new_path(*parent, NULL, path, tmp, 0, 0)) {
                SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_RX_FRAMES_PREV_CYCLE_STR, ERR_NOT_CREATED_ELEMENT_STR);
            }
            break;
        default:
            SRP_LOG_ERR("Incorrect status ID: %d", i);
            break;
        }
    }

    return SR_ERR_OK;
}

/**
 * @brief Callback to be called when operational data of container acm is requested.
 * Subscribe to it by sr_oper_get_items_subscribe call.
 *
 * @param[in]  session           Implicit session (do not stop) with information about the event originator session IDs.
 * @param[out] module_name       Name of the affected module.
 * @param[out] path              Path identifying the subtree that is supposed to be provided, same as the one used for the subscription.
 * @param[in]  request_xpath     XPath as requested by a client. Can be NULL.
 * @param[in]  request_id        Request ID unique for the specific module_name.
 * @param[in]  parent            Pointer to an existing parent of the requested nodes. Is NULL for top-level nodes. Caller is supposed to append the requested nodes to this data subtree and return either the original parent or a top-level node.
 * @param[in]  private_data      Private context opaque to sysrepo, as passed to sr_oper_get_items_subscribe call.
 * @return      Error code (SR_ERR_OK on success).
 */
static int acm_state_cb(sr_session_ctx_t *session, const char *module_name, const char *path, const char *request_xpath, uint32_t request_id, struct lyd_node **parent, void *private_data)
{
    (void)session;
    (void)request_id;
    (void)module_name;
    (void)request_xpath;
    (void)request_id;
    (void)private_data;
    (void)path;
    int bypass_index = 0;
    const char* lib_version = NULL;
    const char* ip_version = NULL;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    lib_version = acm_read_lib_version();
    ip_version = acm_read_ip_version();

    /* container acm, leaf acm-lib-version */
    /* this need to be done like this because *parent is NULL inside this callback
     * so we are actually creating container acm-state sto other leafs can be filled up */
    *parent = lyd_new_path(*parent, sr_get_context(sr_session_get_connection(session)), ACM_LIB_VERSION_XPATH, (char*)lib_version, 0, 0);
    if (NULL == *parent) {
        SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_LIB_VERSION_STR, ERR_NOT_CREATED_ELEMENT_STR);
    }

    /* container acm, leaf acm-ip-version */
    if (NULL == lyd_new_path(*parent, NULL, ACM_IP_VERSION_XPATH, (char*)ip_version, 0, 0)) {
        SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_IP_VERSION_STR, ERR_NOT_CREATED_ELEMENT_STR);
    }

    /* container acm, container bypass1 */
    if (SR_ERR_OK != new_acm_state(parent, bypass_index, ACM_BYPASS_1_STR)) {
        SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_BYPASS_1_STR, ERR_NOT_CREATED_ELEMENT_STR);
    }

    bypass_index = 1;
    /* container acm, container bypass2 */
    if (SR_ERR_OK != new_acm_state(parent, bypass_index, ACM_BYPASS_2_STR)) {
        SRP_LOG_INF(ERROR_MSG_FUN_NODE_EL_AND_MSG_STR, __func__, ACM_BYPASS_2_STR, ERR_NOT_CREATED_ELEMENT_STR);
    }

    return SR_ERR_OK;
}

/**
 * @ingroup acminternal
 * @brief Parse configuration to determine base time.
 *
 * The function parses through configuration with Yang model objects in order to determine
 * the base time (start) of a ACM bypass module configuration.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[out]  time                    Time in seconds and nanoseconds.
 * @param[in]   container_name  Name of the main container.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_base_time(sr_session_ctx_t *session, struct timespec *time, char* container_name)
{
    int i = 0;
    sr_val_t* base_time = NULL;
    size_t counter = 0;
    char path[MAX_STR_LEN] = {0};

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    fill_xpath(path, ACM_BASE_TIME_XPATH, container_name);

    if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &base_time, &counter)) {
        /* iterate trough bypass1 or bypass2 container */
        for (i=0; i<(int)counter; i++) {
            /* if leaf seconds inside container base-time is found */
            if (true == sr_xpath_node_name_eq(base_time[i].xpath, ACM_SECONDS_STR)) {
                time->tv_sec = base_time[i].data.uint64_val;
            }

            /* if leaf fractional-seconds inside container base-time is found */
            if (true == sr_xpath_node_name_eq(base_time[i].xpath, ACM_FRACTIONAL_SECONDS_STR)) {
                time->tv_nsec = base_time[i].data.uint64_val;
            }
        }
    }

    sr_free_values(base_time, counter);

    return EXIT_SUCCESS;
}

/**
 * @brief Parse configuration for control (schedule) parameters of ACM module.
 *
 * The function parses through configuration with Yang model objects in order to determine
 * control parameters of a ACM bypass module.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   container               Array of sr_val_t structures representing container.
 * @param[in]   counter                         Number of elements inside container.
 * @param[out]  connection_mode     ACM connection mode (serial or parallel).
 * @param[out]  speed               Link speed.
 * @param[out]  sched_cycle_time    Duration of schedule cycle.
 * @param[out]  base_time           Starting time of the configuration.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_control_node(sr_session_ctx_t *session, sr_val_t* container, size_t counter, enum acm_connection_mode *connection_mode, enum acm_linkspeed *speed, uint32_t *sched_cycle_time, struct timespec *base_time)
{
    int i = 0;
    int ret = 0;
    char* container_name = NULL;
    (void)session;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* set default values */
    *connection_mode = CONN_MODE_PARALLEL;
    *speed = SPEED_1GBps;

    /* iterate trough bypass1 or bypass2 container */
    for (i=0; i<(int)counter; i++) {
        /* if leaf connection-mode is found */
        if (true == sr_xpath_node_name_eq(container[i].xpath, ACM_CONNECT_MODE_STR)) {
            if (0 == strncmp("parallel", container[i].data.enum_val, MAX_STR_LEN)) {
                *connection_mode = CONN_MODE_PARALLEL;
            }
            if (0 == strncmp("serial", container[i].data.enum_val, MAX_STR_LEN)) {
                *connection_mode = CONN_MODE_SERIAL;
            }
        }

        /* if leaf link-speed is found */
        if (true == sr_xpath_node_name_eq(container[i].xpath, ACM_LINK_SPEED_STR)) {
            if (0.100 == container[i].data.decimal64_val) {
                *speed = SPEED_100MBps;
            }
            else if (1.000 == container[i].data.decimal64_val) {
                *speed = SPEED_1GBps;
            } else {
                SRP_LOG_ERR("Only Linkspeed 1.000 or 0.100 supported. Speed %f is incorrect.", container[i].data.decimal64_val);
                //sr_set_error(session, container[i].xpath, "Only Linkspeed 1.000 or 0.100 supported. Speed is incorrect.");
                return -EINVAL;
            }
        }
        /* if leaf cycle-time is found */
        if (true == sr_xpath_node_name_eq(container[i].xpath, ACM_CYCLE_TIME_STR)) {
            *sched_cycle_time = container[i].data.uint32_val;
        }

        /* if container base-time is found */
        if (true == sr_xpath_node_name_eq(container[i].xpath, ACM_BASE_TIME_STR)) {
            if (NULL != strstr(container[i].xpath, "bypass1")) {
                container_name = "bypass1";
            }
            else if (NULL != strstr(container[i].xpath, "bypass2")) {
                container_name = "bypass2";
            }

            /* container[i].xpath can not be send to new_base_time function to use it to get
             * container leafs, because xpath needs to contain /* at the end. */
            ret = new_base_time(session, base_time, container_name);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_base_time returns %d.", ret);
                return ret;
            }
        }
    }

    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Instantiate new operation configuration.
 *
 * The function parses through XML structure with Yang model objects and sets up the stream
 * schedule configuration with libacmconfig.
 *
 * @param[in]   session                         Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   container_name          Pointer to char that represents container name.
 * @param[in]   stream_key              Pointer to char that represents key of stream entry.
 * @param[in]   sub_container_name  Pointer to char that represents subcontainer name.
 * @param[in]   operation_key           Pointer to char that represents key of stream-operations entry.
 * @param[in]   stream                  Pointer to the parent stream configuration.
 * @param[in]   constant_data           Pointer to constant data for the operation.
 * @param[in]   constant_size           Size of constant data.
 * @param[in]   type                    Type of the parent stream.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_stream_ops(sr_session_ctx_t *session, char *container_name, char *stream_key, char* sub_container_name, char *operation_key, struct acm_stream *stream, char *constant_data, uint16_t constant_size, STREAM_TYPE type)
{
    STREAM_OPCODE opcode = {0};   /* default value */
    char        bufname[ACM_MAX_NAME_SIZE] = {0};
    uint16_t    offset = 0;
    uint16_t    length = 0;
    int ret;
    char err_msg[MAX_STR_LEN] = {0};
    char path[MAX_STR_LEN] = {0};
    sr_val_t* stream_operations_entry = NULL;
    size_t counter = 0;
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    memset(bufname, 0, sizeof(bufname));

    /* get stream-operations list entry */
    fill_xpath(path, ACM_LIST_STREAM_OPERATION_XPATH, container_name, stream_key, sub_container_name, operation_key);
    if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &stream_operations_entry, &counter)) {
        for (i=0; i<(int)counter; i++) {
            parse_stream_ops_opcode(stream_operations_entry[i], &opcode, OPC_READ);
            parse_stream_ops_offset(stream_operations_entry[i], &offset);
            parse_stream_ops_length(stream_operations_entry[i], &length);
            parse_stream_ops_bufname(stream_operations_entry[i], bufname);
        }
    } else {
        snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_STREAM_OPERATIONS_STR);
        SRP_LOG_ERRMSG(err_msg);
        //sr_set_error(session, path, err_msg);
        return EXIT_FAILURE;
    }

    sr_free_values(stream_operations_entry, counter);

    /* add operation to stream */
    switch (opcode) {
    case OPC_READ:
        ret = acm_add_stream_operation_read(stream, offset, length, bufname);
        if (ret != 0) {
            SRP_LOG_ERR("Cannot add read operation. Error: %d", ret);
            return ret;
        }
        break;
    case OPC_INSERT:
        ret = acm_add_stream_operation_insert(stream, length, bufname);
        if (ret != 0) {
            SRP_LOG_ERR("Cannot add insert operation. Error: %d", ret);
            return ret;
        }
        break;
    case OPC_PADDING:
        ret = acm_add_stream_operation_pad(stream, length, (const char) 0x00);
        if (ret != 0) {
            SRP_LOG_ERR("Cannot add pad operation. Error: %d", ret);
            return ret;
        }
        break;
    case OPC_CONST_INSERT:
        if (offset + length > constant_size) {
            SRP_LOG_ERRMSG("Data length too large for insert-constant operation.");
            return -EACMYOPERCONSTLEN;
        }
        ret = acm_add_stream_operation_insertconstant(stream, constant_data + offset, length);
        if (ret != 0) {
            SRP_LOG_ERR("Cannot add insert constant operation. Error %d", ret);
            return ret;
        }
        break;
    case OPC_FORWARD:
        if (type == STREAM_INGRESS_TRIGGERED) {
            ret = acm_add_stream_operation_forwardall(stream);
            if (ret != 0) {
                SRP_LOG_ERR("Cannot add forwardall operation. Error: %d", ret);
                return ret;
            }
        } else {
            ret = acm_add_stream_operation_forward(stream, offset, length);
            if (ret != 0) {
                SRP_LOG_ERR("Cannot add forward operation. Error: %d", ret);
                return ret;
            }
        }
        break;
    default:
        SRP_LOG_ERRMSG("Incorrect operation type.");
        return -EACMYOPERTYPE;
    }

    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Instantiate new ingress stream configuration.
 *
 * The function parses through XML structure with Yang model objects and sets up the ingress stream
 * configuration with libacmconfig.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   module          Pointer to the parent ACM module configuration.
 * @param[in]   stream_entry    Array of sr_val_t structures representing container ingress-stream.
 * @param[in]   counter         Number of elements inside entry.
 * @param[out]  p_stream        Pointer to the created stream instance.
 * @param[in]   stream_key      Pointer to char that represents key of stream entry.
 * @param[in]   container_name  Pointer to char that represents container name.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_ingress_stream(sr_session_ctx_t *session, struct acm_module *module, sr_val_t* stream_entry, int counter, struct acm_stream **p_stream, char *stream_key, char *container_name)
{
    char        *pos;
    int          tmp_stream_ops = 0;
    uint8_t      addfilt_mask[ACM_MAX_FILTER_SIZE];
    uint8_t      addfilt_patt[ACM_MAX_FILTER_SIZE];
    uint8_t      header[ACM_MAX_LOOKUP_SIZE];
    uint8_t      header_mask[ACM_MAX_LOOKUP_SIZE];
    //uint16_t     vlan;
    uint16_t     addfilt_size = 0;
    uint16_t     num_stream_ops = 0;
    struct acm_stream *stream;
    bool has_redundancy_tag = false;
    uint16_t    seq_rec_reset_ms = 0;
    int ret;
    int i = 0;
    sr_xpath_ctx_t st = {0};
    char operation_key[MAX_STR_LEN] = {0};

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if (p_stream == NULL) {
        SRP_LOG_ERRMSG("Provided p_stream is NULL.");
        return -EPERM;
    }

#if 0
    /* The default value for ingress_stream->vlan is not defined in the yang module.
     * So, this is the default value. If there is the correct value in the config,
     * this '-1' will be overwriten.
     *  */
    vlan = -1;
#endif

    memset(addfilt_mask, 0, sizeof(addfilt_mask));
    memset(addfilt_patt, 0, sizeof(addfilt_patt));

    /* go from ingress-stream container */
    for (i=0; i<(int)counter; i++) {
        /* copy frame header and mask */
        parse_hex_string(stream_entry[i], header, ACM_HEADER_STR, ACM_MAX_LOOKUP_SIZE);
        parse_hex_string(stream_entry[i], header_mask, ACM_HEADER_MASK_STR, ACM_MAX_LOOKUP_SIZE);

        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_ADD_FILTER_SIZE_STR)) {
            addfilt_size = stream_entry[i].data.uint16_val;
        }

        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_ADD_FILTER_MASK_STR)) {
            int k;
            int len;

            pos = stream_entry[i].data.string_val;
            len = strlen(stream_entry[i].data.string_val)/2;
            for (k = 0; k < len; k++) {
                sscanf(pos,"%2hhx", &addfilt_mask[k]);
                pos+=2;
            }
        }

        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_ADD_FILTER_PATTERN_STR)) {
            int k;
            int len;

            pos = stream_entry[i].data.string_val;
            len = strlen(stream_entry[i].data.string_val)/2;
            for (k = 0; k < len; k++) {
                sscanf(pos,"%2hhx", &addfilt_patt[k]);
                pos+=2;
            }
        }

        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_HAS_REDUNDANCY_TAG_STR)) {
            if (stream_entry[i].data.bool_val) {
                has_redundancy_tag = true;
            } else {
                has_redundancy_tag = false;
            }
        }

        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_SEQ_RECOVERY_RESET_MS_STR)) {
            seq_rec_reset_ms = stream_entry[i].data.uint16_val;
        }
    }

    /* create stream */
    stream = acm_create_ingress_triggered_stream(header, header_mask, addfilt_patt, addfilt_mask, addfilt_size);

    if (stream == NULL) {
        SRP_LOG_ERRMSG("Cannot create ingress triggered stream.");
        return -EACMYITSTREAMCREATE;
    }

    if (has_redundancy_tag) {
        /* set individual recovery */
        ret = acm_set_rtag_stream(stream, seq_rec_reset_ms);
        if (ret != 0) {
            SRP_LOG_ERR("Setting of RTAG for ingress stream failed. Error: %d", ret);
            acm_destroy_stream(stream);
            return ret;
        }
    }

    /* add stream to module */
    ret = acm_add_module_stream(module, stream);
    if (ret != 0) {
        SRP_LOG_ERR("Cannot add ingress triggered stream. Error: %d", ret);
        acm_destroy_stream(stream);
        return ret;
    }

    /* go from ingress-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_stream_num_of_operations(stream_entry[i], &num_stream_ops);

        /* if stream-operations entry is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/ingress-stream/stream-operations[operation-key='0'] (list instance))*/
        if (NULL != strstr(stream_entry[i].xpath, "stream-operations[operation-key")) {
            /* get operation-key so inside new_stream_ops function we can get right entry of list */
            if (EXIT_SUCCESS != get_key_value(NULL, stream_entry[i].xpath, ACM_STREAM_OPERATIONS_STR, ACM_OPER_KEY_STR, &st, operation_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_OPER_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_stream_ops(session, container_name, stream_key, ACM_INGRESS_STREAM_STR, operation_key, stream, NULL, 0, STREAM_INGRESS_TRIGGERED);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_stream_ops returns %d.", ret);
                return ret;
            }
            tmp_stream_ops++;
        }
    }

    /* check num_stream_ops is equal to tmp_stream_ops */
    if (tmp_stream_ops != num_stream_ops) {
        SRP_LOG_ERRMSG("Incorrect number of operations.");
        return -EACMYOPERNUMBER;
    }

    *p_stream = stream;

    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Instantiate new egress stream configuration.
 *
 * The function parses through configuration with Yang model objects and sets up the egress stream
 * configuration with libacmconfig.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   module          Pointer to the parent ACM module configuration.
 * @param[in]   stream_entry    Array of sr_val_t structures representingcontainer time-triggered-stream.
 * @param[in]   counter         Number of elements inside entry.
 * @param[out]  p_stream        Pointer to the created stream instance.
 * @param[in]   stream_key      Pointer to char that represents key of stream entry.
 * @param[in]   container_name  Pointer to char that represents container name.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_egress_stream(sr_session_ctx_t *session, struct acm_module *module, sr_val_t* stream_entry, int counter, struct acm_stream **p_stream, char *stream_key, char *container_name)
{
    uint8_t    dmac[ETHER_ADDR_LEN];
    uint8_t    smac[ETHER_ADDR_LEN];
    int tmp_stream_ops = 0;
    uint16_t  vlan = 0;
    uint8_t   prio = 0;
    uint16_t  num_stream_ops = 0;
    //uint8_t   addhdr_data[ACM_MAX_FRAME_SIZE];
    char addhdr_data[ACM_MAX_FRAME_SIZE] = {0};
    uint16_t  addhdr_size = 0;
    int ret;
    struct acm_stream *stream = NULL;
    sr_xpath_ctx_t st = {0};
    char operation_key[MAX_STR_LEN] = {0};
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if (p_stream == NULL) {
        SRP_LOG_ERRMSG("Provided p_stream is NULL.");
        return -EPERM;
    }

    /* The default value for egress_stream->vlan is not defined in the yang module.
     * So, this is the default value. If there is the correct value in the config,
     * this '-1' will be overwriten.
     *  */
    vlan = -1;

    /* Set addhdr_data to default values */
    memset(addhdr_data, 0, sizeof(addhdr_data));

    /* go from time-triggered-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_mac_addr(stream_entry[i], smac, ACM_SOURCE_ADDRESS_STR);
        parse_mac_addr(stream_entry[i], dmac, ACM_DEST_ADDRESS_STR);
        parse_stream_vid(stream_entry[i], &vlan);
        parse_stream_priority(stream_entry[i], &prio);
        parse_stream_const_data_size(stream_entry[i], &addhdr_size);
        parse_hex_string_add_const_data(stream_entry[i], addhdr_data, ACM_ADD_CONST_DATA_STR, addhdr_size);
    }

    /* create stream */
    stream = acm_create_time_triggered_stream(dmac, smac, vlan, prio);
    if (stream == NULL) {
        SRP_LOG_ERRMSG("Cannot create time triggered stream.");
        return -EACMYTTSTREAMCREATE;
    }

    /* add stream to module */
    ret = acm_add_module_stream(module, stream);
    if (ret != 0) {
        SRP_LOG_ERR("Cannot add time triggered stream. Error: %d", ret);
        acm_destroy_stream(stream);
        return ret;
    }

    /* go from time-triggered-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_stream_num_of_operations(stream_entry[i], &num_stream_ops);

        /* if stream-operations entry is found (xpath will be for examle /acm:acm/bypass2/stream[stream-key='1']/time-triggered-stream/stream-operations[operation-key='0'] (list instance)) */
        if (NULL != strstr(stream_entry[i].xpath, "stream-operations[operation-key")) {
            /* get operation-key so inside new_stream_ops function we can get right entry of list */
            if (EXIT_SUCCESS != get_key_value(NULL, stream_entry[i].xpath, ACM_STREAM_OPERATIONS_STR, ACM_OPER_KEY_STR, &st, operation_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_OPER_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_stream_ops(session, container_name, stream_key, ACM_EGRESS_STREAM_STR, operation_key, stream, addhdr_data, addhdr_size, STREAM_TIME_TRIGGERED);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_stream_ops returns EXIT_FAILURE. Error: %d", ret);
                return ret;
            }
            tmp_stream_ops++;
        }
    }

    /* check num_stream_ops is equal to tmp_stream_ops */
    if (tmp_stream_ops != num_stream_ops) {
        SRP_LOG_ERRMSG("Wrong NumberOfOperations.");
        return -EACMYOPERNUMBER;
    }

    *p_stream = stream;
    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Instantiate new event stream configuration.
 *
 * The function parses through configuration with Yang model objects and sets up the event stream
 * configuration with libacmconfig.
 *
 * @param[in]   session                         Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   module                  Pointer to the parent ACM module configuration.
 * @param[in]   stream_entry            Array of sr_val_t structures representing container event stream.
 * @param[in]   counter                 Number of elements inside entry.
 * @param[in]   ingress_stream          Pointer to ingress stream to which the event stream belongs to.
 * @param[out]  p_stream                Pointer to the created stream instance.
 * @param[in]   stream_key              Pointer to char that represents key of stream entry.
 * @param[in]   container_name          Pointer to char that represents container name.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_event_stream(sr_session_ctx_t *session, struct acm_module *module, sr_val_t* stream_entry, int counter, struct acm_stream *ingress_stream , struct acm_stream **p_stream, char *stream_key, char *container_name)
{
    (void)module;
    uint8_t    dmac[ETHER_ADDR_LEN] = {0};
    uint8_t    smac[ETHER_ADDR_LEN] = {0};
    uint16_t   vlan = 0;
    uint8_t    prio = 0;
    //uint8_t    addhdr_data[ACM_MAX_FRAME_SIZE] = {0};
    char addhdr_data[ACM_MAX_FRAME_SIZE] = {0};
    uint16_t   addhdr_size = 0;
    uint16_t   num_stream_ops = 0;
    int        tmp_stream_ops = 0;
    struct acm_stream *stream = NULL;
    int ret;
    sr_xpath_ctx_t st = {0};
    char operation_key[MAX_STR_LEN] = {0};
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if (p_stream == NULL) {
        SRP_LOG_ERRMSG("Provided p_stream is NULL.");
        return -EPERM;
    }

    /* The default value for event_stream->vlan is not defined in the yang module.
     * So, this is the default value. If there is the correct value in the config,
     * this '-1' will be overwriten.
     *  */
    vlan = -1;

    /* Set addhdr_data to default values */
    memset(addhdr_data, 0, sizeof(addhdr_data));

    /* go event-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_mac_addr(stream_entry[i], smac, ACM_SOURCE_ADDRESS_STR);
        parse_mac_addr(stream_entry[i], dmac, ACM_DEST_ADDRESS_STR);
        parse_stream_vid(stream_entry[i], &vlan);
        parse_stream_priority(stream_entry[i], &prio);
        parse_stream_const_data_size(stream_entry[i], &addhdr_size);
        parse_hex_string_add_const_data(stream_entry[i], addhdr_data, ACM_ADD_CONST_DATA_STR, addhdr_size);
    }

    /* create stream */
    stream = acm_create_event_stream(dmac, smac, vlan, prio);
    if (stream == NULL) {
        SRP_LOG_ERRMSG("Cannot create event stream.");
        return -EACMYESTREAMCREATE;
    }

    ret = acm_set_reference_stream(ingress_stream, stream);
    if (ret != 0) {
        SRP_LOG_ERR("Cannot create reference to ingress stream. Error: %d", ret);
        return ret;
    }

    /* go from event-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_stream_num_of_operations(stream_entry[i], &num_stream_ops);

        /* if stream-operations entry is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/event-stream/stream-operations[operation-key='0'] (list instance))*/
        if (NULL != strstr(stream_entry[i].xpath, "stream-operations[operation-key")) {
            /* get operation-key so inside new_stream_ops function we can get right entry of list */
            if (EXIT_SUCCESS != get_key_value(NULL, stream_entry[i].xpath, ACM_STREAM_OPERATIONS_STR, ACM_OPER_KEY_STR, &st, operation_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_OPER_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_stream_ops(session, container_name, stream_key, ACM_EVENT_STREAM_STR, operation_key, stream, addhdr_data, addhdr_size, STREAM_EVENT);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_stream_ops returns failed. Error: %d", ret);
                return ret;
            }
            tmp_stream_ops++;
        }
    }

    /* check num_stream_ops is equal to tmp_stream_ops */
    if (tmp_stream_ops != num_stream_ops) {
        SRP_LOG_ERRMSG("Wrong NumberOfOperations.");
        return -EACMYOPERNUMBER;
    }

    *p_stream = stream;
    return EXIT_SUCCESS;

}

/**
 * @ingroup acminternal
 * @brief Instantiate new egress recovery stream configuration.
 *
 * The function parses through configuration with Yang model objects and sets up the egress recovery
 * stream configuration with libacmconfig.
 *
 * @param[in]   session                         Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   module                  Pointer to the parent ACM module configuration.
 * @param[in]   stream_entry            Array of sr_val_t structures representing container egress-stream-recovery.
 * @param[in]   counter                 Number of elements inside entry.
 * @param[in]   parent_stream       Pointer to the parent stream instance.
 * @param[in]   stream_key              Pointer to char that represents key of stream entry.
 * @param[in]   container_name          Pointer to char that represents container name.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_egress_stream_recovery(sr_session_ctx_t *session, struct acm_module *module, sr_val_t* stream_entry, int counter,  struct acm_stream *parent_stream,  char *stream_key, char *container_name)
{
    (void)module;
    int tmp_stream_ops = 0;
    uint16_t   vlan = 0;
    uint8_t    prio = 0;
    //uint8_t    addhdr_data[ACM_MAX_FRAME_SIZE] = {0};
    char addhdr_data[ACM_MAX_FRAME_SIZE] = {0};
    uint16_t   addhdr_size = 0;
    uint8_t    dmac[ETHER_ADDR_LEN] = {0};
    uint8_t    smac[ETHER_ADDR_LEN] = {0};
    struct acm_stream *stream = NULL;
    uint16_t   num_stream_ops = 0;
    int ret;
    sr_xpath_ctx_t st = {0};
    char operation_key[MAX_STR_LEN] = {0};
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* The default value for egress_stream_recovery->vlan is not defined in the yang module.
     * So, this is the default value. If there is the correct value in the config,
     * this '-1' will be overwriten.
     */
    vlan = -1;

    /* Set egress_stream_recovery->addhdr_data to default values */
    memset(addhdr_data, 0, sizeof(addhdr_data));

    /* go event-stream container */
    for (i=0; i<(int)counter; i++) {
        parse_mac_addr(stream_entry[i], smac, ACM_SOURCE_ADDRESS_STR);
        parse_mac_addr(stream_entry[i], dmac, ACM_DEST_ADDRESS_STR);
        parse_stream_vid(stream_entry[i], &vlan);
        parse_stream_priority(stream_entry[i], &prio);
        parse_stream_const_data_size(stream_entry[i], &addhdr_size);
        parse_hex_string_add_const_data(stream_entry[i], addhdr_data, ACM_ADD_CONST_DATA_STR, addhdr_size);
    }

    /* create stream */
    stream = acm_create_recovery_stream(dmac, smac, vlan, prio);
    if (stream == NULL) {
        SRP_LOG_ERRMSG("Cannot create recovery stream.");
        return -EACMYRSTREAMCREATE;
    }

    /* go from egress-stream-recovery container */
    for (i=0; i<(int)counter; i++) {
        /* if leaf operation-list-length */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_OPER_LIST_LENGTH_STR)) {
            num_stream_ops = stream_entry[i].data.uint16_val;
        }
        /* if stream-operations entry is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/egress-stream-recovery/stream-operations[operation-key='0'] (list instance))*/
        if (NULL != strstr(stream_entry[i].xpath, "stream-operations[operation-key")) {
            /* get operation-key so inside new_stream_ops function we can get right entry of list */
            if (EXIT_SUCCESS != get_key_value(NULL, stream_entry[i].xpath, ACM_STREAM_OPERATIONS_STR, ACM_OPER_KEY_STR, &st, operation_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_OPER_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_stream_ops(session, container_name, stream_key, ACM_EGRESS_STREAM_RECOVERY_STR, operation_key, stream, addhdr_data, addhdr_size, STREAM_RECOVERY);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_stream_ops failed. Error: %d", ret);
                return ret;
            }
            tmp_stream_ops++;
        }
    }

    /* add reference to the Event stream */
    ret = acm_set_reference_stream(parent_stream, stream);
    if (ret != 0) {
        SRP_LOG_ERR("cannot set stream reference. Error: %d", ret);
        return ret;
    }

    /* check num_stream_ops is equal to tmp_stream_ops */
    if (tmp_stream_ops != num_stream_ops) {
        SRP_LOG_ERRMSG("Wrong NumberOfOperations.");
        return -EACMYOPERNUMBER;
    }

    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Parse configuration for parameters of schedule window.
 *
 * The function parses through configuration with Yang model objects in order to determine
 * configuration parameters of the schedule window for a specific stream operation.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   container_name  Pointer to char that represents container name.
 * @param[in]   stream_key      Pointer to char that represents key of stream entry.
 * @param[in]   schedule_key     Pointer to char that represents key of schedule-events entry.
 * @param[out]  period          schedule period
 * @param[out]  start           start time
 * @param[out]  end             end time
 *
 * @return EXIT_SUCCESS or error code
 */
int new_shedule_window(sr_session_ctx_t *session, char *container_name,char *stream_key, char *schedule_key, uint32_t *period, uint32_t *start, uint32_t *end)
{
    char err_msg[MAX_STR_LEN] = {0};
    char path[MAX_STR_LEN] = {0};
    sr_val_t* schedule_events_entry = NULL;
    size_t counter = 0;
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* get schedule-events list entry */
    fill_xpath(path, ACM_LIST_SCHEDULE_EVENTS_XPATH, container_name, stream_key, schedule_key);
    if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &schedule_events_entry, &counter)) {
        for (i=0; i<(int)counter; i++) {
            /* if leaf period is found */
            if (true == sr_xpath_node_name_eq(schedule_events_entry[i].xpath, ACM_PERIOD_STR)) {
                *period = schedule_events_entry[i].data.uint32_val;
            }

            /* if leaf time-start is found */
            if (true == sr_xpath_node_name_eq(schedule_events_entry[i].xpath, ACM_TIME_START_STR)) {
                *start = schedule_events_entry[i].data.uint32_val;
            }

            /* if leaf time-end is found */
            if (true == sr_xpath_node_name_eq(schedule_events_entry[i].xpath, ACM_TIME_END_STR)) {
                *end = schedule_events_entry[i].data.uint32_val;
            }
        }
    } else {
        snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_SCHEDULE_ENENTS_STR);
        SRP_LOG_ERRMSG(err_msg);
        //sr_set_error(session, path, err_msg);
        return EXIT_FAILURE;
    }
    sr_free_values(schedule_events_entry, counter);

    return EXIT_SUCCESS;
}

/**
 * @ingroup acminternal
 * @brief Instantiate new stream schedule configuration.
 *
 * The function parses through XML structure with Yang model objects and sets up the stream
 * schedule configuration with libacmconfig.
 *
 * @param[in]   session                         Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   stream_entry            Array of sr_val_t structures representing container stream-schedule.
 * @param[in]   counter                 Number of elements inside entry.
 * @param[in]   stream                  Pointer to the parent stream configuration.
 * @param[in]   is_ingress          Flag to indicate whether the parent stream is ingress (if true).
 * @param[in]   stream_key              Pointer to char that represents key of stream entry.
 * @param[in]   container_name          Pointer to char that represents container name.
 * @return EXIT_SUCCESS or error code
 */
int new_stream_schedule(sr_session_ctx_t *session, sr_val_t* stream_entry, int counter, struct acm_stream *stream, bool is_ingress, char *stream_key, char *container_name)
{
    uint32_t    start;
    uint32_t    end;
    uint32_t    period;
    uint16_t    num_windows = 0;
    int         ret;
    sr_xpath_ctx_t st = {0};
    char schedule_key[MAX_STR_LEN] = {0};
    int i = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* go event-stream container */
    for (i=0; i<(int)counter; i++) {
        /* if leaf operation-list-length */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_SCHEDULE_EVENTS_LENGTH_STR)) {
            num_windows = stream_entry[i].data.uint16_val;
        }

        (void)num_windows; // had to put this here because compile error "variable 'num_windows' set but not used"

        /* if schedule-events entry is found (xpath will be for example (/acm:acm/bypass2/stream[stream-key='1']/stream-schedule/schedule-events[schedule-key='0'] (list instance)))*/
        if (NULL != strstr(stream_entry[i].xpath, "schedule-events[schedule-key")) {
            /* get schedule-key so inside new_shedule_window function we can get right entry of list */
            if (EXIT_SUCCESS != get_key_value(NULL, stream_entry[i].xpath, ACM_SCHEDULE_ENENTS_STR, ACM_SCHEDULE_KEY_STR, &st, schedule_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_SCHEDULE_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_shedule_window(session, container_name, stream_key, schedule_key, &period, &start, &end);
            if (EXIT_SUCCESS != ret) {
                SRP_LOG_ERR("new_shedule_window failed. Error: %d", ret);
                return ret;
            }

            if (is_ingress) {
                ret = acm_add_stream_schedule_window(stream, period, start, end);
                if (EXIT_SUCCESS != ret) {
                    SRP_LOG_ERR("acm_add_stream_schedule_window failed. Error: %d", ret);
                    return ret;
                }
            } else {
                ret = acm_add_stream_schedule_event(stream, period, end);
                if (EXIT_SUCCESS != ret) {
                    SRP_LOG_ERR("acm_add_stream_schedule_event failed. Error: %d", ret);
                    return ret;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Instantiate new stream configuration.
 *
 * The function parses throughconfiguration with Yang model objects and sets up the stream
 * configuration with libacmconfig.
 *
 * @param[in]   session                 Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   module                  Pointer to the parent ACM module configuration.
 * @param[in]   container_name  Pointer to char that represents container name.
 * @param[in]   stream_key      Pointer to char that represents key of entry stream.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_stream_node(sr_session_ctx_t *session, struct acm_module *module, char *container_name, char *stream_key)
{
    char err_msg[MAX_STR_LEN] = {0};
    char path[MAX_STR_LEN] = {0};
    sr_val_t* stream_entry = NULL;
    size_t counter = 0;
    int stream_id;
    struct acm_stream *it_stream = NULL;    /* ingress triggered stream */
    struct acm_stream *tt_stream = NULL;     /* time triggered stream */
    struct acm_stream *e_stream = NULL;
    struct acm_stream *my_stream = NULL;
    struct acm_stream *redundant_stream = NULL;
    int ret;
    int i = 0;
    bool is_ingress;
    bool stream_id_flag = false;
    sr_val_t* event_stream = NULL;
    size_t event_stream_counter = 0;
    sr_val_t* egress_stream_recovery = NULL;
    size_t egress_stream_recovery_counter = 0;
    sr_val_t* stream_schedule = NULL;
    size_t stream_schedule_counter = 0;
    sr_val_t* egress_stream = NULL;
    size_t egress_stream_counter = 0;
    sr_val_t* ingress_stream = NULL;
    size_t ingress_stream_counter = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* get stream entry who has stream-key=stream_key */
    fill_xpath(path, ACM_LIST_STREAM_XPATH, container_name, stream_key);
    if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &stream_entry, &counter)) {
        for (i=0; i<(int)counter; i++) {
            /* find stream-key in stream list */
            if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_STREAM_KEY_STR)) {
                /* Take stream-key value. It will be used later. */
                stream_id = stream_entry[i].data.uint16_val;
                /* this indicates that stream-key leaf is found */
                stream_id_flag = true;
                break;
            }
        }
        /* here we check if there is a key of entry stream */
        if (false == stream_id_flag) {
            SRP_LOG_ERRMSG("No stream-key found.");
            return -EACMYNOSTREAMKEY;
        }
        /* set back flag to false */
        stream_id_flag = false;
    } else {
        snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_STREAM_LIST_STR);
        SRP_LOG_ERRMSG(err_msg);
        //sr_set_error(session, path, err_msg);
        return EXIT_FAILURE;
    }

    i = 0;

    for (i=0; i<(int)counter; i++) {
        /* if ingress-stream container is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/ingress-stream (container)) */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_INGRESS_STREAM_STR)) {
            /* now get every leaf and list instance inside stream list entry inside ingress-stream container */
            fill_xpath(path, ACM_LIST_STREAM_INGRESS_STREAM_CON_XPATH, container_name, stream_key);
            if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &ingress_stream, &ingress_stream_counter)) {
                if ((int)ingress_stream_counter) {
                    ret = new_ingress_stream(session, module, ingress_stream, ingress_stream_counter, &it_stream, stream_key, container_name);
                    if (EXIT_SUCCESS != ret) {
                        SRP_LOG_ERRMSG("new_ingress_stream failed.");
                        return ret;
                    }
                    /* check if redundant, store ID if not */
                    redundant_stream = tt_stream_list_find(stream_id);
                    if (redundant_stream != NULL) {
                        ret = acm_set_reference_stream(redundant_stream, it_stream);
                        if (ret != 0) {
                            SRP_LOG_ERR("Cannot set reference for redundant streams. Error: %d", ret);
                            return ret;
                        }
                    } else {
                        if (EXIT_SUCCESS != tt_stream_list_add(it_stream, stream_id)) {
                            SRP_LOG_ERRMSG("tt_stream_list_add failed.");
                            return -EACMYSTREAMLISTADD;
                        }
                    }
                }
                /* free values */
                sr_free_values(ingress_stream, ingress_stream_counter);
            } else {
                snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_INGRESS_STREAM_STR);
                SRP_LOG_ERRMSG(err_msg);
                //sr_set_error(session, path, err_msg);
                return EXIT_FAILURE;
            }
        }

        /* if time-triggered-stream container is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/time-triggered-stream (container)) */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_EGRESS_STREAM_STR)) {
            /* now get every leaf and list instance inside stream list entry inside time-triggered-stream container */
            fill_xpath(path, ACM_LIST_STREAM_TIME_TRIGGERED_ST_XPATH, container_name, stream_key);
            if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &egress_stream, &egress_stream_counter)) {
                if ((int)egress_stream_counter) {
                    ret = new_egress_stream(session, module, egress_stream, egress_stream_counter, &tt_stream, stream_key, container_name);
                    if (EXIT_SUCCESS != ret) {
                        SRP_LOG_ERRMSG("new_egress_stream failed.");
                        return ret;
                    }
                    /* check if redundant, store ID if not */
                    redundant_stream = tt_stream_list_find(stream_id);
                    if (redundant_stream != NULL) {
                        ret = acm_set_reference_stream(redundant_stream, tt_stream);
                        if (ret != 0) {
                            SRP_LOG_ERR("Cannot set reference for redundant streams. Error: %d", ret);
                            return ret;
                        }
                    } else {
                        if (EXIT_SUCCESS != tt_stream_list_add(tt_stream, stream_id)) {
                            SRP_LOG_ERRMSG("tt_stream_list_add failed.");
                            return -EACMYSTREAMLISTADD;
                        }
                    }
                }
                /* free values */
                sr_free_values(egress_stream, egress_stream_counter);
            } else {
                snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_EGRESS_STREAM_STR);
                SRP_LOG_ERRMSG(err_msg);
                //sr_set_error(session, path, err_msg);
                return EXIT_FAILURE;
            }
        }

        /* if event-stream container is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/event-stream (container)) */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_EVENT_STREAM_STR)) {
            /* now get every leaf and list instance inside stream list entry inside event-stream container */
            fill_xpath(path, ACM_LIST_STREAM_EVENT_STREAM_XPATH, container_name, stream_key);
            if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &event_stream, &event_stream_counter)) {
                if ((int)event_stream_counter) {
                    /* go event-stream container */
                    ret =  new_event_stream(session, module, event_stream, event_stream_counter, it_stream, &e_stream, stream_key, container_name);
                    if (EXIT_SUCCESS != ret) {
                        SRP_LOG_ERRMSG("new_event_stream failed.");
                        return ret;
                    }
                }
                /* free values */
                sr_free_values(event_stream, event_stream_counter);
            } else {
                snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_EVENT_STREAM_STR);
                SRP_LOG_ERRMSG(err_msg);
                //sr_set_error(session, path, err_msg);
                return EXIT_FAILURE;
            }
        }

        /* if egress-stream-recovery container is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/egress-stream-recovery (container)) */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_EGRESS_STREAM_RECOVERY_STR)) {
            /* now get every leaf and list instance inside stream list entry inside egress-stream-recovery container */
            fill_xpath(path, ACM_LIST_STREAM_EGGRESS_STREAM_REC_XPATH, container_name, stream_key);
            if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &egress_stream_recovery, &egress_stream_recovery_counter)) {
                if ((int)egress_stream_recovery_counter) {
                    ret =  new_egress_stream_recovery(session, module, egress_stream_recovery, egress_stream_recovery_counter, e_stream, stream_key, container_name);
                    if (EXIT_SUCCESS != ret) {
                        SRP_LOG_ERR("new_egress_stream_recovery failed. Error: %d", ret);
                        return ret;
                    }
                }
                /* free values */
                sr_free_values(egress_stream_recovery, egress_stream_recovery_counter);
            } else {
                snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_EGRESS_STREAM_RECOVERY_STR);
                SRP_LOG_ERRMSG(err_msg);
                //sr_set_error(session, path, err_msg);
                return EXIT_FAILURE;
            }
        }
    }

    /* Add stream schedule */
    if (it_stream != NULL) {
        /* we are dealing with ingress-triggered stream */
        my_stream = it_stream;
        is_ingress = true;
    } else {
        /* we are dealing with egress-triggered stream */
        my_stream = tt_stream;
        is_ingress = false;
    }

    for (i=0; i<(int)counter; i++) {
        /* if stream-schedule container is found (xpath will be for example /acm:acm/bypass2/stream[stream-key='1']/stream-schedule (container)) */
        if (true == sr_xpath_node_name_eq(stream_entry[i].xpath, ACM_STREAM_SCHEDULE_STR)) {
            /* now get every leaf and list instance inside stream list entry inside stream-schedule container */
            fill_xpath(path, ACM_LIST_STREAM_STREAM_SCHEDULE_XPATH, container_name, stream_key);
            if (SR_ERR_OK == sr_get_items(session, path, 0, 0, &stream_schedule, &stream_schedule_counter)) {
                if ((int)stream_schedule_counter) {
                    ret = new_stream_schedule(session, stream_schedule, stream_schedule_counter, my_stream, is_ingress, stream_key, container_name);
                    if (EXIT_SUCCESS != ret) {
                        SRP_LOG_ERR("new_stream_schedule failed. Error: %d", ret);
                        return ret;
                    }
                }
                /* free values */
                sr_free_values(stream_schedule, stream_schedule_counter);
            } else {
                snprintf(err_msg, MAX_STR_LEN, ERR_FAILED_GET_OBJ_STR, ACM_STREAM_SCHEDULE_STR);
                SRP_LOG_ERRMSG(err_msg);
                //sr_set_error(session, path, err_msg);
                return EXIT_FAILURE;
            }
        }
    }

    sr_free_values(stream_entry, counter);

    return EXIT_SUCCESS;
}

/**
 * @brief Instantiate new ACM module configuration.
 *
 * The function parses through configuration and sets up the ACM module
 * configuration with libacmconfig.
 *
 * @param[in]   session     Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   index           Index of the module of ACM.
 * @param[in]   container   Array of sr_val_t structures representing container.
 * @param[in]   counter     Number of elements inside container.
 * @param[in]   acm             Pointer to the parent ACM configuration.
 *
 * @return EXIT_SUCCESS or error code
 */
int new_acm_module(sr_session_ctx_t *session, int index, sr_val_t* container, size_t counter, struct acm_config *acm)
{
    int stream_id = 0;
    int num_streams = 0;
    struct acm_module* module = NULL;
    enum acm_connection_mode con_mode;
    enum acm_linkspeed speed;
    uint32_t sched_cycle_time = 0;
    struct timespec base_time = {0,0};
    int ret;
    int i = 0;
    char* container_name = NULL;
    char stream_key[MAX_STR_LEN] = {0};
    sr_xpath_ctx_t st = {0};

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    /* get parameters from control node of the Yang model */
    if (EXIT_SUCCESS != new_control_node(session, container, counter, &con_mode, &speed, &sched_cycle_time, &base_time)) {
        SRP_LOG_ERRMSG("Grouping Control is missing in datastore.");
    }

    /* create module instance */
    module = acm_create_module(con_mode, speed, (enum acm_module_id) index);
    if (module == NULL) {
        SRP_LOG_ERRMSG("Cannot create module.");
        return -EACMYMODULECREATE;
    }
    ret = acm_add_module(acm, module);
    if (ret != 0) {
        SRP_LOG_ERRMSG("Cannot add module.");
        acm_destroy_module(module);
        return ret;
    }

    /* set schedule parameters for the module */
    ret = acm_set_module_schedule(module, sched_cycle_time, base_time);
    if (ret != 0) {
        SRP_LOG_ERR("Cannot set schedule of module. Error: %d", ret);
        acm_destroy_module(module);
        return ret;
    }

    for (i=0; i<(int)counter; i++) {
        /* find leaf stream-list-length inside bypass1 or bypass2 container */
        if (true == sr_xpath_node_name_eq(container[i].xpath, ACM_STREAM_LIST_LENGTH_STR)) {
            num_streams = container[i].data.uint8_val;
            break;
        }
    }

    for (i=0; i<(int)counter; i++) {
        /* find all stream list entries
         * xpath will be for example /acm:acm/bypass2/stream[stream-key='1'] (list instance) */
        if (NULL != strstr(container[i].xpath, "stream[stream-key")) {
            /* if stream_id is out of range */
            if (stream_id > num_streams-1) {
                SRP_LOG_ERRMSG("Too many streams defined.");
                return -EACMYSTREAMNUMBER;
            }

            /* if stream list entry is insie bypass1 container */
            if (NULL != strstr(container[i].xpath, "bypass1")) {
                container_name = "bypass1";
            } else if (NULL != strstr(container[i].xpath, "bypass2")) {
                container_name = "bypass2";
            }

            /* get stream-key so inside new_stream_node function we can get right entry of list
             * xpath will be for example /acm:acm/bypass2/stream[stream-key='1'] (list instance) */
            if (EXIT_SUCCESS != get_key_value(NULL, container[i].xpath, ACM_STREAM_LIST_STR, ACM_STREAM_KEY_STR, &st, stream_key)) {
                SRP_LOG_ERR("%s: %s (%s)", __func__, ERR_MISSING_ELEMENT_STR, ACM_STREAM_KEY_STR);
                return EXIT_FAILURE;
            }

            ret = new_stream_node(session, module, container_name, stream_key);
            if (EXIT_SUCCESS != ret) {
                return ret;
            }
            stream_id++;
        }
    }

    /* check stream-list-length is equal to stream_id */
    if (stream_id != num_streams) {
        SRP_LOG_ERRMSG("Actual number of streams not matching stream-list-length.");
        return -EACMYSTREAMNUMBER;
    }

    return EXIT_SUCCESS;


}

/**
 * @brief Function to create and apply configuration.
 *
 * If parameter schedule_change is set to true, only schedule of the running configuration will be
 * updated.
 *
 * @param[in]   session         Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in]   node            Current sr_val_t node.
 * @param[in]   schedule_change Indication for schedule-only change in configuration.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int process_acm_config(sr_session_ctx_t *session, sr_val_t* node, bool schedule_change)
{
    struct acm_config* acm = NULL;
    int bypass1_index = 0;
    int bypass2_index = 1;
    sr_val_t* config_id_node = NULL;
    sr_val_t* bypass1 = NULL;
    size_t bypass1_counter = 0;
    sr_val_t* bypass2 = NULL;
    size_t bypass2_counter = 0;
    uint32_t config_id = 0;
    char error_msg[2*MAX_STR_LEN] = {0};
    int ret = 0;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    acm = acm_create();
    if (acm == NULL) {
        snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, 0, ERR_ACM_CONFIGURATION_STR);
        SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
        sr_set_error(session, node->xpath, error_msg);
        return EXIT_FAILURE;
    }

    tt_stream_list_init();

    /* get container bypass1 */
    if (SR_ERR_OK == sr_get_items(session, ACM_BYPASS1_XPATH, 0, 0, &bypass1, &bypass1_counter)) {
        /* This condition is here because sr_get_items will always find container bypass one inside configuration
         * so we need to check if that container is empty. Container will be empty if his counter is 1.
         * If counter is different that 1, that means that container is not empty, there are leafs and list entries
         * inside him and new_acm_module function should be called. */
        if (1 != bypass1_counter) {
            ret = new_acm_module(session, bypass1_index, bypass1, bypass1_counter, acm);
            if (EXIT_SUCCESS != ret) {
                snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, ret, ERR_BYPASS1_OPERATION_FAILED_STR);
                sr_set_error(session, node->xpath, error_msg);
                SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
                tt_stream_list_clear();
                acm_destroy(acm);
                return EXIT_FAILURE;
            }
        }
        sr_free_values(bypass1, bypass1_counter);
    }

    /* get container bypass2 */
    if (SR_ERR_OK == sr_get_items(session, ACM_BYPASS2_XPATH, 0, 0, &bypass2, &bypass2_counter)) {
        if (1 != bypass2_counter) {
            ret = new_acm_module(session, bypass2_index, bypass2, bypass2_counter, acm);
            if (EXIT_SUCCESS != ret) {
                snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, ret, ERR_BYPASS2_OPERATION_FAILED_STR);
                sr_set_error(session, node->xpath, error_msg);
                SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
                tt_stream_list_clear();
                acm_destroy(acm);
                return EXIT_FAILURE;
            }
        }
        sr_free_values(bypass2, bypass2_counter);
    }

    /* get configuration-id leaf */
    if (SR_ERR_OK == sr_get_item(session, ACM_CONFIGURATION_ID_XPATH, 0, &config_id_node)) {
        config_id = config_id_node->data.uint32_val;
        SRP_LOG_DBG("config ID found: %d.", config_id);
    }

    if (schedule_change) {
        SRP_LOG_DBGMSG("Applying ACM schedule.");
        ret = acm_apply_schedule(acm, config_id, config_id);
        if (ret) {
            snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, ret, ERR_APPLICATION_SCHEDULE_FAILED_STR);
            sr_set_error(session, node->xpath, error_msg);
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
            tt_stream_list_clear();
            acm_destroy(acm);
            return EXIT_FAILURE;
        }
        SRP_LOG_DBGMSG("ACM schedule applied.");
    } else {
        SRP_LOG_DBGMSG("Validating ACM configuration");
        ret = acm_validate_config(acm);
        if (ret) {
            snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, ret, ERR_VALIDATION_FAILED_STR);
            sr_set_error(session, node->xpath, error_msg);
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
            tt_stream_list_clear();
            acm_destroy(acm);
            return EXIT_FAILURE;
        }
        SRP_LOG_DBGMSG("ACM configuration is valid.");

        SRP_LOG_DBGMSG("Applying ACM configuration");
        ret = acm_apply_config(acm, config_id);
        if (ret) {
            snprintf(error_msg, 2*MAX_STR_LEN, ERR_MSG_FORMAT_STR, ret, ERR_APPLICATION_CONFIGURATION_FAILED_STR);
            sr_set_error(session, node->xpath, error_msg);
            SRP_LOG_ERR(ERROR_MSG_FUN_AND_MSG, __func__, error_msg);
            tt_stream_list_clear();
            acm_destroy(acm);
            return EXIT_FAILURE;
        }
        SRP_LOG_DBGMSG("ACM configuration applied.");
    }
    acm_destroy(acm);
    tt_stream_list_clear();

    return EXIT_SUCCESS;
}

/**
 * @brief Callback to be called by the event of editing leaf config-change inside container acm inside acm yang module.
 * Subscribe to it by sr_module_change_subscribe call.
 *
 * @param[in] session        Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in] module_name        Name of the module where the change has occurred.
 * @param[in] xpath          XPath used when subscribing, NULL if the whole module was subscribed to.
 * @param[in] event          Type of the notification event that has occurred.
 * @param[in] request_id     Request ID unique for the specific module_name. Connected events for one request (SR_EV_CHANGE and SR_EV_DONE, for example) have the same request ID.
 * @param[in] private_data   Private context opaque to sysrepo, as passed to sr_module_change_subscribe call.
 * @return      Error code (SR_ERR_OK on success).
 */
static int
acm_config_change_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath, sr_event_t event, uint32_t request_id, void *private_data)
{
    (void)event;
    (void)module_name;
    (void)request_id;
    (void)private_data;
    int rc = SR_ERR_OK;
    sr_change_oper_t op = {0};
    sr_change_iter_t* iter = NULL;
    sr_val_t* old_value = NULL;
    sr_val_t* new_value = NULL;
    sr_val_t* node = NULL;

    /* Pointer to an unsigned integer value that returns the thread id of the thread created. */

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if ((0 == plugin_init)) {
        SRP_LOG_DBG(DEBUG_MSG_WITH_TWO_PARAM, DBG_APPLYING_CHANGES_MSG, __func__);
        return SR_ERR_OK;
    }

    rc = sr_get_changes_iter(session, xpath, &iter);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    rc = sr_get_change_next(session, iter, &op, &old_value, &new_value);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    node = (op == SR_OP_DELETED) ? old_value : new_value;

    /* apply configuration only if leaf is created or modified */
    if ((op == SR_OP_MODIFIED) || (op == SR_OP_CREATED)) {
        /* if leaf config-change is changed */
        if (true == sr_xpath_node_name_eq(node->xpath, ACM_CONFIG_CHANGE_STR)) {
            /* If config-change is true */
            if ((true == node->data.bool_val) && (event == SR_EV_CHANGE)) {
                if (EXIT_SUCCESS != process_acm_config(session, node, false)) {
                    return SR_ERR_OPERATION_FAILED;
                }
            }

            if ((event == SR_EV_DONE) && (true == node->data.bool_val)) {
                SRP_LOG_DBG(DEBUG_MSG_WITH_TWO_PARAM, DBG_APPLYING_CHANGES_MSG, __func__);
            }
        }

        /* if leaf schedule-change is changed */
        if (true == sr_xpath_node_name_eq(node->xpath, ACM_SCHEDULE_CHANGE_STR)) {
            /* If schedule-change is true */
            if ((true == node->data.bool_val) && (event == SR_EV_CHANGE)) {
                if (EXIT_SUCCESS != process_acm_config(session, node, true)) {
                    return SR_ERR_OPERATION_FAILED;
                }
            }

            if ((event == SR_EV_DONE) && (true == node->data.bool_val)) {
                SRP_LOG_DBG(DEBUG_MSG_WITH_TWO_PARAM, DBG_APPLYING_CHANGES_MSG, __func__);
            }
        }
        /* SR_OP_DELETED is supported and nothing happens if node is deleted, also if entire configuration is deleted.
         * Look at the old code, where process_acm_config is called only if config-change and schedule-change are "false."
         * They will always have "false" value in datastore, because we always set them back to "false" after edit-config
         * request.*/

#if 0
        if (op == SR_OP_DELETED) {
            SRP_LOG_ERR(ERROR_MSG_FUN_XML_EL_AND_MSG, __func__, sr_xpath_node_name(node->xpath), "Delete operation not supported.");
            sr_set_error(session, node->xpath, "Delete operation not supported.");
            return SR_ERR_OPERATION_FAILED;
        }
#endif
    }

    sr_free_val(old_value);
    sr_free_change_iter(iter);
    node = NULL;

    return rc;
}

/**
 * @brief Callback to be called by the event of editing leafs inside acm yang module.
 * This callback replaces callbacks for every leaf inside acm yang module.
 * Subscribe to it by sr_module_change_subscribe call.
 *
 * @param[in] session        Implicit session (do not stop) with information about the changed data (retrieved by sr_get_changes_iter) the event originator session IDs.
 * @param[in] module_name        Name of the module where the change has occurred.
 * @param[in] xpath          XPath used when subscribing, NULL if the whole module was subscribed to.
 * @param[in] event          Type of the notification event that has occurred.
 * @param[in] request_id     Request ID unique for the specific module_name. Connected events for one request (SR_EV_CHANGE and SR_EV_DONE, for example) have the same request ID.
 * @param[in] private_data   Private context opaque to sysrepo, as passed to sr_module_change_subscribe call.
 * @return      Error code (SR_ERR_OK on success).
 */
int acm_dummy_cb(sr_session_ctx_t *session, const char *module_name, const char *xpath, sr_event_t event, uint32_t request_id, void *private_data)
{
    (void)module_name;
    (void)event;
    (void)request_id;
    (void)private_data;
    (void)xpath;
    int rc;
    sr_change_oper_t op = {0};
    sr_change_iter_t* iter = NULL;
    sr_val_t* old_value = NULL;
    sr_val_t* new_value = NULL;
    sr_val_t* node = NULL;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    if ((SR_EV_DONE == event) || (0 == plugin_init)) {
        SRP_LOG_DBG(DEBUG_MSG_WITH_TWO_PARAM, DBG_APPLYING_CHANGES_MSG, __func__);
        return SR_ERR_OK;
    }

    /* get changes inside entire acm module */
    rc = sr_get_changes_iter(session, "/acm:*//.", &iter);
    if (SR_ERR_OK != rc) {
        return rc;
    }
    /* iterate trough changes inside module */
    while(SR_ERR_OK == sr_get_change_next(session, iter, &op, &old_value, &new_value)) {
        node = (op == SR_OP_DELETED) ? old_value : new_value;

        /* if changed leaf is not config-change and schedule-change leaf */
        /* those two are covered with acm_config_change_cb */
        if ((true != sr_xpath_node_name_eq(node->xpath, ACM_CONFIG_CHANGE_STR)) && (true != sr_xpath_node_name_eq(node->xpath, ACM_SCHEDULE_CHANGE_STR))) {
            SRP_LOG_DBG(DBG_MSG_CALLBACK_CALLED_STR, __func__, sr_xpath_node_name(node->xpath));
        }
    }

    sr_free_val(old_value);
    sr_free_val(new_value);
    sr_free_change_iter(iter);
    node = NULL;

    return SR_ERR_OK;
}

/**
 * @brief Sysrepo plugin initialization callback.
 *
 * @param[in] session       Sysrepo session that can be used for any API calls needed for plugin initialization (mainly for reading of startup configuration and subscribing for notifications).
 * @param[in] private_data  Private context as passed in sr_plugin_init_cb.
 * @return      Error code (SR_ERR_OK on success). If an error is returned, plugin will be considered as uninitialized.
 */
int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_data)
{
    (void)private_data;
    int rc = SR_ERR_OK;

    SRP_LOG_DBG(DBG_MSG_FUN_CALLED_STR, __func__);

    do {
        /* subscribe for /acm module changes */
        rc = sr_module_change_subscribe(session, ACM_MODULE_STR, NULL,
                                        module_change_cb, NULL, 1, SR_SUBSCR_ENABLED | SR_SUBSCR_DONE_ONLY, &subscription);
        if (SR_ERR_OK != rc) break;

        /* subscribe as state data provider for the /acm:acm/acm-state  */
        rc = sr_oper_get_items_subscribe(session, ACM_MODULE_STR, "/acm:acm-state",
                                         acm_state_cb, NULL, SR_SUBSCR_DEFAULT , &subscription);
        if (SR_ERR_OK != rc) break;

        /* subscribe for /acm:acm/acm:config-change changes */
        rc = sr_module_change_subscribe(session, ACM_MODULE_STR, "/acm:acm/acm:config-change",
                                        acm_config_change_cb, NULL, 3, SR_SUBSCR_DEFAULT, &subscription);
        if (SR_ERR_OK != rc) break;

        /* subscribe for /acm:acm/acm:schedule-change changes */
        rc = sr_module_change_subscribe(session, ACM_MODULE_STR, "/acm:acm/acm:schedule-change",
                                        acm_config_change_cb, NULL, 3, SR_SUBSCR_DEFAULT, &subscription);
        if (SR_ERR_OK != rc) break;

        /* subscribe for "/acm:acm/" changes */
        rc = sr_module_change_subscribe(session, ACM_MODULE_STR, "/acm:acm",
                                        acm_dummy_cb, NULL, 2, SR_SUBSCR_DEFAULT, &subscription);
        if (SR_ERR_OK != rc) break;

    } while (0);


    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR(ERR_MODULE_INIT_FAILED_STR, ACM_MODULE_STR, sr_strerror(rc));
        sr_unsubscribe(subscription);
        return rc;
    }

    if (EXIT_FAILURE == acm_fill_startup_datastore(session)) {
        SRP_LOG_ERR(ERROR_MSG_MOD_FUNC_MSG, ACM_MODULE_STR, __func__, ERR_MSG_EMPTY_STARTUP_DS_STR);
        return SR_ERR_OPERATION_FAILED;
    }

    plugin_init = 1;

    SRP_LOG_INF(INF_MODULE_INIT_SUCCESS_STR, ACM_MODULE_STR);
    return rc;
}

/**
 * @brief Sysrepo plugin cleanup callback.
 *
 * @param[in] session        Sysrepo session that can be used for any API calls needed for plugin cleanup (mainly for unsubscribing of subscriptions initialized in sr_plugin_init_cb).
 * @param[in] private_data   Private context as passed in sr_plugin_init_cb.
 */
void
sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_data)
{
    (void)private_data;
    (void)session;
    /* nothing to cleanup except freeing the subscriptions */
    sr_unsubscribe(subscription);
    SRP_LOG_INF(INF_MODULE_CLEANUP_STR, ACM_MODULE_STR);
}
