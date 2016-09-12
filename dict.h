/* NORI dictionary.
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

#ifndef __NORI_DICT_H
#define __NORI_DICT_H

#include <time.h>

#include "list.h"

/* Maximum name length considered. */
#define NAME_MAX	32

#define RULE_INVALID	0x0
#define RULE_DEF	0x1	/* PDU default destination. */
#define RULE_IP		0x2	/* Rule on IP address. */
#define RULE_PORT	0x2	/* Rule on port. */

#define RULE_PORT_UDP	0	/* PDU dest based on UDP port id. */
#define RULE_PORT_TCP	1	/* PDU dest based on UDP port id. */

#define RULE_DIR_SRC 	0	/* Look on source field. */
#define RULE_DIR_DST 	1	/* Look on destination field. */

#define RULE_STR_SI	0	/* A single destination. */
#define RULE_STR_RR	1	/* Round-robin between destinations. */

/* Rule destination. */
struct rule_dest {
	/* Member of a list. */
	struct list_head listh;

	/* Can use such one? */
	int open;
	/* Last retry operated on such destination? */
	struct timespec lrt;

	/* Target AE name. */
	char ae[NAME_MAX];
	/* Target AE instance. */
	char ai[NAME_MAX];
};

/* Port rule descriptor. */
struct rule_port {
	/* Source or destination filed? */
	int direction;
	/* Port number to check for. */
	unsigned short port;
	/* Protocol for this rule. */
	int proto;

	/* Destination for this rule. */
	struct rule_dest dest;
};

/* IP rule descriptor. */
struct rule_ip {
	/* IPv4 address to check for. */
	unsigned char address[4];
	/* Source or destination filed? */
	int direction;

	/* Destination for this rule. */
	struct rule_dest dest;
};

/* Default rule descriptor. */
struct rule_default {
	/* Strategy to apply. */
	int strategy;
	/* Next dest to use. */
	struct rule_dest * next;

	/* Possible destinations. */
	struct list_head dests;
};

/* Definition for a single rule. */
struct dict_rule {
	/* Member of a list. */
	struct list_head listh;

	/* Type of rule? */
	int type;

	/* Rule specific fields. */
	void * data;
};

/* List of rules. */
extern struct list_head dict_rules;

/* Parse a file in order to load up possible rules written in it.
 *
 * Returns 0 on success, a negative error number on error.
 */
int dict_parse(char * path);

#endif /* __NORI_DICT_H */
