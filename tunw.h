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

#ifndef __NORI_TUNW_H
#define __NORI_TUNW_H

/* We are speaking of a TUN device. */
#define TUNW_MODE_TUN		0
/* We are speaking of a TAP device. */
#define TUNW_MODE_TAP		1

/* Switches the I/O method for the tun FD to non-blocking operations.
 *
 * Returns 0 on success, a negative error number on error.
 */
int tun_async_io(int fd);

/* Closes a previously allocated tun/tap device using it's fd.
 *
 * Returns 0 on success, a negative error number on error.
 */
int tun_close(int fd);

/* Creates a brand-new tun/tap device. Specifying a 0 in the name field leave at
 * the kernel the decision of the name to apply to such interface.
 *
 * You also have to specify the type of device you want to spawn (TUN or TAP?).
 * You can choose persistent=1 if you want the device to remain active.
 *
 * Returns the number of bytes read, a negative number on error.
 */
int tun_create(char * name, int type, int persistent);

/* Read from a tun/tap device.
 *
 * Returns the number of bytes read, a negative number on error.
 */
int tun_read(int fd, char * buffer, int bufsize);

/* Writes into a tun/tap device.
 *
 * Returns the number of bytes written, a negative number on error.
 */
int tun_write(int fd, char * buffer, int bufsize);

#endif /* __NORI_TUNW_H */
