/*
 * TTTech acm-yang
 * Copyright(c) 2019 TTTech Industrial Automation AG.
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
 * Contact: https://tttech.com
 * support@tttech.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */

#ifndef ACM_DEFINES_H_
#define ACM_DEFINES_H_

#define ACM_MODULE_STR                       "acm"
#define ACM_MODULE_NAMESPACE_STR             "urn:tttech-industrial:yang:acm"

/* containers, lists, leafs names */
#define ACM_CONFIG_VERSION_STR               "configuration-id"
#define ACM_BYPASS_1_STR                     "bypass1"
#define ACM_BYPASS_2_STR                     "bypass2"
#define ACM_CONFIG_CHANGE_STR                "config-change"
#define ACM_SCHEDULE_CHANGE_STR              "schedule-change"

#define ACM_STREAM_LIST_LENGTH_STR           "stream-list-length"
#define ACM_STREAM_LIST_STR                  "stream"
#define ACM_STREAM_KEY_STR                   "stream-key"

#define ACM_INGRESS_STREAM_STR               "ingress-stream"
#define ACM_EGRESS_STREAM_STR                "time-triggered-stream"
#define ACM_EVENT_STREAM_STR                 "event-stream"
#define ACM_EGRESS_STREAM_RECOVERY_STR       "egress-stream-recovery"
#define ACM_HEADER_STR                       "header"
#define ACM_HEADER_MASK_STR                  "header-mask"
#define ACM_DEST_ADDRESS_STR                 "destination-address"
#define ACM_SOURCE_ADDRESS_STR               "source-address"
#define ACM_VID_STR                          "vid"
#define ACM_PRIORITY_STR                     "priority"
#define ACM_ADD_FILTER_SIZE_STR              "additional-filter-size"
#define ACM_ADD_FILTER_MASK_STR              "additional-filter-mask"
#define ACM_ADD_FILTER_PATTERN_STR           "additional-filter-pattern"
#define ACM_HAS_REDUNDANCY_TAG_STR           "has-redundancy-tag"
#define ACM_SEQ_RECOVERY_RESET_MS_STR        "seq-recovery-reset-ms"
#define ACM_ADD_CONST_DATA_SIZE_STR          "additional-constant-data-size"
#define ACM_ADD_CONST_DATA_STR               "additional-constant-data"

#define ACM_OPER_LIST_LENGTH_STR             "operation-list-length"
#define ACM_STREAM_OPERATIONS_STR            "stream-operations"
#define ACM_OPER_KEY_STR                     "operation-key"
#define ACM_OPER_NAME_STR                    "operation-name"
#define ACM_OFFSET_STR                       "offset"
#define ACM_LENGTH_STR                       "length"
#define ACM_BUFFER_NAME_STR                  "buffer-name"

#define ACM_STREAM_SCHEDULE_STR              "stream-schedule"
#define ACM_PERIOD_STR                       "period"
#define ACM_SCHEDULE_EVENTS_LENGTH_STR       "schedule-events-length"
#define ACM_SCHEDULE_ENENTS_STR              "schedule-events"
#define ACM_SCHEDULE_KEY_STR                 "schedule-key"
#define ACM_TIME_START_STR                   "time-start"
#define ACM_TIME_END_STR                     "time-end"

#define ACM_CONNECT_MODE_STR                 "connection-mode"
#define ACM_LINK_SPEED_STR                   "link-speed"
#define ACM_CYCLE_TIME_STR                   "cycle-time"
#define ACM_BASE_TIME_STR                    "base-time"
#define ACM_SECONDS_STR                      "seconds"
#define ACM_FRACTIONAL_SECONDS_STR           "fractional-seconds"

