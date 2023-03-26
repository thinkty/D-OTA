# OTA
Implementation of the Over-The-Air Device-Firmware-Update on the ESP8266 running the ESP8266 RTOS SDK (based on FreeRTOS).

## Requirements

Make sure the development environment is set before building.
For how to setup the tools, see the [Get Started](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/) document from Espressif.

This project directory is expected to be in the `~/esp` directory along with the [*ESP8266_RTOS_SDK*](https://github.com/espressif/ESP8266_RTOS_SDK) and the *xtensa-lx106* toolchain since it is for the ESP8266 MCU.

Example:
```
    \esp
    |----ESP8266_RTOS_SDK/
    |----xtensa-lx106-elf/
    |----project/
```

For the description on the project structure, check the [document](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/build-system.html#example-project).

## Usage

Run `make menuconfig` to configure the project with the `sdkconfig` file, and make sure to save it.

The default serial port for your ESP8266 board can be set through `Serial flasher config > Default serial port`.
Depending on the OS, on Windows, the port will have names like `COM1`. On Linux, it will be like `/dev/ttyUSB#` or `/dev/ttyACM#`.

If an error message such as `Permission denied: '/dev/ttyACM1'` appears, run `sudo usermod -a -G dialout $USER` to add the current user to the group which has permissions or run `sudo chmod -R 777 /dev/ttyUSB0` to change the device to be accessible by all users, groups, etc.

| Command                      |                                                                               Description                                                                               |
|------------------------------|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| `make menuconfig`            |                                                 Open the configuration tool to create the `sdkconfig` configuration file                                                |
| `make all`                   | Compile your project code. In the first time running after installing ESP8266 RTOS SDK, the command will compile all components for the SDK as well and take some time. |
| `make flash`                 |                            Compile, generate bootloader, partition table, and application binaries, and flash app image to your ESP8266 board                           |
| `make flash ESPPORT= <port>` |                                                       Override the port specified in the sdkconfig file and flash                                                       |
| `make monitor`               |                                                              Open the serial monitor for your ESP8266 board                                                             |
| `make flash monitor`         |                                                               Compile, flash, and open monitor all-in-one                                                               |
| `make partition_table`       |                                                              Print the current partition table of the device                                                              |
| `make help`                  | For additional information about the toolchain or visit the [README](https://github.com/espressif/ESP8266_RTOS_SDK#compiling-the-project)                               |


Similar to overriding the port when flashing, the port for the monitor can be modfied as well.
See the [document](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#environment-variables) for more variables that can be specified.

## License

MIT

