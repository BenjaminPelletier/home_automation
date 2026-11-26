# flow_director

[ESP32 WROOM-32E](https://a.co/d/07rO5TLr) board that drives relays to control pool valves, the booster pump, and aux devices.

## Sensors

* Pool mode (controls both valve relays and reports the selected mode)
    * **Pool**: Intakes from the pool, returns to the pool.
    * **Spa fill**: Intakes from the pool, returns to the spa.
    * **Spa**: Intakes from the spa, returns to the spa.
    * **Spa drain**: Intakes from the spa, returns to the pool.
* Booster pump (on/off)
* Aux 2 (on/off)
* Aux 3 (on/off)
* Pressure (fixed at 0.0 psi every 15 seconds until the [DPG Bluetooth gauge](../dpg_reader) is integrated)
* IP address
* Minutes since boot
* Number of boots (saved across restarts)

## Programming

Use [`flow-director.yaml`](./flow-director.yaml) for the initial FTDI flash and future updates. Before flashing, set `wifi_ssid`, `wifi_password`, and `flow_director_api_encryption_key` in an untracked `secrets.yaml`. Use a 2.4 GHz Wi-Fi network and generate the encryption key with `openssl rand -base64 32`.

### Via OTA

After the first FTDI flash with this configuration, let the board boot and join Wi-Fi. From this folder, with ESPHome 2026.11.0 or newer, run:

```sh
esphome run flow-director.yaml --device flow-director.local
```

If the hostname does not resolve, replace it with the controller's IP address. Keep the same API encryption key used for the FTDI flash. The board reboots into Pool mode with the booster pump and aux relays off.

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
| -        | -            | 1     | 32   |
| -        | -            | 2     | 33   |
| -        | -            | 3     | 25   |