#define ACM_STATE_STR                        "acm-state"
#define ACM_LIB_VERSION_STR                  "acm-lib-version"
#define ACM_IP_VERSION_STR                   "acm-ip-version"
#define ACM_MIN_SCHED_TICK_STR               "minimal-schedule-tick"
#define ACM_MAX_MSG_BUF_SIZE_STR             "maximal-message-buffer-size"
#define ACM_HALT_ERR_OCCURED_STR             "HaltOnErrorOccured"
#define ACM_IP_ERROR_FLAG_STR                "IPErrorFlags"
#define ACM_POLICING_ERROR_FLAGS_STR         "PolicingErrorFlags"
#define ACM_RUNT_FRAMES_STR                  "RuntFrames"
#define ACM_MII_ERRORS_STR                   "MIIErrors"
#define ACM_LAYER_MISMATCH_CNT_STR           "Layer7MismatchCnt"
#define ACM_DROPED_FRAMES_CNT_STR            "DroppedFramesCounterPrevCycle"
#define ACM_SCATTER_DMAF_STR                 "FramesScatteredPrevCycle"
#define ACM_TX_FRAMES_PREV_CYCLE_STR         "TxFramesPrevCycle"
#define ACM_GMII_ERROR_PREV_CYCLE_STR        "GMIIErrorPrevCycle"
#define ACM_DISABLE_OVERRRUN_STR             "AcmDisableOverrunPrevCycle"
#define ACM_TX_FRAMES_CYCLE_CHANGE_STR       "TxFramesCycleChange"
#define ACM_RX_FRAMES_PREV_CYCLE_STR         "RxFramesPrevCycle"
#define ACM_RX_FRAMES_CYCLE_CHANGE_STR       "RxFramesCycleChange"
#define ACM_SOF_ERRORS_STR                   "SofErrors"

/* container acm */
#define ACM_SCHEDULE_CHANGE_XPATH					"/acm:acm/schedule-change"
#define ACM_CONFIG_CHANGE_XPATH						"/acm:acm/config-change"
#define ACM_CONFIGURATION_ID_XPATH 					"/acm:acm/configuration-id"
#define ACM_BASE_TIME_XPATH 						"/acm:acm/%s/base-time/*"
#define ACM_BYPASS1_XPATH 							"/acm:acm/bypass1/*"
#define ACM_BYPASS2_XPATH 							"/acm:acm/bypass2/*"
#define ACM_LIST_STREAM_XPATH 					    "/acm:acm/%s/stream[stream-key='%s']/*"
#define ACM_LIST_STREAM_INGRESS_STREAM_CON_XPATH	"/acm:acm/%s/stream[stream-key='%s']/ingress-stream/*"
#define ACM_LIST_STREAM_TIME_TRIGGERED_ST_XPATH		"/acm:acm/%s/stream[stream-key='%s']/time-triggered-stream/*"
#define ACM_LIST_STREAM_EVENT_STREAM_XPATH			"/acm:acm/%s/stream[stream-key='%s']/event-stream/*"
#define ACM_LIST_STREAM_EGGRESS_STREAM_REC_XPATH	"/acm:acm/%s/stream[stream-key='%s']/egress-stream-recovery/*"
#define ACM_LIST_STREAM_STREAM_SCHEDULE_XPATH		"/acm:acm/%s/stream[stream-key='%s']/stream-schedule/*"
#define ACM_LIST_STREAM_OPERATION_XPATH 		    "/acm:acm/%s/stream[stream-key='%s']/%s/stream-operations[operation-key='%s']/*"
#define ACM_LIST_SCHEDULE_EVENTS_XPATH 		    	"/acm:acm/%s/stream[stream-key='%s']/stream-schedule/schedule-events[schedule-key='%s']/*"
#define ACM_TIME_TRIGGERED_STREAM_XPATH 		    "/acm:acm/%s/stream[stream-key='%s']/*"

/* container acm-state XPATH */
#define ACM_LIB_VERSION_XPATH 						"/acm:acm-state/acm-lib-version"
#define ACM_IP_VERSION_XPATH 						"/acm:acm-state/acm-ip-version"

