(
    echo "Building FFmpeg" && \
    cd ffmpeg/ && \
    ./configure --enable-debug --enable-gpl --enable-libx264 --enable-shared && \
    make
)

echo
echo "Building main.cpp"
g++ main.cpp -o main -g -Iffmpeg/libavfilter -Iffmpeg/libavcodec -Iffmpeg/libavdevice -Iffmpeg/libavformat -Iffmpeg/libavutil -Iffmpeg/libswresample -Iffmpeg/libswscale -Lffmpeg/libavfilter -Lffmpeg/libavcodec -Lffmpeg/libavdevice -Lffmpeg/libavformat -Lffmpeg/libavutil -Lffmpeg/libswresample -Lffmpeg/libswscale -lavfilter -lavcodec -lavdevice -lavformat -lavutil -lswresample -lswscale
