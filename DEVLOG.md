# Development Log
## Concept and Prototyping
The project originated on an Arduino board, driving an 8x8 LED matrix to mimic a beating heart. To increase visual resolution and enable remote interaction, the architecture was migrated to an ESP32 controlling a 16x16 matrix. The current hardware utilizes jumper wires and a breadboard for rapid iteration, with initial plans to transition to a permanent soldered implementation once the circuit and features are finalized. But this never came to fruition to my novice soldering skills at the moment. PlatformIO (PIO) was selected as the build environment for its robust library management and dependency tracking.

## Remote Deployment and CI/CD
I desired to deploy the device remotely as I live in Sweden and I'd be leaving the matrix behind in Mexico City. This required a robust Over-The-Air (OTA) update pipeline to push firmware patches without physical access. I implemented an automated CI/CD workflow utilizing GitHub Actions to decouple the build process from the physical hardware.

* **Automated Build:** Upon pushing code to the repository, GitHub Actions provisions a build environment, compiles the PlatformIO project, and generates the `.bin` firmware file.
* **Artifact Hosting:** The workflow uploads the compiled binary to a secure server to act as the host for the file.
* **MQTT Signaling:** An automated script publishes a lightweight MQTT payload to the controller's specific topic. This payload does not contain the code; instead, it contains the version metadata and the HTTPS download URL.
* **HTTPS Transfer Protocol:** The ESP32 listens for this MQTT trigger. Upon receipt, it initiates an HTTPS GET request to the provided URL. Using the ESP32's native OTA update libraries, the device streams the incoming binary data and writes it directly into its currently inactive OTA flash partition.
* **Boot Partition Swap:** Once the download completes and the file integrity is verified, the ESP32 updates its bootloader pointer to target the newly flashed partition and executes a software reset, booting into the updated firmware.

## Debugging Reliability and Hardware Timings
During testing, the LED matrix exhibited unreliable behavior, occasionally failing to update or illuminate. Troubleshooting this required isolating hardware, timing, and network variables:

1. **Hardware & State Verification:** A dedicated status LED that would blink red was implemented to indicate Wi-Fi connection state. Because this status LED remained functional even when the matrix failed, general hardware failure and power delivery issues were ruled out.
2. **Network Configuration:** To rule out cryptographic mismatches, I verified the router's security protocols (using `system_profiler SPAirPortDataType` locally to confirm WPA2 compatibility, which matches the ESP32's capabilities).
3. **Signal Integrity:** By monitoring the device's serial output over Unix (`screen 115200`), I analyzed the telemetry and discovered the RSSI (Received Signal Strength Indicator) was consistently weaker than -80 dBm. The packet loss was due to physical distance from the modem and wall interference, leading to a dedicated AP (access point) solution detailed below.

With the hardware abstraction and network infrastructure stabilized, development is shifting to the client-side interface: a mobile application for Android. This application will serve as a painting interface, formatting and publishing user inputs to the controller's MQTT API for real-time matrix rendering.

## Discussion: Design Tradeoffs and Architecture Evaluation
Designing this system required balancing real-time performance with remote reliability. Below is an evaluation of the core architectural decisions and their inherent tradeoffs:

### 1. Timing Abstraction: RMT Peripheral vs. CPU Bit-Banging
*   **Decision:** Investigated offloading WS2812 LED signal generation to the ESP32's Remote Control (RMT) peripheral to test a theory about interrupt collisions, but the final implementation relies on standard CPU bit-banging via the `FastLED` library.
*   **The Lesson:** When the matrix exhibited unreliable behavior, my initial hypothesis was that the ESP32's Wi-Fi radio stack was firing hardware interrupts that collided with the strict microsecond-level timing loops required by standard CPU bit-banging. Testing the RMT peripheral, which generates signals in hardware independently of the CPU, was a crucial diagnostic exercise. However, implementing RMT did not resolve the connection unreliability. This failed hypothesis was valuable because it systematically eliminated CPU timing interference as the culprit, forcing me to investigate network-layer issues. This ultimately revealed that physical interference and weak RSSI (-80 dBm) were the true root causes. Because RMT wasn't necessary for stability, I reverted to standard bit-banging to retain the familiar `FastLED` ecosystem and maintain cross-platform portability.
*   **Strength (RMT):** WS2812 LEDs require strict microsecond-level timing. Bit-banging these signals via the CPU requires disabling hardware interrupts, which instantly crashes the Wi-Fi stack. Using the RMT peripheral allows perfect hardware-driven timing while freeing the CPU to handle concurrent network traffic.
*   **Weakness (RMT):** Utilizing the RMT peripheral consumes dedicated hardware channels and relies on ESP32-specific APIs (ESP-IDF). This tightly couples the firmware to Espressif silicon, eliminating the ability to easily port the codebase back to standard Arduino AVR or RP2040 microcontrollers.

### 2. OTA Pipeline: HTTPS + MQTT vs. Pure MQTT
*   **Decision:** Split the OTA process into MQTT (for signaling) and HTTPS (for binary transfer).
*   **Strength:** HTTPS is purpose-built for large file transfers, handling TCP packet reassembly and TLS security natively. It bypasses MQTT broker payload limits and avoids overwhelming the ESP32's limited RAM.
*   **Weakness:** Increases system complexity. It requires maintaining an external hosting server (e.g., GitHub Releases) for the binaries and managing two distinct networking protocols simultaneously, increasing the potential surface area for network timeouts.

### 3. Build Environment: Cloud CI/CD (GitHub Actions) vs. Local Compilation
*   **Decision:** Automated the build process via GitHub Actions rather than relying on local PlatformIO compilation.
*   **Strength:** Ensures reproducible builds independent of the local machine environment and automates the remote deployment pipeline.
*   **Weakness:** Increases deployment latency for minor iteration testing and requires managing sensitive environment variables (MQTT credentials, Wi-Fi keys) within cloud infrastructure rather than keeping them strictly local.

### 4. OTA Strategy: A/B Partition Swap vs. In-Place Update
*   **Decision:** Utilized the ESP32's dual-partition (A/B) OTA update mechanism.
*   **Strength:** Provides fail-safe redundancy. If a network drop corrupts the incoming binary or the new firmware triggers a boot loop, the bootloader automatically reverts to the previous working partition, preventing a remote brick.
*   **Weakness:** Halves the available flash memory. Allocating space for two identical application partitions severely limits the remaining storage available for file systems (SPIFFS/LittleFS) or large application assets.

### 5. Network Infrastructure: Dedicated AP vs. ESP32 NAT Mesh
*   **Decision:** Installed a dedicated hardware router near the device instead of chaining a secondary ESP32 as a NAT repeater to solve RSSI degradation.
*   **Strength:** A dedicated Access Point guarantees maximum throughput, minimizes packet jitter (crucial for real-time matrix painting), and provides a highly stable -40 dBm RSSI.
*   **Weakness:** Requires higher physical footprint, relies on external consumer hardware, and increases the financial cost of the deployment compared to a purely software-driven mesh solution.
