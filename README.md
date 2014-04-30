# FeOS v2.0-prerelease
[![Build Status](https://travis-ci.org/fincs/FeOS-v2.svg)](https://travis-ci.org/fincs/FeOS-v2)

## Introduction

FeOS is a research operating system targeting ARMv6-based computers.

## Build prerequisites

You need the following in order to build FeOS/High:

- devkitARM r42 or higher
- A working GCC (including G++) 4.8.0+ installation for the host (Windows users: use TDM-GCC)

Before building, you must set the `FEOSSDK` environment variable to point to the `/sdk` directory (if on Windows, you **must** use Unix-style paths, like `/c/Users/.../gitrepos/FeOS/sdk`).
