# Nfv Over RIna, NORI

NORI software allow to use untouched IP existing application over RINA technologies by "tunneling" the traffic using a given rule set. This means that you can use your software with a minimal reconfiguration, but with absolutely no recompilation/code adjustments, over recursive internetworking technologies.

Using this approach the upper IP layer has absolutely no knowledge of the underlay network, and this can be changed as you need to match quick reconfiguration without minimal disruption of the service.

The name can be a little confusing, but I keep it for historic purposes. The tool was born to provide chaining of IP traffic between different nodes, thus leveraging the concept of service chaining and Network Function Virtualization, but now allows to operate on a larger set of configurations.

### License

This software is distributed under Apache 2.0 license.  
See the license file for more info.

### Compatibility

This software was initially bound to IRATI implementation of RINA, but a wrapper allows to extend it with other stack implementations. Interaction with the RINA stack is mainly done by `rinaw`.

This software is ready-to-use if you are using the IRATI stack, otherwise you will need to update `rinaw` with your own glue for the RINA stack. The repository will be probably updated in the future to support such new emerging alternatives (if they come out, of course).

### Pre-requisites

The software need the following libraries & tools to be compiled and run:

* Pthread for multi-threading.
* TUN/TAP capabilities.
* A RINA stack.

### Compile

If you are using the IRATI stack, everything is already set-up and ready to be used. You just need a minimal adjustment in the `makefile` in order to point to the headers and libraries of the stack.

Generic steps in order to build the software are:

1. Compile and install the desired RINA stack on your machine.
2. Modify ROOT, SYSH and US variables in order to point to the root, system headers and userspace stuff of the stack. This is necessary if you install the stack in a particular folder to keep it separate from the machine standard files.
3. Invoke the make.

If you are not using a supported RINA stack, you will need to adjust the `rinaw` wrapper in order to match the desired stack implementation system libraries calls. No other changes are necessary. 

### Dictionary syntax

The syntax of the dictionary does NOT follow a standard and is really simple by now. Enhancement on the dictionary will come on the future with the next releases. You have to specify one rule per line, and that will be order taken in account by the application to evaluate them. To apply a permessive behavior you can specify a *default* rule (but keep it as the last one), otherwise the packets will be discarded by the application (following the policy: no rule, no party).

There are just some options available for the moment, which are:

* **IP**, syntax: `ip <src/dst> <address> <name>,<instance>`  
IP rules will route traffic by checking the IP address (destination or source). If the match is successfull, the packet is sent to the given  application. You can specify if to consider source or destination address.   


* **Default**, syntax: `default <strategy> <name>,<instance>*`  
Default rule will route all the traffic, regardless of the type, to one or more destinations, depending on the strategy chosen. Keep the default rule as the last one in the dictionary, because it will "eat" up al the other rules. Do not specify a default if you want to block all the traffic which does not match one of your rules.    
The available strategies, for the moment, are: 
    - **si**, single instance, which means only one destination; 
    - **rr**, round-robin strategy, which means send one packet per destination in a round-robin style. First packet is sent to the first destination, second to the second, the n+1-th packet (assuming 'n' destinations) is sent to the first again.  

### Run NORI

To use NORI you need to invoke the program like this:
`./nori <name> <instance> <dif_to_use> <dictionary>`

The software will create a tunX interface (depending on your system), and you can proceed by setting an IP address. Support for more personalization (give the name you desire, use TAP instead of TUN, etc...) will be added with future updates of the software.

### Known limitations

* Actually performances with NORI **are limited**, since it adds an additional computation step to the overall data path. This can be improved using different type of strategies for packet processing, like introducing zero-copy or multi-threading. This has to be evaluated carefully.


* **MTU of the interfaces must be adjusted** to include additional space for RINA headers. Usually setting it to 1400 make it work without any problem. This limitation is especially present while using IRATI shim over ethernet.