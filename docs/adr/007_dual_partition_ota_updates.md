# ADR 007: Dual-Partition Secure Boot OTA Updater

## Status
Approved

## Context
DeviceOS requires a resilient mechanism for remote firmware updates (Over-the-Air - OTA). The updater must protect against network drops, corrupted binaries, and unauthorized firmware injections (security attacks). It must operate on microcontrollers with two application partitions (`ota_0` and `ota_1`) and a bootloader.

## Decision
1. **Dual-Partition Swapping:** Implement active/inactive slot updating. The runtime streams firmware blocks sequentially into the inactive slot without affecting the active application code.
2. **Rolling digest Check:** Update a simple checksum incrementally as chunks write, avoiding caching the full binary in RAM.
3. **Cryptographic Validation:** After streaming, verify the signature using the ECDSA public key obtained from the `SecureElement` co-processor. If verification fails, abort and clear the slot.
4. **Boot Swap Flag:** Set active boot partition variables to boot from the updated partition upon next reset.
5. **Rollback Watchdog:** If the new firmware fails to initialize or triggers watchdog resets, the bootloader reverts boot flags to the previous stable slot.

## Consequences
* **Positives:** Bricking risks are minimized. Signature verification is isolated inside the secure element, keeping keys safe.
* **Negatives:** Flash requirement is doubled to support twin app partitions.
