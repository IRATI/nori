/* Wrap around IRATI C++ system libraries.
 *
 * Copyright (c) 2016 Kewin Rausch <kewin.rausch@create-net.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors and changes:
 */

#ifndef __NORI_RINAW_H
#define __NORI_RINAW_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Unreliable default QoS cube. */
#define qos_unreliable_default(x) 		\
	x.averageBandwidth = 0;			\
	x.averageSDUBandwidth = 0;		\
	x.delay = 0;				\
	x.jitter = 0;				\
	x.maxAllowableGap = -1;			\
	x.maxSDUsize = 0;			\
	x.orderedDelivery = 0;			\
	x.partialDelivery = 0;			\
	x.peakBandwidthDuration = 0;		\
	x.peakSDUBandwidthDuration = 0;		\
	x.undetectedBitErrorRate = 0

/* Reliable default QoS cube */
#define qos_reliable_default(x)	 		\
	x.averageBandwidth = 0;			\
	x.averageSDUBandwidth = 0;		\
	x.delay = 0;				\
	x.jitter = 0;				\
	x.maxAllowableGap = 0;			\
	x.maxSDUsize = 0;			\
	x.orderedDelivery = 1;			\
	x.partialDelivery = 0;			\
	x.peakBandwidthDuration = 0;		\
	x.peakSDUBandwidthDuration = 0;		\
	x.undetectedBitErrorRate = 0

/* What we consider a flow in RINA; basically a handle. */
typedef int rina_flow;

/* QoS fields. */
struct rina_qos {
	unsigned int averageBandwidth;
	unsigned int averageSDUBandwidth;
	unsigned int peakBandwidthDuration;
	unsigned int peakSDUBandwidthDuration;
	double undetectedBitErrorRate;
	int partialDelivery;
	int orderedDelivery;
	int maxAllowableGap;
	unsigned int delay;
	unsigned int jitter;
	unsigned int maxSDUsize;
};

/* Information about a flow. */
struct rina_AP_info {
	char name[256];
	int port;
};

/*
 * Rina subsystem operations:
 */

int rina_init(void);

/* Stop any operation and prepare to close. */
int rina_stop(void);

/*
 * I/O operations:
 */

/* Read a SDU... */
int rina_read_sdu(rina_flow port, char * buffer, unsigned int size);

/* Send a SDU somewhere... */
int rina_write_sdu(rina_flow port, char * buffer, unsigned int size);

/*
 * Operations on the single flow:
 */

/* Swap the flow behavior to async. */
int rina_async_flow(rina_flow port);

/* Request a flow to a certain AE within a DIF. */
rina_flow rina_request_flow(
	const char * srcn, /* Source info */
	const char * srci,
	const char * dstn, /* Destination info */
	const char * dsti,
	struct rina_qos * qos); /* Qos to use. */

/* Release a previously registered flow. */
int rina_release_flow(rina_flow port);

/* Swap the flow behavior to sync. */
int rina_sync_flow(rina_flow port);

/*
 * Operations at AE level:
 */

/* Creates an AE into RINA subsystems. */
int rina_create_AE(
	const char * name, const char * instance, const char * difn);

/* Releases an AE from the RINA subsystems. */
int rina_release_AE(
	const char * name, const char * instance, const char * difn);

/*
 * Main procedure which reacts to RINA events.
 */

/* Listen for important events to occurr within the RINA subsystem. */
int rina_listen_for_events(
	/* Serve new flow for this AE. */
	void * (* flow_serve)(void * args),
	/* React to a flow deallocation. */
	void (* flow_release)(int port),
	/* You want async operation? */
	int async);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __NORI_RINAW_H */
