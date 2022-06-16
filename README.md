# Screensharing over RTMP with FFmpeg libraries

## Running

1. Clone repo with submodules: `git clone --recursive https://github.com/diftucs/ffmpeg-rtmp-screensharing`.
2. `cd ffmpeg-rtmp-screensharing/`.
3. Compile FFmpeg and this project: `./compile.sh`.
4. Run with local libraries: `LD_LIBRARY_PATH="ffmpeg/libavfilter:ffmpeg/libavcodec:ffmpeg/libavdevice:ffmpeg/libavformat:ffmpeg/libavutil:ffmpeg/libswresample:/ffmpeg/libswscale:$LD_LIBRARY_PATH" ./main`.

## Important notes

- It is written to work with FFmpeg version 5.0
- The stream is pushed to `rtmp://localhost/publishlive/livestream`
- The stream is in an FLV container and encoded with H.264
- The code can with little effort be converted to saving the recording instead
- No audio is saved
- The recording part probably only works on Linux
