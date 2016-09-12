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

#include <stdlib.h>
#include <string>

#define RINA_PREFIX "nori"
#include <librina/logs.h>
#include <librina/librina.h>

//#include <asm/unistd_32.h>
#include <asm/unistd_64.h>

#include <pthread.h>

using namespace std;
using namespace rina;

extern "C"
{

/* Interrupt the listening loop when waiting for RINA subsystem events. */
static int rina_list_stop = 0;

/*
 * Rina subsystem operations:
 */

int rina_init(void) {
	/* Raise the log level to hide everything... */
	setLogLevel("ERR");

	return 0;
}

int rina_stop(void) {
	/* This cause any running loop to be stopped. */
	rina_list_stop = 1;

	return 0;
}

/*
 * I/O operations:
 */

#include "rinaw.h"

/* Read a SDU... */
int rina_read_sdu(rina_flow port, char * buffer, unsigned int size) {
	try {
		return ipcManager->readSDU(port, buffer, size);
	} catch (FlowNotAllocatedException fnae) {
		return -2;
	} catch (Exception e) {
		return -1;
	}
}

/* Send a SDU somewhere... */
int rina_write_sdu(rina_flow port, char * buffer, unsigned int size) {
	try {
		return ipcManager->writeSDU(port, buffer, size);
	} catch (FlowNotAllocatedException fnae) {
		return -2;
	} catch (Exception e) {
		return -1;
	}
}

/*
 * Operations on the single flow:
 */

/* Swap the flow behavior to async. */
int rina_async_flow(rina_flow port) {
	long int result = -1;

	result = syscall(__NR_flow_io_ctl, port, F_GETFL);
	result = syscall(__NR_flow_io_ctl, port, F_SETFL, result | O_NONBLOCK);

	if(result < 0) {
		printf("Error wile controlling flow on %d\n", port);
		return result;
	}

	return 0;
}

/* Request a flow to a certain AE within a DIF. */
rina_flow rina_request_flow(
	const char * srcn, /* Source info */
	const char * srci,
	const char * dstn, /* Destination info */
	const char * dsti,
	struct rina_qos * qos) { /* Qos to use. */

	AllocateFlowRequestResultEvent * afrrevent;
	FlowSpecification qos_spec;
	IPCEvent * event;

	string sn(srcn);
	string si(srci);
	string dn(dstn);
	string di(dsti);

	unsigned int seqnum;

	/* Setup the qos to respect. */
	qos_spec.averageBandwidth = qos->averageBandwidth;
	qos_spec.averageSDUBandwidth = qos->averageSDUBandwidth;
	qos_spec.delay = qos->delay;
	qos_spec.jitter = qos->jitter;
	qos_spec.maxAllowableGap = qos->maxAllowableGap;
	qos_spec.maxSDUsize = qos->maxSDUsize;
	qos_spec.orderedDelivery = qos->orderedDelivery;
	qos_spec.partialDelivery = qos->partialDelivery;
	qos_spec.peakBandwidthDuration = qos->peakBandwidthDuration;
	qos_spec.peakSDUBandwidthDuration = qos->peakSDUBandwidthDuration;
	qos_spec.undetectedBitErrorRate = qos->undetectedBitErrorRate;

	seqnum = ipcManager->requestFlowAllocation(
		ApplicationProcessNamingInformation(sn, si),
		ApplicationProcessNamingInformation(dn, di),
		qos_spec);

	/* Again, wait for an event or user break. */
	while(!rina_list_stop) {
		event = ipcEventProducer->eventWait();

		if (event && event->eventType ==
				ALLOCATE_FLOW_REQUEST_RESULT_EVENT &&
			event->sequenceNumber == seqnum) {

			break;
		}
	}

	afrrevent = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

	rina::FlowInformation flow =
		ipcManager->commitPendingFlow(
			afrrevent->sequenceNumber,
			afrrevent->portId,
			afrrevent->difName);

	if (flow.portId < 0) {
		return -1;
	}

	return flow.portId;
}

/* Release a previously registered flow. */
int rina_release_flow(rina_flow port) {
	DeallocateFlowResponseEvent * resp = 0;
	unsigned int seqNum;
	IPCEvent * event;

	seqNum = ipcManager->requestFlowDeallocation(port);

	/* Again, wait for an event or user break. */
	while(!rina_list_stop) {
		event = ipcEventProducer->eventWait();

		if (event && event->eventType ==
				DEALLOCATE_FLOW_RESPONSE_EVENT &&
			event->sequenceNumber == seqNum) {

			break;
		}
	}

	resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
	ipcManager->flowDeallocationResult(port, resp->result == 0);

	return 0;
}

/* Swap the flow behavior to sync. */
int rina_sync_flow(rina_flow port) {
	long int result = -1;

	result = syscall(__NR_flow_io_ctl, port, F_GETFL);
	result = syscall(__NR_flow_io_ctl, port, F_SETFL, result & ~O_NONBLOCK);

	if(result < 0) {
		printf("Error wile controlling flow on %d\n", port);
		return result;
	}

	return 0;
}

/*
 * Operations at AE level:
 */

/* Creates an AE into RINA subsystems. */
int rina_create_AE(
	const char * name, const char * instance, const char * difn) {

	unsigned int seqnum = 0;

	ApplicationRegistrationInformation ari;
	RegisterApplicationResponseEvent * resp = 0;
	IPCEvent * event = 0;

	string ns(name);
	string is(instance);
	string dn(difn);

	/* This is an AP, not an IPC process */
	ari.ipcProcessId = 0;
	ari.appName = ApplicationProcessNamingInformation(ns, is);

	ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
	ari.difName = ApplicationProcessNamingInformation(dn, string());

	seqnum = ipcManager->requestApplicationRegistration(ari);

	/* Again, wait for an event or user break. */
	while(!rina_list_stop) {
		event = ipcEventProducer->eventWait();

		/* Event! */
		if (event && event->eventType ==
				REGISTER_APPLICATION_RESPONSE_EVENT &&
			event->sequenceNumber == seqnum) {

			break;
		}
	}

	resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

	/* Maintain aligned the ipcm. */
	if (resp->result == 0) {
		ipcManager->commitPendingRegistration(seqnum, resp->DIFName);
	} else {
		ipcManager->withdrawPendingRegistration(seqnum);
		return -1;
	}

	return 0;
}

/* Releases an AE from the RINA subsystems. */
int rina_release_AE(
	const char * name, const char * instance, const char * difn) {

	unsigned int seqnum = 0;

	UnregisterApplicationResponseEvent * resp = 0;
	IPCEvent * event = 0;

	string ns(name);
	string is(instance);
	string dn(difn);

	seqnum = ipcManager->requestApplicationUnregistration(
		ApplicationProcessNamingInformation(ns, is),
		ApplicationProcessNamingInformation(dn, string()));

	/* Again, wait for an event or user break. */
	while(!rina_list_stop) {
		event = ipcEventProducer->eventWait();
		if (event &&
			event->eventType ==
				UNREGISTER_APPLICATION_RESPONSE_EVENT &&
			event->sequenceNumber == seqnum) {

			break;
		}
	}

	resp = dynamic_cast<UnregisterApplicationResponseEvent*>(event);

	if (resp->result != 0) {
		ipcManager->appUnregistrationResult(seqnum, true);
	} else {
		ipcManager->appUnregistrationResult(seqnum, false);
		return -1;
	}

	return 0;
}

/*
 * Main procedure which reacts to RINA events.
 */

int rina_listen_for_events(
	void * (* flow_serve)(void * args),
	void (* flow_release)(int port),
	int async) {

	pthread_t t = 0;
	struct rina_AP_info * ai;

	do {
		/* Wait no more than half microsecond...*/
		IPCEvent * event = ipcEventProducer->eventTimedWait(0, 500000);
		int port_id = 0;

		if (!event) {
			continue;
		}

		switch (event->eventType) {

		/* AE creation event. */
		case REGISTER_APPLICATION_RESPONSE_EVENT:
			ipcManager->commitPendingRegistration(
				event->sequenceNumber,
				dynamic_cast<RegisterApplicationResponseEvent*>(
					event)->DIFName);
			break;

		/* AE release event. */
		case UNREGISTER_APPLICATION_RESPONSE_EVENT:
			ipcManager->appUnregistrationResult(
				event->sequenceNumber,
				dynamic_cast<UnregisterApplicationResponseEvent*>(
					event)->result == 0);
			break;

		/* A signal inform that a flow creation event occurs. */
		case FLOW_ALLOCATION_REQUESTED_EVENT: {
			rina::FlowInformation flow =
				ipcManager->allocateFlowResponse(
				*dynamic_cast<FlowRequestEvent*>(
					event), 0, true);

			ai = (struct rina_AP_info *)malloc(
				sizeof(struct rina_AP_info));

			if(!ai) {
				printf("No more memory!");
				flow_release(flow.portId);
				break;
			}

			/* Populate information about this flow. */
			memcpy(ai->name,
				flow.remoteAppName.toString().c_str(),
				strlen(flow.remoteAppName.toString().c_str()));

			ai->port = flow.portId;

			/* Create the thread which will take care of it. */
			if(pthread_create(&t, NULL, flow_serve, ai)) {
				printf("Pthread failure");
				free(ai);
				break;
			}

			break;
		}

		/* A signal inform that a flow has been de-allocated. */
		case FLOW_DEALLOCATED_EVENT:
			port_id = dynamic_cast<FlowDeallocatedEvent*>(
				event)->portId;

			flow_release(port_id);

			ipcManager->flowDeallocated(port_id);
			break;

		default:
			break;
		}

	/* Loop only if you have not to stop or to wait. */
	} while(!rina_list_stop && !async);

	return 0;
}

} /* extern "C" */
