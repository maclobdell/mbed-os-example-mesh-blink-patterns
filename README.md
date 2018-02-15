# Blink Patterns - Thread mesh application for Mbed OS

With this application, you can use the [mesh networking API](https://os.mbed.com/docs/latest/reference/mesh.html) that [Mbed OS](https://github.com/ARMmbed/mbed-os) provides.

The application demonstrates a office/home alert system, where coded messages can be recorded on any device (by pressing a button in a pattern) and all the rest of the devices in the mesh will received the pattern and start blinking it on their LEDs.

The application uses two buttons and a tri-color LED with three inputs (one for each color).  

One button changes the LED color (red, green, or blue).  

The other button, when pressed in a pattern (think, "shave and a hair cut, two bits"), will cause the pattern timing (time between presses) to be stored, then repeated back by blinking the pattern on the LED.   The pattern has a maximum of 20 blinks.  

The application has been configured for Thread networks.

The application is based on [mbed-os-example-mesh-minimal](https://github.com/ARMmbed/mbed-os-example-mesh-minimal).

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

### Compile the application

```
mbed compile -m KW24D -t ARM
```

A binary is generated in the end of the build process.

### Program the target

Drag and drop the binary to the target to program the application.

As soon as the target is up and running you can verify the correct behaviour. Open a serial console and see the IP address obtained by the device.

<span class="notes">**Note:** This application uses the baud rate of 115200.</span>

```
connected. IP = 2001:db8:a0b:12f0::1
```

You can use this IP address to `ping` from your PC and verify that the connection is working correctly.


## Tested Hardware & Compiler

This application was tested with [FRDM-KW24D512](https://os.mbed.com/platforms/FRDM-KW24D512).
It should work just fine with many other boards, but they haven't been tested yet.

Also, it was tested with Arm compiler.  It can be build with Mbed CLI or in the [Mbed Online Compiler](https://os.mbed.com/compiler)
Memory optimizations will likely be required

## Memory optimizations

For information on memory optimizations that might be required for different hardware, see [mbed-os-example-mesh-minimal](https://github.com/ARMmbed/mbed-os-example-mesh-minimal).
