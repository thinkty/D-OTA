# OTA
Implementation of the over-the-air device firmware update on the ESP8266 board running the [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK/tree/master).

## Requirements

Make sure the development environment is set before building.
For how to setup the tools, see the [Get Started](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/) document from Espressif.

### Project Directory

This project directory is expected to be in the `~/esp` directory along with the [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK) and the *xtensa-lx106* toolchain since it is for the ESP8266 MCU.

Example:
```
    \esp
    |----ESP8266_RTOS_SDK/
    |----xtensa-lx106-elf/
    |----project/
```

For the description on the project structure, check the [document](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/build-system.html#example-project).

### Internet Connection

Unlike the previous [project](https://github.com/thinkty/ESP8266-RTOS-OTA-DFU-AP), the ESP8266 device needs to connect to the internet in order to receive updates and publish sensor values.
In order to access the internet, this project utilizes an access point (WiFi).
The WiFi SSID and Password needs to be set in the configuration file (sdkconfig) which can be done using the command `make menuconfig` and selecting the `Example Configuration` option.

### Sensors

This project uses two additional sensors:

- DHT22 : for collecting temperature and humidity data ([app_task_dht.h](https://github.com/thinkty/OTA/blob/main/main/app_task_dht.h))
- SZH-SSBH-011 : a CDS photoresistor sensor to measure light intensity ([app_task_cds.h](https://github.com/thinkty/OTA/blob/main/main/app_task_cds.h))

The sensor values are read in a preset interval and publishes the data to the pub/sub broker [bridge](https://github.com/thinkty/bridge).

### Pub/Sub Broker

As mentioned above, the sensor values are published to this broker and also the device subscribes to the update topic to receive firmware updates.
[Bridge](https://github.com/thinkty/bridge) is just another hobby project of mine and it doesn't necessarily have to be Bridge.
The user can choose whatever messaging platform/server/protocol they want to use, but it will require some changes to the code to meet the communication protocols for that specific service.

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

