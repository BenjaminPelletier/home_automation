# ATOMS3 Lite notifier

Yes, ESPHome supports this using built-in components: an RGB light, a blinking effect, a GPIO button, and a selector exposed to Home Assistant. No custom component, MQTT broker, or separate PlatformIO project is needed. This is for the **ATOMS3 Lite** (ESP32-S3, 8 MB flash), with its one WS2812 RGB LED on GPIO35 and front button on GPIO41. See [M5Stack's hardware documentation](https://docs.m5stack.com/en/core/AtomS3-Lite). Other ATOM models have different hardware.

| Home Assistant option | LED |
| --- | --- |
| `Good` | Solid green |
| `Bad` | Red for one second, off for one second, repeating |
| `Clear` | Off |

The large **front button** selects `Clear` and stops blinking locally, even without Wi-Fi or Home Assistant. Home Assistant receives the updated selector state when connected. The small **side reset/download button** is for rebooting/flashing, not acknowledgement.

Every reboot/power loss starts in `Clear`. During a network outage the device keeps its current indication and tries to reconnect; automatic network timeout reboots are disabled. New remote commands require a connection. A later `Good` or `Bad` command replaces the cleared state; acknowledgement does not change the underlying Home Assistant sensor or condition.

## 1. Add the configuration

Use **ESPHome 2026.10.0 or newer**; update your existing installation if needed. This configuration uses ESP-IDF and encrypted wireless firmware updates.

1. Open your existing ESPHome Device Builder/dashboard. Copy `notifier.yaml` into its configuration directory (normally `/config/esphome/` inside the Home Assistant ESPHome app/add-on). Alternatively, create a device named `atom-notifier` in the dashboard, skip its initial install, choose **Edit**, replace all wizard-generated YAML with this file's contents, and save. If asked for a chip, choose ESP32-S3; the supplied YAML replaces the wizard's hardware settings.
2. Open **Secrets** in the dashboard, or edit `secrets.yaml` beside `notifier.yaml`. Merge in the keys from `secrets.yaml.example`, preserving existing entries. Enter your **2.4 GHz Wi-Fi** SSID/password. Generate a unique encryption key using the generator on the [ESPHome API page](https://esphome.io/components/api/#configuration-variables), and paste it into `notifier_api_encryption_key` in quotes. This same key secures the API and wireless firmware updates. Replace every placeholder.
3. Choose **Validate** from the notifier card's menu and fix any missing-secret or version errors.

ESPHome compiles YAML into firmware; saving YAML alone does not change the device. Keep your actual `secrets.yaml` private. The supplied `.gitignore` excludes it and generated build files from Git.

## 2. Install over USB the first time

This replaces the device's existing firmware. Use a USB-C **data** cable.

1. Connect the ATOMS3 Lite to your computer. In desktop **Chrome or Edge**, open the ESPHome dashboard, choose **Install → Plug into this computer**, allow access to the USB/serial port, and follow the prompts. This browser method requires HTTPS or localhost.
2. If that method is unavailable, choose **Install → Manual download**, compile, and download the **Factory** image (`firmware.factory.bin`, containing the bootloader and partition table). Open [ESPHome Web](https://web.esphome.io/) in Chrome/Edge, **Connect** to the device's port, and **Install** the downloaded file. Use this project's firmware rather than generic "prepare for first use" firmware. An OTA-only image is unsuitable for this first browser flash.
3. If connection fails, enter download mode: with USB connected, hold the small **side reset/download button** until the internal green indicator lights, then release. This is M5Stack's documented procedure. Re-select the USB port if it changes and retry. Use the side button, not the large front button.
4. After flashing, briefly press the side reset button or unplug/replug USB if it stays in download mode. The RGB LED starts off. Open ESPHome **Logs** to confirm Wi-Fi connection and find its IP address.

If USB is instead attached to the machine running ESPHome, select the install option for that machine's USB/serial port. Container/VM installations need USB passthrough. After the first install, the notifier needs only USB power and Wi-Fi; a USB power supply is sufficient.

## 3. Connect Home Assistant and your automation

In Home Assistant, open **Settings → Devices & services** and configure the discovered ESPHome device. If discovery fails, choose **Add integration → ESPHome** and enter `atom-notifier.local` or the IP address from the logs (API port `6053`). Supply your `notifier_api_encryption_key` value if prompted for the encryption/Noise key. See the [ESPHome integration guide](https://www.home-assistant.io/integrations/esphome/).

Find the device's **Notification state** entity. Its ID will usually be `select.atom_notifier_notification_state`; substitute the actual ID if yours differs. In your existing automation, add a **Select: Select option** action targeting this entity with option `Good` or `Bad` (case-sensitive). YAML examples:

```yaml
# Solid green
- action: select.select_option
  target:
    entity_id: select.atom_notifier_notification_state
  data:
    option: "Good"
```

```yaml
# Slowly blinking red
- action: select.select_option
  target:
    entity_id: select.atom_notifier_notification_state
  data:
    option: "Bad"
```

Use `option: "Clear"` in the same action to clear remotely. These are **Home Assistant automation action snippets**, not ESPHome YAML. There is no separate light entity: the selector owns the notification and LED together. You do not need to enable permission for the device to call Home Assistant actions. See [Home Assistant Select actions](https://www.home-assistant.io/integrations/select/#list-of-actions).

## 4. Verify and update

- Select `Good`: check steady green, then press the front button and check the LED turns off and the selector reads `Clear`.
- Select `Bad`: check one second red / one second off. Press during either phase and check it stays off. Test `Bad → Good` to check blinking stops.
- Power-cycle and check it starts in `Clear`. Disconnect Home Assistant or Wi-Fi while in `Bad` to check local blinking and acknowledgement continue.

An automation that repeatedly sends `Bad` will relight the notifier after acknowledgement. Trigger on the relevant event/state transition if you want acknowledgement to last until the next event.

Adjust `led_brightness` at the top of `notifier.yaml` (default `50%`) or the effect's two `duration` values to change brightness/timing. After edits, choose **Install → Wirelessly** while the powered device is reachable. Updates reboot into `Clear`. If name discovery fails, use its IP address; if Wi-Fi credentials are wrong, correct the secrets and reinstall by USB.

For an existing ESPHome command-line installation, run these from the directory containing the YAML and populated secrets. Choose USB for the first upload and the network address for later uploads:

```sh
esphome config notifier.yaml
esphome run notifier.yaml
esphome logs notifier.yaml
```

## Validation

Validated with **ESPHome 2026.10.2** using `esphome config notifier.yaml` and `esphome compile notifier.yaml --only-generate`, with temporary test credentials. Both passed. A complete firmware build and USB flash were not performed here; LED colors/timing, button behavior, and Home Assistant discovery still need verification on your physical device using the steps above.

