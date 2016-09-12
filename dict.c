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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dict.h"

/* List of rules actually used. */
LIST_HEAD(dict_rules);

int dict_def_rule_parse(struct dict_rule * rule, char * str) {
	char * strategy = 0;
	char * name = 0;
	char * instance = 0;

	struct rule_default * def = malloc(sizeof(struct rule_default));
	struct rule_dest * d = 0;

	if(!def) {
		printf("        Not enough memory!\n");
		return -1;
	}

	INIT_LIST_HEAD(&def->dests);
	def->next = 0;

	/* Remember about the 'default' data. */
	rule->data = def;

	strategy = strtok(0, " ");

	if(strcmp(strategy, "rr") == 0) {
		def->strategy = RULE_STR_RR;
		printf("        Round-robin strategy selected.\n");
	} else {
		def->strategy = RULE_STR_SI;
		printf("        Single destination strategy selected.\n");
	}

	do {
		name = strtok(0, ",");

		if(name) {
			d = malloc(sizeof(struct rule_dest));

			if(!d) {
				printf("        Not enough memory!\n");
				return -1;
			}

			instance = strtok(0, " ");

			INIT_LIST_HEAD(&d->listh);
			strcpy(d->ae, name);
			strcpy(d->ai, instance);

			/* Add to the possible destinations. */
			list_add(&d->listh, &def->dests);

			/* Setup the first destination to use. */
			if(!def->next) {
				def->next = d;
			}

			printf(
"        New destination %s-%s added to rule\n", d->ae, d->ai);
		}
	} while(name);

	return 0;
}

int dict_ip_parse(struct dict_rule * rule, char * str) {
	int one = 0;
	int two = 0;
	int three = 0;
	int four = 0;

	char * token = 0;
	struct rule_ip * ip = malloc(sizeof(struct rule_ip));

	if(!ip) {
		printf("        Not enough memory!\n");
		return -1;
	}

	memset(ip, 0, sizeof(struct rule_ip));
	rule->data = ip;

	/* Get the direction. */
	token = strtok(0, " ");

	if(!token) {
		printf("        Bad direction format for IP rule!\n");
		return -1;
	}

	if(strcmp(token, "dst") == 0) {
		ip->direction = RULE_DIR_DST;
	} else {
		ip->direction = RULE_DIR_SRC;
	}

	/* Get the address. */
	token = strtok(0, " ");

	if(!token) {
		printf("        Bad address format for IP rule!\n");
		return -1;
	}

	sscanf(token, "%d.%d.%d.%d", &one, &two, &three, &four);

	ip->address[0] = (unsigned char)one;
	ip->address[1] = (unsigned char)two;
	ip->address[2] = (unsigned char)three;
	ip->address[3] = (unsigned char)four;

	/* Get the next hop. */
	memset(&ip->dest, 0, sizeof(struct rule_dest));
	token = strtok(0, ",");

	if(!token) {
		printf("        Bad next hop name format for IP rule!\n");
		return -1;
	}

	strncpy(ip->dest.ae, token, NAME_MAX);
	token = strtok(0, ",");

	if(!token) {
		printf("        Bad next hop instance format for IP rule!\n");
		return -1;
	}

	strncpy(ip->dest.ai, token, NAME_MAX);

	printf("        Direction %d, %d.%d.%d.%d --> %s-%s\n",
		ip->direction,
		(unsigned char)ip->address[0],
		(unsigned char)ip->address[1],
		(unsigned char)ip->address[2],
		(unsigned char)ip->address[3],
		ip->dest.ae,
		ip->dest.ai);

	return 0;
}

