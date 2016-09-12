# Makefile for NORI.
#
# Copyright (c) 2016 Kewin Rausch <kewin.rausch@create-net.org>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Contributors and changes:
#
CPP=g++
CC=gcc

#
# Update root folder to aim to IRATI userspace sandbox.
#	- ROOT stants from root folder of the stack.
# 	- SYSH stand for system headers files to use.
#	- US stand for user space headers files to use.
#
ROOT=/pristine
SYSH=$(ROOT)/headers
US=$(ROOT)/userspace

#
# Further organization for the compile action.
#
LIBS=$(US)/lib
LIBRINA_LIBS=-L$(LIBS) -lpthread 
INCLUDES=-I$(US)/include -I$(SYSH)/include

all:
	#
	# Wraps around the IRATI C++ system library.
	#
	$(CPP) $(INCLUDES) -c -Wall -Werror -fPIC rinaw.cc
	LD_LIBRARY_PATH=$(US)/lib $(CPP) $(INCLUDES) $(LIBRINA_LIBS) -shared -o librinaw.so rinaw.o -lrina
	
	#
	# Build nori.
	#
	LD_LIBRARY_PATH=$(US)/lib $(CC) -lpthread -o nori main.c dict.c tunw.c ./librinaw.so
	
clean:
	rm -rf *.o 
