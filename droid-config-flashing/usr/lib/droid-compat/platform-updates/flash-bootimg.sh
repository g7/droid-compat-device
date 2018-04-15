#!/bin/sh

# These placeholders are changed during package build
CMDLINE=@CMDLINE@
DTB_PATH=@DTB_PATH@

if [ -n "$CMDLINE" ]; then
	CMDLINE_SWITCH="--cmdline \"$CMDLINE\""
fi

if [ -n "$DTB_PATH" ]; then
	DTB_SWITCH="--remove-original-dtbs --dtb \"$DTB_PATH\""
fi

eval /usr/bin/yabit \
	--input /boot/hybris-boot.img \
	$CMDLINE_SWITCH \
	$DTB_SWITCH \
	--output /tmp/hybris-boot-patched.img

/usr/sbin/flash-partition boot /tmp/hybris-boot-patched.img