/* container acm-state, containers bypass1 and bypass2 xpath */
#define ACM_HALT_ON_ERROR_OCCURED_XPATH 			"/acm:acm-state/%s/HaltOnErrorOccured"
#define ACM_IP_ERROR_FLAGS_XPATH 					"/acm:acm-state/%s/IPErrorFlags"
#define ACM_POLICING_ERROR_FLAGS_XPATH 				"/acm:acm-state/%s/PolicingErrorFlags"
#define ACM_RUNT_FRAMES_XPATH 						"/acm:acm-state/%s/RuntFrames"
#define ACM_MII_ERRORS_XPATH 						"/acm:acm-state/%s/MIIErrors"
#define ACM_LAYER7_MISMATCH_CNT_XPATH 				"/acm:acm-state/%s/Layer7MismatchCnt"
#define ACM_DROPPED_FRAMES_CNT_PREV_CYCLE_XPATH 	"/acm:acm-state/%s/DroppedFramesCounterPrevCycle"
#define ACM_FRAMES_SCATTERED_PREV_CYCLE_XPATH 		"/acm:acm-state/%s/FramesScatteredPrevCycle"
#define ACM_TX_FRAMES_PREV_CYCLE_XPATH 				"/acm:acm-state/%s/TxFramesPrevCycle"
#define ACM_GMII_ERROR_PREV_CYCLE_XPATH 			"/acm:acm-state/%s/GMIIErrorPrevCycle"
#define ACM_DISABLE_OVERRUN_PREV_CYCLE_XPATH 		"/acm:acm-state/%s/AcmDisableOverrunPrevCycle"
#define ACM_TX_FRAMES_CYCLE_CHANGE_XPATH 			"/acm:acm-state/%s/TxFramesCycleChange"
#define ACM_RX_FRAMES_PREV_CYCLE_XPATH 				"/acm:acm-state/%s/RxFramesPrevCycle"
#define ACM_RX_FRAMES_CYCLE_CHANGE_XPATH 			"/acm:acm-state/%s/RxFramesCycleChange"
#define ACM_SOF_ERRORS_XPATH			 			"/acm:acm-state/%s/SofErrors"


#define ERR_ACM_STATUS_MODULE_STR	                "Cannot get ACM status module %d, status %d. Error: %d"
#define ERR_ACM_CONFIGURATION_STR					"ACM configuration cannot be created."
#define ERR_APPLICATION_SCHEDULE_FAILED_STR			"Application of schedule failed."
#define ERR_VALIDATION_FAILED_STR					"Validation of configuration failed."
#define ERR_APPLICATION_CONFIGURATION_FAILED_STR	"Validation of configuration failed."
#define ERR_BYPASS1_OPERATION_FAILED_STR			"Operation failed for module 'bypass1'."
#define ERR_BYPASS2_OPERATION_FAILED_STR			"Operation failed for module 'bypass2'."
#define ERR_CONFIG_ENABLE_STR						"Config-enable is false or it does not exist."

#define DBG_MSG_CALLBACK_CALLED_STR					"%s(): Callback called for node %s."
#define ERR_MSG_FORMAT_STR 							"[return: %d] %s"

/**
 * @ingroup acminternal
 * @brief stream types
 */
typedef enum {
    STREAM_TIME_TRIGGERED = 0,  /**< time-triggered stream */
    STREAM_INGRESS_TRIGGERED,   /**< ingress-triggered stream */
    STREAM_EVENT,               /**< event stream */
    STREAM_RECOVERY,            /**< recovery stream */
} STREAM_TYPE;

/**
 * @ingroup acminternal
 * @brief operation types
 */
typedef enum {
    OPC_READ = 0,       /**< read operation */
    OPC_INSERT,         /**< insert operation */
    OPC_CONST_INSERT,   /**< insert constant operation */
    OPC_FORWARD,        /**< forward operation */
    OPC_PADDING,        /**< padding operation */
} STREAM_OPCODE;
#endif
