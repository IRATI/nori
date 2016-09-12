/* Nfv Over RIna entry point.
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
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <pthread.h>

#include "dict.h"
#include "list.h"
#include "proto.h"
#include "rinaw.h"
#include "tunw.h"

/*
 * Misc.
 */

#define ts_diff_to_s(a, b)				\
	(((b.tv_sec - a.tv_sec) * 1) + ((b.tv_nsec - a.tv_nsec) / 1000000000))

/* Information about a known flow. */
struct known_flow {
	/* This is member of a list. */
	struct list_head listh;

	/* RINA port to use. */
	rina_flow id;

	/* Name. */
	char name[NAME_MAX];
	/* Instance. */
	char instance[NAME_MAX];
};

/*
 * Early exit of application.
 */

/* How many ctrl-c before forced exit? */
#define CTRLC_TRIPLE_FAULT	3

/* Ctrl-c handler. */
volatile sig_atomic_t nori_ctrlc = 0;
/* Counts how many 'ctrl-c' have been invoked on this program. */
unsigned int nori_ctrl_i = 0;

/*
 * Multi-thread alignment.
 */

/* Lock to align multi-threaded operations. */
static pthread_spinlock_t nori_mt_lock;

/*
 * Default values for tun/tap creation.
 */

/* Tun/tap fd to use for operations. */
int nori_dev_fd = 0;
/* Let the kernel choose the name of the device. */
static char nori_dev_name[16] = {0};
/* Type of tun/tap device to create. */
static int nori_dev_type = TUNW_MODE_TUN;
/* Persistent or not? */
static int nori_dev_pers = 0;

/*
 * Known end-points and RINA stuff.
 */

/* List of know AE which connected with this application. */
static LIST_HEAD(nori_known_ae);

/* IRATI instance to use. */
static char * nori_instance = 0;
/* IRATI name to use. */
static char * nori_name = 0;
/* IRATI DIF name to use. */
static char * nori_dif = 0;

/******************************************************************************
 * Early fail.                                                                *
 ******************************************************************************/

/* Handle a break-execution signal from the user. */
void handle_ctrlc(int signal) {
	printf("! CTRL-C detected; breaking the execution !\n");

	nori_ctrlc = 1;
	nori_ctrl_i++;

	/* Force stop on triple ctrl-c. */
	if(nori_ctrl_i == CTRLC_TRIPLE_FAULT) {
		printf("! Application FAULT... terminating !\n");

		/* Stop any active loop at rina side. */
		rina_stop();

		/*
		 * Do whatever necessary to dispose resources, but take into
		 * account that using rina_* operation can fail (loops now) are
		 * immediately stopped.
		 */

		exit(-1);
	}
}

/******************************************************************************
 * Handle new flow allocation/deallocation.                                   *
 ******************************************************************************/

/* This happens in a multi-threaded context! */
void * flow_allocated(void * args) {
	char * name = 0;
	char * inst = 0;

	struct known_flow * kf = 0;
	struct rina_AP_info * ap = (struct rina_AP_info *)args;

	kf = malloc(sizeof(struct known_flow));

	if(!kf) {
		printf("No more memory while serving a flow.");
		goto out;
	}

	memset(kf, 0, sizeof(struct known_flow));
	INIT_LIST_HEAD(&kf->listh);

	name = strtok(ap->name, ":");
	inst = strtok(0, ":");

	kf->id = ap->port;
	strncpy(kf->name, name, NAME_MAX);
	strncpy(kf->instance, inst, NAME_MAX);

	/* Use the list in an atomic context. */
	pthread_spin_lock(&nori_mt_lock);
	list_add(&kf->listh, &nori_known_ae);
	pthread_spin_unlock(&nori_mt_lock);

	printf("%s-%s connected...\n", kf->name, kf->instance);

out:
	/* Free the given ap info. */
	free(ap);
}

/* Remove this from the known flows. */
void flow_deallocated(rina_flow port) {
	int found = 0;
	struct known_flow * kf = 0;

	/* Use the list in an atomic context. */
	pthread_spin_lock(&nori_mt_lock);
	list_for_each_entry(kf, &nori_known_ae, listh) {
		if(kf->id == port) {
			found = 1;
			break;
		}
	}

	if(found) {
		list_del(&kf->listh);
	}
	pthread_spin_unlock(&nori_mt_lock);

	if(found) {
		printf("%s-%s disconnected...\n", kf->name, kf->instance);
		free(kf);
	}
}

/******************************************************************************
 * Core routines of NORI.                                                     *
 ******************************************************************************/

