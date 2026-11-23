# DPG pressure gauge bridge and display

`dpg-pressure.yaml` connects directly to the Bluetooth LE pressure gauge, subscribes to its measurement notifications, publishes pressure to Home Assistant through ESPHome's native API, and displays the same value locally with LVGL. The screen works without Home Assistant or Wi-Fi once the firmware is installed.

## Hardware

Configured for the Elegoo 2.8-inch 240x320 ESP32-WROOM-32E resistive-touch display (documented as E32R28T): ILI9341 LCD, 4 MB flash, and no PSRAM assumed. Elegoo documents ILI9341 even though this board has USB-C. The XPT2046 is the touch controller; touch is not required or configured for this interface.

| Function | ESP32 pin |
| --- | --- |
| LCD clock | GPIO14 |
| LCD MOSI | GPIO13 |
| LCD chip select | GPIO15 |
| LCD data/command | GPIO2 |
| Backlight PWM | GPIO21 |

LCD reset is wired to the ESP32 reset signal, so no reset GPIO is configured. LCD MISO/GPIO12 is unnecessary for this write-only interface and is omitted. GPIO2 and GPIO15 are onboard boot strapping pins; their warnings are suppressed only for the documented onboard connections.

The UI is a landscape pressure readout in psi, with a stream/connection status underneath. A 12% LVGL buffer uses about 18 KiB, and Bluetooth is limited to one connection to conserve RAM. The backlight is adjustable from Home Assistant. No external fonts, assets, or custom ESPHome components are needed.

If the screen is blank or colors are wrong, first check the actual board against the linked manufacturer documentation. Other boards sold as a "cheap yellow display" can have ST7789 or ILI9342 controllers. USB connector type alone does not identify the LCD. Change `display_model` only after identifying a different controller; the configured pins must also match. Adjust `lvgl.rotation` between 90 and 270 to reverse the landscape orientation.

## Install

Use ESPHome **2026.11.0 or newer**. Configuration validation and a full ESP32 firmware build passed with 2026.11.0 using dummy Wi-Fi secrets. The firmware fits in its OTA application partition. Seventeen synthetic-packet assertions passed against the actual C++ parsing lambdas, including signed readings, mbar conversion, invalid metadata, and unit changes. The user has confirmed basic pressure streaming and display operation; the new statistics sensors still need an on-device check.

1. Copy **both `dpg-pressure.yaml` and `pressure_statistics.h`** into the same ESPHome configuration directory (or create a device there and replace its YAML, then copy the header alongside it). The local header is required for compilation. If the file is in a subdirectory, place `secrets.yaml` alongside it or in an ancestor ESPHome configuration directory.
2. Supply `wifi_ssid` and `wifi_password` in `secrets.yaml`. A `secrets.yaml.example` is included; use a 2.4 GHz Wi-Fi network. Verify `gauge_mac` matches your gauge.
3. Set the gauge to **psi** initially. Close SECA/nRF Connect and disconnect other BLE clients so the ESP32 can connect. Keep the gauge awake and nearby.
4. Validate and install through ESPHome Device Builder, using USB for the first installation. Subsequent installations can use OTA. If using the CLI, run the following from this directory with ESPHome installed:

   ```sh
   esphome config dpg-pressure.yaml
   esphome compile dpg-pressure.yaml
   esphome run dpg-pressure.yaml
   ```

5. In Home Assistant, accept the discovered device under **Settings > Devices & services > ESPHome**, or add the ESPHome integration using the ESP32's IP address. It exposes `Pressure`, `Gauge connected`, `Gauge unit code` (diagnostic), `Display backlight`, `Pressure reporting interval` (configuration), `Minimum pressure`, `Maximum pressure`, `Average pressure`, and `Pressure statistics period` (configuration).
6. Confirm the screen and Home Assistant agree with several readings on the gauge, including zero. Power the gauge off: pressure should clear to `--.--` locally and become unknown in Home Assistant. Power it back on and check that the stream resumes.

## Home Assistant reporting interval

After installing the updated firmware, open this ESPHome device in Home Assistant and find **Pressure reporting interval** under its configuration controls. Set a value from **0.1 to 60 seconds** in 0.1-second steps. The default is **1 second**; the ESP32 saves the selected value across reboots.

This sets the minimum spacing between valid pressure readings forwarded to Home Assistant. For example, select 10 seconds to forward a fresh reading at most once every 10 seconds. It forwards the next arriving reading when the interval has elapsed, without averaging or inventing additional readings; if the gauge sends more slowly, Home Assistant also updates more slowly. A changed interval takes effect on subsequent readings.

The LVGL display continues to receive every gauge update. Disconnects, invalid data, and the existing 30-second stale-stream timeout clear Home Assistant's pressure immediately, even with a long reporting interval. The first valid reading after startup or recovery is also forwarded immediately. The public sensor is still named `Pressure` to retain its Home Assistant identity. The updated configuration passed ESPHome 2026.11.0 validation and full compilation, plus 18 synthetic timing assertions against the actual reporting filter (including runtime interval changes, failure/recovery, and timer rollover).

This control changes ESP32-to-Home-Assistant reporting only. It does not change the gauge's own measurement/BLE notification rate or write to `df471525`; the draft records that characteristic's read layout, but its write format has not been verified.

## Completed-window pressure statistics

