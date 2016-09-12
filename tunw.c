/* Wrap around TUN/TAP functionalities.
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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include "tunw.h"

#define TUN_PATH	"/dev/net/tun"

int tun_async_io(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);

	if(flags < 0) {
		return flags;
	}

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int tun_close(int fd) {
	return close(fd);
}

int tun_create(char * name, int type, int persistent) {
	int fd = 0;
	int err = 0;

	struct ifreq i = {0};

	if(type == TUNW_MODE_TAP) {
		printf("TAP not supported yet.\n");
		return -1;
	}

	if(persistent) {
		printf("Creation of persistent device not supported yet.\n");
		return -1;
	}

	fd = open(TUN_PATH, O_RDWR);

	if(fd < 0) {
		return -1;
	}

	/* We go stright for a TUN at the moment. */
	i.ifr_flags = IFF_TUN;

	if(strlen(name) != 0) {
		strncpy(i.ifr_name, name, IFNAMSIZ);
	}

	err = ioctl(fd, TUNSETIFF, (void *)&i);

	if(err < 0) {
		close(fd);
		return -1;
	}

	/* Get the name assigned by the kernel. */
	strncpy(name, i.ifr_name, IFNAMSIZ);

	return fd;
}

int tun_read(int fd, char * buf, int size) {
	return read(fd, buf, size);
}

int tun_write(int fd, char * buf, int size) {
	return write(fd, buf, size);
}
