# scrcpy MJPEG fallback build

These two binaries are a matching client and Android server built from
Genymobile/scrcpy commit `22b1c4887ce851afeaa5749a3bb35a14bc993794`
(upstream pull request 6961, version 4.1). The source is available at
https://github.com/Genymobile/scrcpy/pull/6961 and is licensed under Apache 2.0;
see `LICENSE` in this directory.

Build host: macOS ARM64, Meson/Ninja, Android SDK, Java 17, Gradle wrapper,
Homebrew FFmpeg 9, SDL3 and libusb. The client dynamically loads Homebrew
libraries from `/opt/homebrew/opt/{ffmpeg,sdl3,libusb}`. The Gradle dependency
repositories were supplemented with Aliyun Google and Central mirrors for the
build; no application source files were changed.

SHA-256 of the bundled files:

```text
scrcpy         b92519c5d4072a48a5b958acc9c34417fea54c36a090328b231ea12ea4b564a6
scrcpy-server  9812bb2607b718ec85fbf722cc8da0ed83140519846610a5777c7653ed1e13f6
```

Rebuild from the pinned source:

```sh
git clone https://github.com/Genymobile/scrcpy.git out/scrcpy-mjpeg-src
git -C out/scrcpy-mjpeg-src fetch origin pull/6961/head
git -C out/scrcpy-mjpeg-src checkout 22b1c4887ce851afeaa5749a3bb35a14bc993794
cd out/scrcpy-mjpeg-src
git apply --unidiff-zero ../../integrations/scrcpy-mjpeg-frame-pacing.patch
meson setup x --buildtype=release --strip -Db_lto=true
ninja -C x
```

If Maven Central is blocked, insert these two entries before `google()` in
both `repositories` blocks of `build.gradle`, then rerun `ninja -C x`:

```gradle
maven { url 'https://maven.aliyun.com/repository/google' }
maven { url 'https://maven.aliyun.com/repository/central' }
```

The resulting files are `x/app/scrcpy` and `x/server/scrcpy-server`. Keep them
as a pair. The launcher at `scripts/mirror-device.sh` selects MJPEG because the
custom guest's MediaCodec screen encoder currently fails during initialization.
The local patch changes frame pacing so JPEG encoding time counts toward the
60 fps interval; it does not modify scrcpy's input protocol.

For a Mac with different Homebrew library versions, create a local runtime
bundle on the build host and copy it to the target project's ignored `out/`:

```sh
python3 scripts/bundle-scrcpy-mjpeg.py out/scrcpy-mjpeg-src/x/app/scrcpy out/scrcpy-mjpeg-bundle
```

The launcher prefers `out/scrcpy-mjpeg-bundle/scrcpy` when present. The runtime
bundle contains third-party libraries and is kept outside the project's source
archive; review their licenses before redistributing it.