The **Pressure statistics period** selector in Home Assistant offers exactly **1 minute, 5 minutes, 10 minutes, 15 minutes, 30 minutes, and 60 minutes**. It defaults to 1 minute and saves the choice across reboots. This is independent of the existing live **Pressure reporting interval** control.

Three sensors publish together once each selected period has elapsed:

| Sensor | Value for the immediately preceding completed window |
| --- | --- |
| Minimum pressure | Lowest valid pressure reading |
| Maximum pressure | Highest valid pressure reading |
| Average pressure | Arithmetic mean of all valid pressure readings |

All values are in psi and use every valid gauge notification before the Home Assistant reporting throttle. Windows are consecutive and non-overlapping. For example, with 5 minutes selected, the first report summarizes minutes 0-5, the next minutes 5-10, and so on. A sample arriving exactly at a boundary belongs to the next window. A one-second housekeeping timer closes windows even without incoming BLE data; actual publication can be up to about one second after a boundary due to scheduling, without moving the window boundaries.

The first window starts when the saved selection is restored during boot. Changing the selection discards the in-progress partial window and starts a full window of the new length immediately. These are elapsed-time windows, not wall-clock-aligned intervals. Published results remain visible until the next completed window; after a reboot, the sensors remain unknown until the first window completes. In-progress statistics are not persisted.

NaN, infinite values, disconnect events, and stale-stream signals do not count as readings. A window with no valid samples reports unknown for all three sensors. A window containing a data gap summarizes only its valid samples: no values are filled into the gap. The average is a sample mean, not a time-weighted average. Each completed window is reported even if its results equal those of the preceding window (`force_update: true`).

The local `pressure_statistics.h` helper keeps only a sample count, running sum, minimum, and maximum; it does not store a history of readings. ESPHome 2026.11.0 configuration validation and a full ESP32 firmware build passed for these statistics sensors. The focused C++ tests passed for all six periods, precise and delayed boundaries, empty windows, invalid readings, period changes, re-applying the same period, timer rollover, and full-rate averaging. To run them from this directory using a C++ compiler:

```sh
g++ -std=c++17 -Wall -Wextra -Werror tests/test_pressure_statistics.cpp -o /tmp/test_pressure_statistics
/tmp/test_pressure_statistics
```

For an on-device check, select 1 minute, observe several known readings, and confirm that the three sensors publish after the minute completes. Disconnect the gauge for a whole following window and confirm that all three become unknown at its end. Change the selection and confirm that the next report waits for a full new period.

## Pressure protocol and behavior

These UUIDs and packet offsets come from the draft's nRF Connect observations on 2026-11-22, not a published gauge protocol. Basic streaming has been confirmed on the physical gauge; unknown protocol fields and configuration writes remain unverified.

| Item | Interpretation |
| --- | --- |
| Service | `df471523-9da9-47cb-abf2-44ba6959979b` |
| Measurement characteristic | `df471524-9da9-47cb-abf2-44ba6959979b`; signed little-endian int32 at bytes 2-5 |
| Unit characteristic | `df47152b-9da9-47cb-abf2-44ba6959979b`; byte 1 is unit code, byte 2 is decimal places |
| Scaling | `reading = raw / 10^decimal_places` |
| Supported unit codes | 1 = psi; 6 = mbar (multiply by 0.0145038 to report psi) |

No pressure is accepted until unit/precision metadata has been received on the current connection. The metadata is read shortly after connection and every 5 seconds as well as subscribed to notifications, so startup does not depend on the gauge announcing an unchanged unit. Unsupported units, short packets, and decimal counts above 9 produce unknown pressure. On metadata changes, the old reading is cleared until a new pressure notification arrives.

The measurement characteristic is notification-only in this configuration: there is no periodic pressure polling and no write to the measurement-rate characteristic. Its observed reporting interval ranges from 100 ms to 10 seconds. Pressure becomes unknown on disconnect or after 30 seconds without a measurement. Unit polling does not refresh the pressure timeout.

Keep the gauge unit fixed while collecting data. Unit metadata and measurements arrive on separate characteristics without a known shared sequence number; changing units during a stream could briefly pair a reading with the previous scale. To change units reliably, disconnect/power off the bridge, change the gauge unit, then reconnect it. psi and mbar are supported; other codes have not been decoded.

The extra measurement fields at bytes 10-13 and 16-19 remain unused because their meaning is uncertain. A successful firmware build cannot verify the reverse-engineered protocol, notification behavior, physical display orientation, or free heap during live BLE/Wi-Fi operation. If the gauge connects but never streams, inspect its GATT properties and compare with nRF Connect before adding any guessed initialization writes. Temporarily set `logger.level: DEBUG` for BLE diagnostics.

## References

- [Elegoo 2.8-inch display introduction](https://wiki.elegoo.com/en/oshw-parts-%26-accessories/esp32-dispaly-introduction)
- [LCDwiki E32R28T controller and pin assignments](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)
- [ESPHome BLE client sensor and raw-data parsing](https://esphome.io/components/sensor/ble_client/)
- [ESPHome LVGL and display-buffer sizing](https://esphome.io/components/lvgl/)
- [ESPHome MIPI SPI display driver](https://esphome.io/components/display/mipi_spi/)

- [ESPHome template number controls](https://esphome.io/components/number/template/)
- [ESPHome copy sensor](https://esphome.io/components/copy/)
- [ESPHome template select controls](https://esphome.io/components/select/template/)
- [ESPHome template sensor](https://esphome.io/components/sensor/template/)
