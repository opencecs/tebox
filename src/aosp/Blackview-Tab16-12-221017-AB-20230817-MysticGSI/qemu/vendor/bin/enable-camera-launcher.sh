#!/system/bin/sh

# The GSI first-boot flow disables the stock camera launcher alias. Keep it
# available on this QEMU profile after PackageManager has finished booting.
sleep 20
/system/bin/pm enable --user 0 com.android.camera2/com.android.camera.CameraLauncher
