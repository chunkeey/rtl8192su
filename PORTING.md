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
- Build (in the pandaboard_alpine appliance, where the port was made): in `DEFAULT_EXTMODS` (`scripts/cross-build-kernel.sh`), so every
  board kernel gets it unless its APKBUILD sets
  `_extmods_skip="drivers/r92su"`. The script checks `kconfig.require`
  against the kernel config, builds the driver against that kernel (in a copy,
  with `-Werror`) and installs it in `lib/modules/<release>/updates/`.
- Status: works on the PandaBoard (on-board results below); cross-checked
  against r8712u. Built for the BeagleBoard-xM and pcDuino kernels too, not
  run on them yet.

In this repository the port starts after upstream's `1f09c7a`, so
`git log -p 1f09c7a..` shows the whole port.

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
- **Signal in dBm**: the wiphy said `CFG80211_SIGNAL_TYPE_UNSPEC` (0–100),
  and current nl80211 only sends a station's signal for `..._MBM`, so
  wpa_supplicant's `SIGNAL_POLL` gave `RSSI=-9999` (seen on the board with
  r5). The firmware only reports quality 0–100, so it's now converted with
  Realtek's `dBm = pct/2 − 95` (rtl8723bs `translate_percentage_to_dbm`) for
  the station, the scan results (which were raw 0–100 sent as unspec) and
  the connected BSS. So -95 to -45 dBm: a mapped quality, not a measured
  power.
- **RX replay counters per TID** (r12): CCMP/TKIP kept one replay counter
  per key and dropped any frame whose PN wasn't higher than the last. The AP
  numbers frames from one counter but sends its AC queues independently, so
  interleaved traffic classes (e.g. SSH bulk as CS1/background next to best
  effort) lost valid frames. Now one counter per TID plus one for non-QoS,
  as 802.11 and mac80211 (`rx_pn[IEEE80211_NUM_TIDS + 1]`).