int nori_send_to(char * name, char * instance, char * buf, int size) {
	rina_flow id = -1;
	struct rina_qos q;
	struct known_flow * kf  = 0;

	/* NOTE:
	 * This can be slow, so maybe is better to pre-allocate the flow when
	 * the rule is added to the dictionary?
	 */
	pthread_spin_lock(&nori_mt_lock);
	list_for_each_entry(kf, &nori_known_ae, listh) {
		if(strcmp(name, kf->name) == 0 &&
			strcmp(instance, kf->instance) == 0) {

			id = kf->id;
			break;
		}
	}
	pthread_spin_unlock(&nori_mt_lock);

	/* Not existing, so create it anew. */
	if(id < 0) {
		kf = malloc(sizeof(struct known_flow));

		if(!kf) {
			return -1;
		}

		/* NOTE: Use unreliable by default. User should choose this. */
		qos_unreliable_default(q);

		kf->id = rina_request_flow(
			nori_name, nori_instance, name, instance, &q);

		if(kf->id < 0) {
			/*
			printf("Failed to allocate the flow to %s-%s...\n",
				name, instance);
			 */
			free(kf);
			return -1;
		}

		INIT_LIST_HEAD(&kf->listh);
		strcpy(kf->name, name);
		strcpy(kf->instance, instance);

		/* Add to the list, so can be reused. */
		pthread_spin_lock(&nori_mt_lock);
		list_add(&kf->listh, &nori_known_ae);
		pthread_spin_unlock(&nori_mt_lock);

		id = kf->id;

		printf("New flow allocated to %s-%s, id %d\n",
			name, instance, id);
	}

	/*printf("Sending to %d\n", id);*/

	return rina_write_sdu(id, buf, size);
}

/* Returns 1 if action has not been taken, 0 on success, -1 on failure. */
int nori_take_ip_action(struct dict_rule * rule, char * buf, int size) {
	struct rule_ip * r = (struct rule_ip *)rule->data;
	int off = TUN_INITIAL_OFFSET;

	if(r->direction == RULE_DIR_SRC) {
		off += IPV4_SOURCE_OFFSET;
	} else {
		off += IPV4_DEST_OFFSET;
	}

	/* Are the addresses equals? */
	if(memcmp(buf + off, r->address, 4) == 0) {
		/*
		printf("Taking IP action, src=%d.%d.%d.%d\n",
			(unsigned char)buf[off],
			(unsigned char)buf[off + 1],
			(unsigned char)buf[off + 2],
			(unsigned char)buf[off + 3]);
		 */
		if(nori_send_to(r->dest.ae, r->dest.ai, buf, size) < 0) {
			return -1;
		}

		return 0;
	}

	return 1;
}

/* Returns 0 on success, -1 on failure. */
int nori_take_default_action(struct dict_rule * rule, char * buf, int size) {
	struct rule_default * rd = (struct rule_default *)rule->data;
	struct rule_dest * de = 0;

	struct timespec now;

	/* Go though the first destination only! */
	if(rd->strategy == RULE_STR_SI) {
		de = list_first_entry_or_null(
			&rd->dests, struct rule_dest, listh);

		if(!de) {
			printf("No destination for default rule!\n");
			return -1;
		}

		/*printf("Taking default SI action...\n");*/

		return nori_send_to(de->ae, de->ai, buf, size);
	} else if (rd->strategy == RULE_STR_RR) {
/* Repeat the selection getting the next possible destination. */
repeat:
		de = rd->next;

		/*
		printf("Taking default RR action fro %s-%s...\n",
			de->ae, de->ai);
		 */

		/* Next is end of the list? */
		if(de->listh.next == &rd->dests) {
			/* Take again the first one. */
			rd->next = list_first_entry(
				&rd->dests, struct rule_dest, listh);
		}
		/* There's someone after us? */
		else {
			rd->next = list_next_entry(de, listh);
		}

		/*
		 * Ok, 'de' has been selected and next set properly.
		 */

		if(!de->open) {
			clock_gettime(CLOCK_REALTIME, &now);

			/* More than 1 seconds elapsed from the last time we
			 * tried to use this.
			 */
			if(ts_diff_to_s(de->lrt, now) > 1) {
				de->open = 1;
			} else {
				goto repeat;
			}
		}

		if(nori_send_to(de->ae, de->ai, buf, size) > 0) {
			de->open = 1;
		}
		/* Error while sending... not possible to send this to him. */
		else {
			clock_gettime(CLOCK_REALTIME, &de->lrt);
			de->open = 0;

			goto repeat;
		}
	}

	return 0;
}

/* Analyze the data and take action depending on the rules. */
int nori_take_action(char * buf, int size) {
	struct dict_rule * r = 0;

	list_for_each_entry(r, &dict_rules, listh) {
		switch(r->type) {
		case RULE_IP:
			/* If the action has not been taken, list for all. */
			if(nori_take_ip_action(r, buf, size) > 0) {
				continue;
			}
			return 0; /* Complete stop. */
		case RULE_DEF:
			nori_take_default_action(r, buf, size);
			return 0; /* Complete stop. */
		default:
			printf("Unknown action %d!\n", r->type);
			break;
		}
	}

	return 0;
}