int dict_port_parse(struct dict_rule * rule, char * str) {
	char * token = 0;
	struct rule_port * port = malloc(sizeof(struct rule_port));

	if(!port) {
		printf("        Not enough memory!\n");
		return -1;
	}

	memset(port, 0, sizeof(struct rule_port));
	rule->data = port;

	/*
	 * Get the direction.
	 */

	token = strtok(0, " ");

	if(!token) {
		printf("        Bad direction format for Port rule!\n");
		return -1;
	}

	if(strcmp(token, "dst") == 0) {
		port->direction = RULE_DIR_DST;
	} else {
		port->direction = RULE_DIR_SRC;
	}

	/*
	 * Get the protocol.
	 */

	token = strtok(0, " ");

	if(!token) {
		printf("        Bad protocol format for Port rule!\n");
		return -1;
	}

	if(strcmp(token, "TCP") == 0) {
		port->proto = RULE_PORT_TCP;
	} else {
		port->proto = RULE_PORT_UDP;
	}

	/*
	 * Get the port.
	 */

	token = strtok(0, " ");

	if(!token) {
		printf("        Bad port format for Port rule!\n");
		return -1;
	}

	port->port = (unsigned short)atoi(token);

	/*
	 * Get the next hop.
	 */

	memset(&port->dest, 0, sizeof(struct rule_dest));
	token = strtok(0, ",");

	if(!token) {
		printf("        Bad next hop name format for Port rule!\n");
		return -1;
	}

	strncpy(port->dest.ae, token, NAME_MAX);
	token = strtok(0, ",");

	if(!token) {
		printf("        Bad next hop instance format for Port rule!\n");
		return -1;
	}

	strncpy(port->dest.ai, token, NAME_MAX);

	printf("        Direction %d, protocol %d, %d --> %s-%s\n",
		port->direction,
		port->proto,
		(unsigned short)port->port,
		port->dest.ae,
		port->dest.ai);

	return 0;

	return 0;
}

int dict_parse(char * path) {
	FILE * fd = fopen(path, "r");

	char * line = 0;
	size_t len = 0;
	ssize_t br = 0;

	char * token = 0;
	char * val1 = 0;
	char * val2 = 0;
	char * ap = 0;
	char * str = 0;
	char * ai = 0;

	int i = 0;
	int j = 0;

	struct dict_rule * r = 0;
	struct rule_dest * d = 0;

	if(!fd) {
		return -1;
	}

	/* Line per line read of the given dictionary...
	 */
	while((br = getline(&line, &len, fd)) != -1) {
		/* Remove any new line character. */
		for(i = 0; i < len; i++) {
			if(line[i] == '\n') {
				line[i] = 0;
			}
		}

		token = strtok(line, " ");

		if(token) {
			r = malloc(sizeof(struct dict_rule));

			if(!r) {
				printf("Not enough memory; loading stopped.");
				goto out;
			}

			memset(r, 0, sizeof(struct dict_rule));
			INIT_LIST_HEAD(&r->listh);

			printf("    '%s' rule detected\n", token);

			/* Default rule, something like:
			 *     default <strategy> (<name>,<instance>)1+
			 *
			 * Supported stratiegies:
			 *     si - single
			 *     rr - round robin
			 */
			if(strcmp("default", token) == 0) {
				if(dict_def_rule_parse(r, str)) {
					printf("Error!\n");
					free(r);
					r = 0;
					continue;
				}

				r->type = RULE_DEF;
				list_add_tail(&r->listh, &dict_rules);

				continue;
			}

			/* IP rule, something like:
			 *     default <direction> <address> <name>,<instance>
			 */
			if(strcmp("ip", token) == 0) {
				if(dict_ip_parse(r, str)) {
					printf("Error!\n");
					free(r);
					r = 0;
					continue;
				}

				r->type = RULE_IP;
				list_add_tail(&r->listh, &dict_rules);

				continue;
			}

			/* PORT rule, something like:
			 *     default <direction> <protocol> <port>
			 *     		<name>,<instance>
			 */
			if(strcmp("port", token) == 0) {
				if(dict_port_parse(r, str)) {
					printf("Error!\n");
					free(r);
					r = 0;
					continue;
				}

				r->type = RULE_PORT;
				list_add_tail(&r->listh, &dict_rules);

				continue;
			}


			printf("Rule %s not recognized...\n", token);
			/* Nothing was valid, if we are here... */
			free(r);
			r = 0;
		}
	}

out:
	fclose(fd);
	return 0;
}
