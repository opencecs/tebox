import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTimestamp;
import android.media.AudioTrack;

/** End-to-end playback through AudioFlinger and the HAL's FMQs. */
public final class AudioSmoke {
    public static void main(String[] args) throws Exception {
        AudioFormat format = new AudioFormat.Builder().setSampleRate(48000)
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build();
        int bufferSize = Math.max(3840, AudioTrack.getMinBufferSize(48000,
                AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT));
        AudioTrack track = new AudioTrack.Builder().setAudioFormat(format)
                .setAudioAttributes(new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                .setTransferMode(AudioTrack.MODE_STREAM).setBufferSizeInBytes(bufferSize).build();
        try {
            if (track.getState() != AudioTrack.STATE_INITIALIZED) throw new AssertionError("init");
            short[] pcm = new short[960]; // 480 stereo frames, 10 ms
            for (int i = 0; i < 480; ++i) {
                pcm[2*i] = pcm[2*i+1] = (short)(Math.sin(i * 2 * Math.PI * 440 / 48000) * 1000);
            }
            track.play();
            long start = System.nanoTime();
            for (int i = 0; i < 150; ++i) {
                int written = track.write(pcm, 0, pcm.length, AudioTrack.WRITE_BLOCKING);
                if (written != pcm.length) throw new AssertionError("write=" + written);
            }
            Thread.sleep(150);
            long elapsedMs = (System.nanoTime() - start) / 1000000;
            long position = Integer.toUnsignedLong(track.getPlaybackHeadPosition());
            if (position < 24000 || elapsedMs < 500) {
                throw new AssertionError("clock position=" + position + " elapsedMs=" + elapsedMs);
            }
            AudioTimestamp timestamp = new AudioTimestamp();
            boolean hasTimestamp = track.getTimestamp(timestamp);
            track.pause();
            track.flush();
            track.play();
            if (track.write(pcm, 0, pcm.length) != pcm.length) throw new AssertionError("resume");
            track.stop();
            System.out.println("AUDIO_SMOKE_PASS position=" + position + " elapsedMs=" + elapsedMs
                    + " timestamp=" + hasTimestamp + " timestampFrames=" + timestamp.framePosition);
        } finally {
            track.release();
        }
    }
}
