# flow_director

[ESP32 WROOM-32E](https://a.co/d/07rO5TLr) board that drives relays to control pool valves, the booster pump, the pool heater, and aux devices.

## Sensors

* Pool mode (controls both valve relays and reports the selected mode)
    * **Pool**: Intakes from the pool, returns to the pool.
    * **Spa fill**: Intakes from the pool, returns to the spa.
    * **Spa**: Intakes from the spa, returns to the spa.
    * **Spa drain**: Intakes from the spa, returns to the pool.
* Booster pump (on/off)
* Pool heater (on/off)
* Aux 2 (on/off)
* Aux 3 (on/off)
* Pressure (live DPG reading in psi)
* Minimum pressure, Maximum pressure, and Average pressure (last completed statistics period, in psi)
* Gauge connected, Gauge status, and Gauge unit code (Bluetooth diagnostics)
* IP address
* Minutes since boot
* Number of boots (saved across restarts)

## Bluetooth pressure gauge

Flow Director connects directly to the [DPG gauge](../dpg_reader). Home Assistant exposes two settings under the Flow Director device's configuration controls. Both are saved across reboots.

* **Pressure reporting interval** sets the minimum time between live pressure reports from **0.1 to 60 seconds** in 0.1-second steps. The default is **15 seconds**. The first valid reading after startup or recovery publishes immediately. After that, the next fresh reading is sent when the interval has elapsed. Pressure becomes unknown on disconnect, invalid data, or after 30 seconds without a measurement. The gauge can be set to psi or mbar.
* **Pressure statistics period** offers **1, 5, 10, 15, 30, or 60 minutes** and defaults to **1 minute**. Flow Director computes the minimum, maximum, and arithmetic mean from every valid reading. All three sensors update together when a period ends. An empty period reports unknown. Changing the period starts a new full period immediately and keeps the last completed results visible until it ends. Statistics start fresh after a reboot.

If **Gauge connected** stays off, make sure the gauge is awake and nearby. Close SECA or nRF Connect and disconnect the stand-alone DPG reader if it is using the gauge. Verify that `gauge_mac` in `flow-director.yaml` matches the gauge.

## Programming

Use [`flow-director.yaml`](./flow-director.yaml) for the initial FTDI flash and future updates. Before flashing, set `wifi_ssid`, `wifi_password`, and `flow_director_api_encryption_key` in an untracked `secrets.yaml`. Use a 2.4 GHz Wi-Fi network and generate the encryption key with `openssl rand -base64 32`.

When building away from this checkout, keep `flow-director.yaml` and `../dpg_reader/pressure_statistics.h` in the same relative layout. ESPHome needs the shared header when compiling an OTA update.

### Via OTA

After the first FTDI flash with this configuration, let the board boot and join Wi-Fi. From this folder, with ESPHome 2026.11.0 or newer, run:

```sh
esphome run flow-director.yaml --device flow-director.local
```

If the hostname does not resolve, replace it with the controller's IP address. Keep the same API encryption key used for the FTDI flash. The board reboots into Pool mode with the booster pump, pool heater, and aux relays off.

### Via FTDI

Pin headers (left to right with ESP above):

1. 5V
2. TX (green)
3. RX (blue)
4. Gnd (black)
5. Gnd
6. IO0

To put into programming mode: hold IO0 when powering up, then release.

## Valves

| Valve cable | Electrical | Flow director connector |
|-------------|------------|-------------------------|
| Black       | Common     | Red                     |
| Red         | NC, pool   | Black                   |
| White       | NO, spa    | Yellow                  |

## Peripheral connectors

| Location | Purpose      | Relay | GPIO |
|----------|--------------|-------|------|
| Left     | Intake valve | 5     | 27   |
|          | Return valve | 4     | 26   |
|          | Booster pump | 8     | 13   |
|          | Aux 2        | 7     | 12   |
| Right    | Aux 3        | 6     | 14   |
| -        | Pool heater  | 1     | 32   |
| -        | -            | 2     | 33   |
| -        | -            | 3     | 25   |
