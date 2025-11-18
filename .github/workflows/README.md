# GitHub Actions Workflows

This directory contains CI/CD workflows for the Inky C Library.

## Workflows

### 1. Emulator Build (`emulator-build.yml`)
**Platforms**: Ubuntu x64, macOS, Windows x64

**Purpose**: Builds and tests the emulator library on non-Raspberry Pi platforms.

**What it does**:
- Builds `libinky_emu.a` static library
- Compiles and runs test programs
- Generates test images (PPM files)
- Uploads library and test artifacts

**Triggers**:
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches

**Artifacts**:
- `libinky-emulator-{os}` - Emulator library for each platform
- `test-images-{os}` - Generated test images

---

### 2. Hardware Build (`hardware-build.yml`)
**Platforms**: Raspberry Pi (32-bit armv7l, 64-bit aarch64)

**Purpose**: Builds the hardware library on self-hosted Raspberry Pi runners.

**What it does**:
- Verifies running on ARM Linux (Raspberry Pi)
- Builds `libinky_hw.a` static library with GPIO/SPI/I2C support
- Uploads hardware library artifacts

**Triggers**:
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches

**Artifacts**:
- `libinky-hardware-armv7l` - Hardware library for 32-bit Raspberry Pi
- `libinky-hardware-aarch64` - Hardware library for 64-bit Raspberry Pi

**Note**: This workflow requires self-hosted GitHub Actions runners on Raspberry Pi devices.

---

## Setting Up Self-Hosted Runners

To enable the hardware builds, you need to set up self-hosted runners on your Raspberry Pi devices:

### On each Raspberry Pi:

1. Go to your GitHub repository → Settings → Actions → Runners
2. Click "New self-hosted runner"
3. Follow the instructions for Linux ARM
4. Add labels to identify the architecture:
   - For 32-bit Pi: Add label `armv7l`
   - For 64-bit Pi: Add label `aarch64`

### Install runner as a service:

```bash
# After configuring the runner
sudo ./svc.sh install
sudo ./svc.sh start
sudo ./svc.sh status
```

### Prerequisites on Raspberry Pi:

```bash
sudo apt-get update
sudo apt-get install -y build-essential git
```

---

## Workflow Status

You can view workflow runs at:
`https://github.com/{your-username}/inky_c/actions`

Green checkmarks ✅ indicate successful builds.
