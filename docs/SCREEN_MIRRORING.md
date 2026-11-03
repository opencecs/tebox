# 查看设备画面

## 本机 MJPEG 投屏

定制系统的 MediaCodec 屏幕编码路径仍会报 `IllegalStateException`。项目内包含一对固定版本的 scrcpy 客户端和服务端，使用不依赖 MediaCodec 的 MJPEG 路径：

```sh
scripts/mirror-device.sh 127.0.0.1:5570
```

需要 macOS ARM64 和 ADB。默认客户端还需要 Homebrew 的 `ffmpeg`、`sdl3`、`libusb`；若存在 `out/scrcpy-mjpeg-bundle/scrcpy`，脚本会优先使用该目录里的自带动态库版本。启动脚本会先确认指定设备在线。默认启用 UHID 鼠标和键盘；只看画面可加 `--view-only`。5568 上已实际验证点击和滑动。不要使用 scrcpy 默认的 SDK 注入模式：该模式在当前 guest 会遇到 `INJECT_EVENTS` 权限错误。

5570 上实测 1080×2400 原始画面可以打开。默认投屏会缩至 540×1200，JPEG 质量 50、帧率上限 60；5568 的动态画面实测约 45–56 fps，静止画面会降到 0 fps。该设置不改变设备自身分辨率。此版本不提供 MJPEG 录制。客户端和服务端的来源、版本及许可证见 `prebuilts/host/darwin-arm64/scrcpy-mjpeg/SOURCE.md`。

远程设备可在当前 Mac 上直接打开 scrcpy 窗口，脚本会临时建立 SSH 隧道并在退出时关闭：

```sh
scripts/mirror-remote-device.sh USER@REMOTE_MAC 127.0.0.1:5568
scripts/mirror-remote-device.sh USER@REMOTE_MAC 127.0.0.1:5569
```

SSH 会在终端询问密码；项目不保存密码。远程 Mac 需要已运行 ADB server，且两台设备在其 ADB 列表中在线。

## 远程 Mac 屏幕共享

5568、5569 在远程 Mac 上运行时，可用 macOS「屏幕共享」查看该 Mac 的 QEMU 窗口：

```sh
bash scripts/open-remote-display.sh USER@HOST
```

在系统登录界面输入远程 Mac 的账号，再选择对应的 QEMU 窗口。脚本只检查 5900 端口并打开系统屏幕共享，不保存密码，也不改变虚拟机镜像。只检查连通性可加 `--check-only`。
