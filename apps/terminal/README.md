# template

This is a template for starting new 3DS libctru projects.


**Current milestone:** T1 HARDWARE PASS

### T1 — Native Backend Networking

Hardware verified on New Nintendo 3DS:

- Runtime server configuration loaded from `sdmc:/3ds/INFINIT3/config.ini`
- No permanent LAN/DHCP address is compiled into the application
- Native libctru socket networking initializes successfully
- HTTP GET `/market.txt` succeeds against the existing INFINIT3 backend
- Live NAS100, US30, and GOLD values render on hardware
- `X` performs a successful network retry/refetch
- `START` exits cleanly to Homebrew Launcher
- No crash, freeze, reboot, or exception observed

T1 tested `.3dsx` SHA-256:

`014c30c045f70b3e6b93c94c643a6e3e85675bafaf2091a75b178200a8be2054`

