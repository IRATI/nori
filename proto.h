/* Provides information to decode flowing traffic.
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

#ifndef __NORI_PROTO_H
#define __NORI_PROTO_H

/* 
 * Displacement of TUN interfaces. 
 */

#define TUN_INITIAL_OFFSET	4

/* 
 * Displacement starting from IP header. 
 */

#define IPV4_PROTO_OFFSET	9
#define IPV4_SOURCE_OFFSET	12
#define IPV4_DEST_OFFSET	16
#define IPV4_HEADER_SIZE(x)	((x  & 0x0f) * 4) /* Using IHL */

/*
 * Displacements for ICMP protocol.
 */

#define IPV4_ICMP_CODE		1

/* 
 * Displacements valid for both TCP and UDP protocols. 
 */

#define IPV4_TDP_DESTP_OFFSET	2

/* 
 * Displacements for TCP protocol. 
 */

/* 
 * Displacements for UDP protocol. 
 */

#endif /* __NORI_PROTO_H */
