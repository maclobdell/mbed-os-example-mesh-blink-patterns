# Blink Pattern - Thread mesh application for Mbed OS

With this application, you can use the [mesh networking API](https://os.mbed.com/docs/latest/reference/mesh.html) that [Mbed OS](https://github.com/ARMmbed/mbed-os) provides.

The application demonstrates a office/home alert system, where coded messages can be recorded on any device (by pressing a button in a pattern) and all the rest of the devices in the mesh will received the pattern and start blinking it on their LEDs.

The application uses two buttons and a tri-color LED with three inputs (one for each color).  

One button changes the LED color (red, green, or blue).  

The other button, when pressed in a pattern (think, "shave and a hair cut, two bits"), will cause the pattern timing (time between presses) to be stored, then repeated back by blinking the pattern on the LED.   The pattern has a maximum of 20 blinks.  

The application has been configured for Thread networks.

The application is based on https://github.com/ARMmbed/mbed-os-example-mesh-minimal.

## Setup

### Download the application

```
mbed import https://github.com/maclobdell/mbed-os-example-mesh-blink-patterns
cd mbed-os-example-mesh-blink-patterns
```


##### Thread commissioning

By default, the Thread application uses the static network link configuration defined in the [mesh API configuration file](https://github.com/ARMmbed/mbed-os/blob/master/features/nanostack/FEATURE_NANOSTACK/mbed-mesh-api/mbed_lib.json).
If you want to use the Thread commissioning, see [how to commission a Thread device in practice](https://os.mbed.com/docs/latest/tutorials/mesh.html#how-to-commission-a-thread-device-in-practice)

## Testing

By default the application is built for the LED blink pattern demo, in which the device sends a multicast message to all devices in the network when the button is pressed. All devices that receive the multicast message will change the LED blink pattern to the state defined in the message. Note, that the Thread devices can form a network without the existence of the border router. The following applies only to the case when the border router is set-up.

As soon as both the border router and the target are up and running you can verify the correct behaviour. Open a serial console and see the IP address obtained by the device.

<span class="notes">**Note:** This application uses the baud rate of 115200.</span>

```
connected. IP = 2001:db8:a0b:12f0::1
```

You can use this IP address to `ping` from your PC and verify that the connection is working correctly.


## Memory optimizations

On some limited platforms, for example NCS36510 or KW24D, building this application might run out of RAM or ROM.
In those cases, you might try following these instructions to optimize the memory usage.

### Mbed TLS configuration

The custom Mbed TLS configuration can be set by adding `"macros": ["MBEDTLS_USER_CONFIG_FILE=\"mbedtls_config.h\""]` to the `.json` file. The [example Mbed TLS config](https://github.com/ARMmbed/mbed-os-example-mesh-minimal/blob/master/mbedtls_config.h) minimizes the RAM and ROM usage of the application. The configuration works on K64F, but it is not guaranteed to work on every Mbed Enabled platform.

This configuration file saves you 8.7 kB of RAM but uses 6.8 kB of more flash.

### Disabling the LED blink pattern example

You can disable the LED blink pattern example by specifying `enable-led-control-example": false` in the `mbed_app.json`

This saves you about 2.5 kB of flash.

### Change network stack's event loop stack size

Nanostack's internal event-loop is shared with Mbed Client and therefore requires lots of stack to complete the security hanshakes using TLS protocols.
In case client functionality is not used, you can define the following to use 2kB of stack

`"nanostack-hal.event_loop_thread_stack_size": 2048`

This saves you 4kB of RAM.

### Change Nanostack's heap size

Nanostack uses internal heap, which you can configure in the .json. A thread end device with comissioning enabled requires at least 15kB to run.

In `mbed_app.json`, you find the following line:

```
"mbed-mesh-api.heap-size": 15000,
```

For 6LoWPAN, you can try 12kB. For the smallest memory usage, configure the node to be in nonrouting mode. See [module-configuration](https://github.com/ARMmbed/mbed-os/tree/master/features/nanostack/FEATURE_NANOSTACK/mbed-mesh-api#module-configuration) for more detail.

### Move Nanostack's heap inside the system heap

Nanostack's internal heap can be moved within the system heap.
This helps on devices that have split RAM and compiler fails to fit statically allocated symbols into one section, for example, the NXP KW24D device.

Mesh API has the [use-malloc-for-heap](https://github.com/ARMmbed/mbed-os/blob/master/features/nanostack/FEATURE_NANOSTACK/mbed-mesh-api/README.md#configurable-parameters-in-section-mbed-mesh-api) option to help this.

Add the following line to `mbed_app.json` to test:

```
"mbed-mesh-api.use-malloc-for-heap": true,
```

### Use release profile

For devices with small memory, we recommend using release profiles for building and exporting. Please see the documentation about [Build profiles](https://os.mbed.com/docs/latest/tools/build-profiles.html).

Examples:

```
$ mbed export -m KW24D -i make_iar --profile release
OR
$ mbed compile -m KW24D -t IAR --profile release
```

## Troubleshooting

If you have problems, you can review the [documentation](https://os.mbed.com/docs/latest/tutorials/debugging.html) for suggestions on what could be wrong and how to fix it.