- **RX reorder window 64** (r12), as r8712u (`wsize_b`), not 32 ("taken
  from a monitor"): with 32, gaps were given up while the AP still had
  frames in flight, and the retransmissions dropped as late.
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

- Tested on one dongle (0bda:8171) and one AP for about an hour. Not
  tested: AP restarts and roaming, WEP/TKIP, IBSS, a
  real (not simulated) fatal USB error.
- RX on a busy link still drops about 0.9 % in the reorder buffer: mostly
  real duplicates (the AP resent frames after missing our block-ack) and
  frames later than the 100 ms reorder timeout. Not chased further.
- After a software replug (USB `authorized` 0/1, no power cycle) the first
  firmware upload fails (`cpu_status=0x9`: IMEM checksum not reported); the
  next open works.
- No TX rate is reported (`SIGNAL_POLL` says `LINKSPEED=0`).
- `debugfs.c`, `trace.c` and the WPS button (`R92SU_WPC`) aren't built.
- No AP mode or hardware crypto, and TX status is approximate (upstream
  limitations).

## Cross-check against r8712u (Linux 6.12)

`drivers/staging/rtl8712` in v6.12, the last kernel that had it, was the
reference: it drove this hardware for about 14 years and loads the same
`rtl8712u.bin`.

**Same in both drivers**: the firmware header and DMEM (`fw_priv`) layout;
the TCR upload status bits and the upload order (IMEM, EMEM, CPU enable,
DMEM, FWRDY); the B/C-cut power-on sequence (register for register: EFUSE
leakage, SPS0, AFE, LDOs, PLL 0x11/0x51/0x11, clock switch, CR = 0x37FC,
0xFE5C bit 7, 0xFE1C, TXDMA poll); the USB register addressing (wValue = the
address's low 16 bits); the H2C command IDs 0–74 and C2H event IDs 0–25; the
`wlan_bssid_ex` (join/survey), set-key, set-sta-key, set-auth and site-survey
layouts, and the cipher IDs; the TX descriptor bits (pkt size/offset/OWN/
FSG/LSG, MACID, QSEL, NON_QOS, KEY_ID, SEC_TYPE, BMC, priority in the dword-3
seq field, the 1M forced rate for EAPOL/ARP/DHCP); the priority-to-QSEL map;
the RX 128-byte packet stepping; the MAC address at efuse 0x12; the chip
revision field (PMC_FSM bits 15–19).

**Fixed, found by the comparison**:
- RX loop: a data frame received while not connected hit `continue`, which
  skipped the advance. The loop re-read that packet and dropped the rest of
  the aggregated transfer, firmware events (survey, join) included. The C2H
  clipping check also used the transfer's total length rather than what
  remains.
- H2C padding (`r92su_h2c_copy`, `r92su_h2c_connect`) was `8 - len % 8`:
  an aligned command got 8 extra bytes, declared in the H2C length and put
  past the allocation. Now `round_up(len, 8)`, as r8712u does.
- Probe refused a device whose EEPROM id isn't 0x8129 (r8712u never checks
  it), an unknown board type (r8712u assumes 1T2R), or an unknown chip
  revision (r8712u and the vendor code assume B-cut). All three now warn and
  use r8712u's defaults.
- The wait after the core reset in power-on: 1 ms, now 20 ms as in r8712u.

**Different on purpose, the first suspects if something fails**:

| | r92su | r8712u | to try |
|---|---|---|---|
| USB RX aggregation (0x102500D9) | 48 pages (6 KB) | 1 ("invalidates usb rx aggregation") | `r92su.norxagg=1` |
| DMEM: A-MPDU, aggregation offload, 40 MHz | on (unless `noht`) | off / off / on | `r92su.noht=1` |
| DMEM: QoS, rate control and MLME offload | on | off | needs a rebuild |
| DMEM: turbo_mode | 0 | 1 | needs a rebuild |
| DMEM: rf_config | from the EEPROM board type | always 1T2R (module parameter) | r92su is more accurate |
| TX length a multiple of 512/64 | `URB_ZERO_PACKET` | 8 bytes of padding + descriptor offset +8 | needs a rebuild |
| H2C seq bit 7 | "last" flag, 7-bit counter | plain 8-bit counter | firmware evidently ignores it |

Module parameters go on the kernel command line (`r92su.norxagg=1`) or in
`/etc/modprobe.d/` before the dongle is plugged in.

## On the board (r5–r13)

- r5/r7: probes (C-cut, 1T1R), firmware 0x11ec boots, associates (WPA2-PSK,
  CCMP), DHCP lease. r7 reports the signal in dBm.
- r9 works both ways: a 5 MB TCP download over wlan0 at 34.8 Mbit/s
  (wlan0 rx +5.18 MB, eth0 +674 bytes), checksum intact; RX mostly MCS 7,
  ~3.3 packets per USB transfer (RX aggregation), 30 late frames dropped by
  the reorder buffer (r8's RX diagnostics, see below).
- "Unicast IP never arrives" (r7) was the test setup, not the driver: eth0
  and wlan0 on the same subnet. ARP flux made eth0 answer for wlan0's
  address (the router and the PC sent replies to eth0's MAC), and strict
  rp_filter dropped what did arrive on wlan0. With arp_ignore=1,
  arp_announce=2 and rp_filter=2 it works. That's an appliance setting,
  not a driver one, and it affects the onboard WL1271 too.
- r8 (diagnostics) hung at boot: rx_alignment was 0 until open's late MAC
  setup, and the leftover scan didn't advance (soft lockup, the trial boot
  fell back to r7). Fixed in r9: 128 from the start.
- r7: once open failed after boot, every later open said -EAGAIN (state not
  STOP) until a rebind. r8 logs the failing open step and who marks the
  device dead.
- r10: recovery from USB errors, as r8712u (it retries RX `-EPROTO`, drops
  failed TX frames, and its open clears `driver_stopped` and re-inits).
  RX `-EPROTO` is resubmitted (up to 32 in a row), TX errors other than
  "device gone" drop the frame. A dead device that's still attached is
  revived (URBs killed, RX re-armed, state STOP) by `ndo_stop` or the next
  open, which redoes power-on and firmware upload; dead while up, a work
  item does down + up (under rtnl, at most every 30 s). On the board: a
  simulated fatal error while connected restarted the interface in 1 s and
  it reconnected within 6 s; a software unplug/replug re-probed and was
  back after about 9 s (the first open failed, see Known gaps). A 116 MB
  OTA bundle went over wlan0 in 63 s with no USB errors.
- r12 A/B on one boot (test_old_rx, since removed): 116 MB SSH bulk
  (background AC) plus 5 × 5 MB HTTP (best effort) received over wlan0.
  Old: 100,744 unicast frames, 1,010 replay-check drops, 1,856 reorder
  drops (2.84 %), 38 s. New: 99,104 frames, 0 replay-check drops, 921
  reorder drops (0.93 %), 34 s.
- RX reorder: the window now syncs to the first frame of a BA session, as
  r8712u does (it ignores the firmware's ADDBA-report start sequence).

## Diagnostics (removed; restore on request)

Removed so normal runs are quiet (first in r11; back for r12's A/B test;
removed again in r13). `git revert 9cbeaef` ("r92su: remove the A/B
scaffolding") brings them back, together with `test_old_rx` (1 = the old
single replay counter and 32-frame reorder window; the window applies to
BA sessions set up afterwards). Then bump the kernel pkgrel. What they do:

- an `rxdbg:` line every 10 s while connected and the counters change:
  USB transfers, packets, C2H events, data frames (unicast to us,
  multicast, CRC-flagged, HT), descriptors left over after `pkt_cnt` ran
  out, data frames by RX rate index (0–3 CCK, 4–11 OFDM, 12+ MCS), and
  drops by the handler that dropped them (`r92su_rx_*`);
- `test_mark_dead` (module parameter, writable): `echo 1 >
  /sys/module/r92su/parameters/test_mark_dead` marks the device dead on
  the next service work, as a fatal USB error does (tests recovery).

Kept, because they only print when something fails: "open: <step> failed",
"device marked dead by <function>", the USB error reasons, "rx/tx urb …",
and "reviving" / "restarting the interface".
