# Screensharing over RTMP with FFmpeg libraries

Compile with `./compile.sh` and run with `./main`.

## Important notes

- It is written to work with FFmpeg version 5.0
- The stream is pushed to `rtmp://localhost/publishlive/livestream`
- The stream is in an FLV container and encoded with H.263
- The code can with little effort be converted to saving the recording instead
- No audio is saved
- The recording part probably only works on Linux
