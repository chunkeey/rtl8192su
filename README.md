# r92su: Realtek RTL8188SU/RTL8191SU/RTL8192SU USB Wi-Fi driver

An out-of-tree cfg80211 driver for Realtek's RTL8188SU, RTL8191SU and RTL8192SU
USB Wi-Fi chips (87 USB IDs, see `usb.c`), installable with DKMS. These are
common, cheap 802.11n dongles. The kernel's staging driver for them, `r8712u`,
was removed after Linux 6.12, so newer kernels have no driver for them at all.

This is Christian Lamparter's r92su driver from
[chunkeey/rtl8192su](https://github.com/chunkeey/rtl8192su) (as of `1f09c7a`,
2017), ported to current kernels. The firmware does the 802.11 work, and the
driver implements station mode (`.connect`, `.scan` and the rest).

## Status and support

A hobby project, published as is.

- **Where it runs:** one dongle (`0bda:8171`) so far, on
  - an ARM board (PandaBoard, Linux 6.18): about an hour of on-board testing,
    then hours of streaming in a network speaker appliance;
  - x86_64 (Fedora 44, Linux 7.2.13): works, tested briefly.
- **Kernels:** 6.13 and newer (older kernels: use `r8712u`). Tested on 6.18 and
  7.2; 6.13 to 6.17 are untested.
- **Known gaps** are listed in [PORTING.md](PORTING.md). Among them: no AP mode,
  no hardware crypto, and WEP/TKIP, IBSS and roaming are untested.
- **No support promised.** Issues and patches are read eventually. New kernel
  releases may break the build until someone fixes it.
- **How it was made:** the port and its fixes were written with an AI assistant
  (Claude), checked against `r8712u` and tested on the hardware above, but not
  reviewed line by line. It runs in your kernel, where a bug can crash or hang
  the machine. Keep that in mind on a computer you care about.

## Requirements

- Kernel headers for the running kernel (Fedora: `kernel-devel`, Debian/Ubuntu:
  `linux-headers-$(uname -r)`, Arch: `linux-headers`).
- A kernel with `CONFIG_USB`, `CONFIG_CFG80211`, `CONFIG_CRYPTO_LIB_ARC4`,
  `CONFIG_CRYPTO_CCM` and `CONFIG_CRYPTO_AES`. Distribution kernels have them.
- The firmware `rtlwifi/rtl8712u.bin`, from linux-firmware (installed on most
  distributions; on Fedora it's in the `realtek-firmware` package).

## Install with DKMS

From a clone of this repository:

```sh
sudo dkms add .
sudo dkms build r92su/0.1
sudo dkms install r92su/0.1
```

DKMS then rebuilds the driver for each new kernel. The version (`0.1`) is
`PACKAGE_VERSION` in `dkms.conf`. To remove it again:

```sh
sudo dkms remove r92su/0.1 --all
```

**Secure Boot:** a kernel with Secure Boot enforced only loads signed modules.
DKMS signs the driver with a local key, which you have to enroll once (on most
distributions `mokutil --import` with the key DKMS reports, then a reboot to
confirm it).

## Build by hand

```sh
make                          # against the running kernel's headers
make KDIR=/path/to/kernel/build
make R92SU_WERROR=1           # treat warnings as errors
```

This only builds `r92su.ko`; it doesn't install or load it.

## License

GPL-2.0-only, as the source files' headers state.