/* Main loop for NORI. */
int nori_loop(void) {
	struct known_flow * kf  = 0;
	struct known_flow * tmp = 0;

	char buf[4096] = {0};
	int bytes = 0;

	/* While TRUE! */
	while(!nori_ctrlc) {
		/* Save it against removal/insertion... */
		pthread_spin_lock(&nori_mt_lock);
		list_for_each_entry_safe(kf, tmp, &nori_known_ae, listh) {
			if(kf->id > 0) {
				/* Read without waiting! */
				rina_async_flow(kf->id);
				bytes = rina_read_sdu(kf->id, buf, 4096);
				rina_sync_flow(kf->id);

				/* Move it on the interface. */
				if(bytes > 0) {
					tun_write(nori_dev_fd, buf, bytes);
				}
			}
		}
		pthread_spin_unlock(&nori_mt_lock);

		/* Now read for traffic on the interface. */
		bytes = tun_read(nori_dev_fd, buf, 4096);

		/* Everything which comes from the interface will be dumped to
		 * the flow, if it already exists.
		 */
		if(bytes > 0) {
			nori_take_action(buf, bytes);
		}

		/* Listen for events, but do not wait them! */
		rina_listen_for_events(flow_allocated, flow_deallocated, 1);
	}
}

/******************************************************************************
 * Misc. procedures.                                                          *
 ******************************************************************************/

/* Show the help text. */
void help(void) {
	printf(
"Nfv Over RIna - NORI\n"
"Copyright (c) 2016 Kewin Rausch <kewin.rausch@create-net.org>\n"
"\n"
"This software allows to move IP traffic from legacy interfaces over recursive "
"internetworking architectures.\n"
"Usage: nori <ap_name> <ap_inst> <dif_name> [options] <rules dictionary>\n"
"\n"
"Options:\n"
"    --help, Show this text.\n"
"\n");
}

/* Parsing the arguments feed the states of NORI changing its default values. */
int parse_args(int argc, char ** argv) {
	int i = 0;
	char * current = 0;
	char * option  = 0;

	if(strcmp("--help", argv[1]) == 0) {
		help();
		return 1;
	}

	/* 4 is the minimal amount of necessary options. */
	if(argc < 5) {
		printf("Not enough arguments!\n");
		return 1;
	}

	nori_name = argv[1];
	nori_instance = argv[2];
	nori_dif = argv[3];

	for(i = 4; i < argc; i++) {
		current = argv[i];

		/* Not an --<something> found? */
		if(current[0] != '-' || current[1] != '-') {
			continue;
		}

		option = current + 2;

		if(strcmp(option, "devname") == 0) {
			if(argc + 1 >= argc) {
				printf("Not enough arguments!");
				return 1;
			}

			/* Consume one argument. */
			strncpy(nori_dev_name, argv[i+1], 16);
			i += 1;

			continue;
		}

		if(strcmp(option, "persistent") == 0) {
			/* The device will survive after NORI exit. */
			nori_dev_pers = 1;
			continue;
		}
	}

	return 0;
}

/******************************************************************************
 * ENTRY POINT.                                                               *
 ******************************************************************************/

int main(int argc, char ** argv) {
	/* User want to terminate this. */
	signal(SIGINT, handle_ctrlc);

	/* Direct invocation, no arguments. */
	if(argc < 2) {
		help();
		return 0;
	}

	pthread_spin_init(&nori_mt_lock, 0);

	/* Initialize RINA subsystem. */
	if(rina_init()) {
		printf("Failed to initialize RINA...\n");
		return 0;
	}

	/* Perform operations depending on what the user requested. */
	if(parse_args(argc, argv)) {
		return 0;
	}

	printf("Starting NORI instance %s-%s\n", 
		nori_name, nori_instance);

	nori_dev_fd = tun_create(nori_dev_name, nori_dev_type, nori_dev_pers);

	if(nori_dev_fd < 0) {
		printf("Cannot create TUN/TAP device.\n");
		goto stop;
	}

	/* We want async I/O. */
	tun_async_io(nori_dev_fd);

	/* Whatever happens, the dictionary is always the last argument. */
	if(dict_parse(argv[argc - 1])) {
		goto closefd;
	}

	/* Try to register an AE. */
	if(rina_create_AE(nori_name, nori_instance, nori_dif)) {
		goto closefd;
	}

	/* Does not return until the end. */
	nori_loop();

	/* Release a prevously allocated AE. */
	rina_release_AE(nori_name, nori_instance, nori_dif);

closefd:
	close(nori_dev_fd);
stop:
	/* Stop and dispose any waiting loop. */
	rina_stop();

	return 0;
}
