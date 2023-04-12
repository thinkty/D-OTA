# D-OTA
Implementation of the delta (differential) over-the-air device firmware update on the ESP8266 board running the [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK/tree/master).
Delta OTA update is different from a normal OTA update in a way that it receives only the differential of the previous firmware and the new firmware.
Therefore, the resulting transactions have relatively low overhead.

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

### Partitions

The flash partition is stored in the [partitions.csv](https://github.com/thinkty/OTA/blob/main/partitions.csv) to support OTA and also delta OTA updates.

The OTA partitions to store the new and current images are 640 KB each, and the storage for the delta file is 128 KB.
This means that the images should be less than 640 KB.
The storage partition is in between the two OTA partitions due to the required offset (that seems to be a [bug](https://github.com/espressif/ESP8266_RTOS_SDK/issues/1097)?).

To specify the partition table, run `make menuconfig`, select `Partition Table` > `Partition Table` > `Custom partition table CSV`, and specify the file name in `Custom partition CSV file`.
Details on the partition tables can be found [here](https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/docs/en/api-guides/partition-tables.rst).

In addition, to enable the file system to store the delta file, enable [SPIFFS](https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/storage/spiffs).

### Delta Patch Tools

There are various differential generator/patcher algorithms and compression algorithms available.
For this project, I've used [detools](https://github.com/eerimoq/detools) (>= 0.53.0) to create/apply the patch, and [heatshrink](https://github.com/atomicobject/heatshrink) (>= 0.4.1) to handle the compression/decompression of the patch.

To create the delta file, one can simply install the detools command line tool and run the following command.

```
detools create_patch --compression heatshrink old-image new-image patch-name
```

## Usage

Run `make menuconfig` to configure the project with the `sdkconfig` file, and make sure to save it.

The default serial port for your ESP8266 board can be set through `Serial flasher config > Default serial port`.
Depending on the OS, on Windows, the port will have names like `COM1`. On Linux, it will be like `/dev/ttyUSB#` or `/dev/ttyACM#`.

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

