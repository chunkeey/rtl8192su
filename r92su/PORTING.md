# r92su on Linux 6.18

Driver for Realtek RTL8188SU / RTL8191SU / RTL8192SU USB Wi-Fi (87 USB IDs,
`usb.c`). Since `r8712u` left staging (late 2024), no mainline driver handles
these chips.

- Source: <https://github.com/chunkeey/rtl8192su>, `r92su/` at `1f09c7a` (2017,
  around 4.11), by Christian Lamparter. It's a cfg80211 driver: the firmware
  does the 802.11 MLME/SME, and the driver implements `.connect`, `.scan` and
  the rest. Upstream calls it a work in progress (its TODO: station mode
  never fully validated, no hardware crypto, locking unverified, no AP mode).
- Firmware: `rtlwifi/rtl8712u.bin` (`MODULE_FIRMWARE`). It comes from Alpine's
  linux-firmware, and the modloop picks it up through modinfo.
- Build: `_extmods="drivers/r92su"` in a kernel APKBUILD.
  `scripts/cross-build-kernel.sh` builds it against that kernel (in a copy,
  with `-Werror`) and installs it in `lib/modules/<release>/updates/`.
- Status: compiles and is packaged. **Not tested on hardware yet.**

The pristine import is its own commit, so `git log -p drivers/r92su` shows
the whole port.

## API changes (mechanical)

| Was (4.11) | Now (6.18) |
|---|---|
| `<asm/unaligned.h>` | `<linux/unaligned.h>` |
| `ACCESS_ONCE()` | `READ_ONCE()` |
| `setup_timer()` + `unsigned long` callback, `del_timer*()` | `timer_setup()` + `timer_container_of()`, `timer_delete*()` |
| `struct timespec` + `ktime_get_ts()` | `ktime_get_seconds()` |
| `netif_rx_ni()` | `netif_rx()` (any context since 5.18) |
| `ndev->destructor = free_netdev` | `ndev->needs_free_netdev = true` |
| `.ndo_change_mtu = eth_change_mtu` | dropped (it's the default) |
| `memcpy(ndev->dev_addr, …)` | `eth_hw_addr_set()` |
| cfg80211 ops `add_key`/`del_key`/`set_default_key` | gained `int link_id` (MLO; ignored) |
| `set_wiphy_params` | gained `int radio_idx` (ignored) |
| `set_monitor_channel` | gained `struct net_device *` |
| `change_virtual_intf(…, u32 *flags, …)` | flags argument dropped |
| `ieee80211_data_to_8023_exthdr` / `ieee80211_amsdu_to_8023s` | new `data_offset`/`is_amsdu` and `check_sa`/`mesh_control` arguments (0/true, NULL/0: no mesh) |
| `ieee80211_data_from_8023()` (cfg80211) | removed upstream; local `r92su_data_from_8023()` in `tx.c` (station and IBSS cases only) |
| missing `fallthrough;`, `MODULE_DESCRIPTION` | added |

## Changes in behaviour (not just compile fixes)

- **WEP/TKIP RC4**: the `"arc4"` crypto_cipher no longer exists. Replaced by
  the library `arc4_setkey()`/`arc4_crypt()` (`CONFIG_CRYPTO_LIB_ARC4`, as
  mac80211 does). Each key has its own `struct r92su_rc4` with a spinlock,
  because TX and RX can rekey the same state concurrently. The old code
  shared one tfm with no lock.
- **CCMP**: the AEAD request and AAD were VLAs on the stack and went into a
  scatterlist. With `CONFIG_VMAP_STACK=y` (this kernel) that's invalid. Now
  one `kzalloc(GFP_ATOMIC)` per frame holds the request and a copy of the AAD,
  as in mac80211's `aead_api.c`. Encrypt now returns an error, and TX drops
  the frame rather than sending it unencrypted with a garbage MIC. Decrypt's
  short-frame check was `size_t <= 0` (never true); it's a real bounds check now.
- **Connect-time scan hang**: since 6.7 cfg80211 calls `.connect` with the
  wiphy mutex held. `r92su_internal_scan()` waited on `scan_done` with no
  timeout, and `survey_done_work` skipped `complete()` when the device was no
  longer open (unplug, ifdown). That hung connect while it held the wiphy
  mutex, and USB disconnect needs that mutex. Now the wait is bounded
  (2 × scan timeout + 1 s), `complete()` always runs, `ndo_stop` releases a
  waiter, and the completion is re-armed per scan so a stale count from an
  earlier scan can't let connect continue before the results arrive.
- **WEP RX length**: `ieee80211_wep_decrypt()` passed a length that included
  the ICV, where `_decrypt_data()` expects it excluded. It decrypted 4 bytes
  past the frame and checked the ICV in the wrong place, so every WEP frame
  failed. It now passes the length without the ICV (as TKIP already did),
  with a bounds check.
- **Key setup error paths**: `kfree(new_key)` followed by
  `PTR_ERR(new_key->…tfm)` (use after free) for WEP, TKIP and CCMP.

## Checked, no change needed

- Teardown: `unregister_netdev()` on a wdev netdev works without
  `cfg80211_register_netdevice()`; cfg80211's netdev notifier takes the wiphy
  lock itself, and the driver never takes it, so there's no lock inversion
  with `r92su->lock`.
- The cfg80211 calls it makes (`connect_result`, `disconnected`,
  `scan_done`, `ibss_joined`, `inform_bss`, `rx_mgmt`,
  `michael_mic_failure`) queue events and don't need the wiphy lock.
- `wiphy_register()` sanity checks: the ops are paired, there are no interface
  combinations, WoWLAN or ACLs. Nothing trips.
- The driver keeps its own BSS/station state, and it doesn't touch the wdev
  fields that MLO moved (`current_bss`, `ssid`).

## Known gaps

- Not run on hardware. Expect firmware/protocol issues only a device shows.
- `debugfs.c`, `trace.c` and the WPS button (`R92SU_WPC`) aren't built.
- No AP mode or hardware crypto, and TX status is approximate (upstream
  limitations).
