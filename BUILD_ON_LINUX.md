# Building SC944D Firmware on Linux (Docker)

This guide explains how to set up a Docker-based build environment for the SC944D firmware.

## Prerequisites

1.  **Docker**: Ensure Docker is installed and running (`docker --version`).
2.  **MQX 4.0 Source Code**: You must have the Freescale MQX 4.0 source code available locally on your Linux/Mac machine.
    - *Note: This repository does not contain the MQX source code due to licensing/size.*

## Setup

1.  **Build the Docker Image**:
    The included `build.sh` script handles this automatically.

2.  **Run the Build Environment**:
    You need to tell the script where your MQX installation is located using the `MQX_PATH` environment variable.

    ```bash
    export MQX_PATH=/path/to/your/Freescale_MQX_4_0
    chmod +x build.sh
    ./build.sh
    ```

    This will:
    - Build the `sc944d-builder` Docker image (if not present).
    - Start a container with:
        - Your project mounted at `/workspace/project`
        - Your MQX sources mounted at `/workspace/mqx`

## Porting to GCC

The original project relies on CodeWarrior specific syntax which is not compatible with standard GCC. You will need to modify the C code and create a new Makefile.

### 1. Header Files and Pragmas
CodeWarrior uses `#pragma` directives that GCC may not understand (or handles differently via `__attribute__`).
- **Interrupts**: CodeWarrior uses `__declspec(interrupt)`. GCC uses `__attribute__((interrupt_handler))`.
- **Packing**: CodeWarrior might use `#pragma options align=packed`. GCC uses `__attribute__((packed))`.

### 2. Makefile
You need to create a `Makefile` that uses `m68k-linux-gnu-gcc`.
Key flags for ColdFire V2 (MCF52259):
- `-mcpu=52259`
- `-msoft-float` (Unless using hardware FPU)
- Include paths: `-I/workspace/mqx/mqx/source/include` ...

### 3. Linker Script
You will need a GNU linker script (`.ld`) to replace the CodeWarrior linker command file (`.lcf`). This defines the memory map (Flash, RAM, Stack).
