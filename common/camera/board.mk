# SPDX-License-Identifier: Apache-2.0
#
# GloDroid project (https://github.com/GloDroid)
#
# Copyright (C) 2022 Roman Stratiienko (r.stratiienko@gmail.com)

BCC_PATH := $(patsubst $(CURDIR)/%,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BOARD_BUILD_AOSPEXT_LIBCAMERA := true
BOARD_LIBCAMERA_SRC_DIR := glodroid/vendor/libcamera
BOARD_LIBCAMERA_PIPELINES ?= simple
BOARD_LIBCAMERA_EXTRA_MESON_ARGS := -Dandroid=enabled
BOARD_LIBCAMERA_EXTRA_TARGETS := lib:libcamera-hal.so:hw:camera.libcamera:

DEVICE_MANIFEST_FILE += $(BCC_PATH)/android.hardware.camera.provider@2.5.xml

BOARD_VENDOR_SEPOLICY_DIRS       += $(BCC_PATH)/sepolicy/vendor
