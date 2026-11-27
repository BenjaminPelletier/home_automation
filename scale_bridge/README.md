# TKO scale bridge

This ESPHome node passively listens for the TKO scale at `A0:77:9E:B8:2C:7D` and exposes **Weight** in kg through the native API. It uses the same Elegoo ESP32-32E / E32R28T board and ESP-IDF framework as `dpg_reader`. The sensor holds the last stable reading while the scale sleeps. After an ESP32 restart, it waits for a new reading.

## Install and check a weigh-in

1. In this directory, copy `secrets.yaml.example` to `secrets.yaml` and enter your 2.4 GHz Wi-Fi credentials. The Wi-Fi secret names match `dpg_reader`.
2. Connect the ESP32 by USB for the first install. Run these commands from this directory.

   ```sh
   esphome config scale-bridge.yaml
   esphome run scale-bridge.yaml
   ```

3. In Home Assistant, add the discovered **TKO Scale Bridge** ESPHome device. If discovery does not pick it up, add the ESPHome integration and enter the ESP32's IP address. [Home Assistant ESPHome setup](https://www.home-assistant.io/integrations/esphome/)
4. Set the scale to kg for the first test. Step on and stay until the display settles. Check for a `Stable weight` log line and compare **Weight** in Home Assistant with the scale display.

## Capture an unrecognized weigh-in

If the first weigh-in after boot produces no stable reading, install the same configuration with raw logging enabled. Capture one or two complete weigh-ins.

```sh
esphome -s scale_log_level DEBUG run scale-bridge.yaml
```

Send the `tko_scale` lines from before stepping on through stepping off, plus the settled display value and unit for each weigh-in. The logs include the manufacturer ID, RSSI, and payload for the configured MAC. Run the original install command to return to normal logging.

## What the screenshots show

The nRF Connect captures show 13-byte manufacturer payloads with a company ID ending in `C0` and the scale's own MAC in the last six bytes. The first two payload bytes are the big-endian weight. Properties `0x24` (seen in both screenshots) indicate an idle or settling value in hundredths of a kg. The first capture's `1D 65` decodes to 75.25 kg and the second capture contains zero. The configuration publishes stable kg packets with properties `0x25` (hundredths) or `0x21` (tenths). The stable flag still needs confirmation on this physical scale. [OKOK/Chipsea C0 protocol and reference captures](https://github.com/KristianP26/ble-scale-sync/commit/f779c1f)
