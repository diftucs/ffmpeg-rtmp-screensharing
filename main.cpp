/*
	Copyright © 2022 diftucs

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <https://www.gnu.org/licenses/>
*/

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
}

int main()
{
	// Initialize devices, here used for x11grab
	avdevice_register_all();

	// Init input AVFormatContext. Will contain the input AVStream
	AVFormatContext *inputFormatContext = avformat_alloc_context();
	// Init input AVStream. Will have AVInputFormat x11grab and provide screen data
	avformat_open_input(&inputFormatContext, getenv("DISPLAY"), av_find_input_format("x11grab"), NULL);

	// Init an AVCodecContext with the codec needed to decode the input AVStream
	AVCodecContext *inputCodecContext = avcodec_alloc_context3(avcodec_find_decoder(inputFormatContext->streams[0]->codecpar->codec_id));
	// Apply the input AVStream's parameters to the codec
	avcodec_parameters_to_context(inputCodecContext, inputFormatContext->streams[0]->codecpar);
	// Initialize the input codec
	avcodec_open2(inputCodecContext, NULL, NULL);

	// Init the output AVFormatContext. Will contain the output AVStream
	AVFormatContext *outputFormatContext;
	avformat_alloc_output_context2(&outputFormatContext, NULL, "flv", NULL);
	// Init an AVCodecContext with the codec needed to encode the processed frames
	//
	// Note:
	// The default codec when initializing an AVFormatContext with an FLV container like above is AV_CODEC_ID_FLV1 (see AVFormatContext->oformat->video_codec).
	// That is the outdated H.263 codec, which can be replaced by H.264 by explicitly specifying AV_CODEC_ID_H264 when creating the AVCodecContext.
	// This will result in keeping the FLV container, but having H.264 encoded data within instead.
	AVCodecContext *outputCodecContext = avcodec_alloc_context3(avcodec_find_encoder(AV_CODEC_ID_H264));

	// Set the desired properties of the encoded packets
	outputCodecContext->bit_rate = 400000;
	outputCodecContext->width = 1920;
	outputCodecContext->height = 1080;
	outputCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	outputCodecContext->time_base = (AVRational){1, 60};

	// Init the output AVStream
	AVStream *outputStream = avformat_new_stream(outputFormatContext, NULL);
	// Apply the output AVCodecContext's codec and packet properties to the output AVStream
	avcodec_parameters_from_context(outputStream->codecpar, outputCodecContext);

	// Additionally, set properties unique to the encoded data
	// A minimum of 12 frames between each full frame (the rest only describe change)
	outputCodecContext->gop_size = 12;
	// Make a timestamp increment of one correspond to 1/60 of a second
	// During the encoding phase the timestamps are set to increase by one every frame
	// This results in a 60 FPS stream
	outputCodecContext->time_base = (AVRational){1, 60};

	// Open the output codec
	avcodec_open2(outputCodecContext, NULL, NULL);

	// Open the output stream
	avio_open(&outputFormatContext->pb, "rtmp://localhost/publishlive/livestream", AVIO_FLAG_WRITE);

	// Disable FLV-specific duration and filesize writing to header when calling av_write_trailer()
	// This should be set for streams since writing to a header that is already sent is impossible
	// Removing this results in a warning from FFmpeg
	AVDictionary *outputHeaderDictionary = NULL;
	av_dict_set(&outputHeaderDictionary, "flvflags", "no_duration_filesize", 0);

	// Write output header
	avformat_write_header(outputFormatContext, &outputHeaderDictionary);

	// Create scaling context
	SwsContext *sws_ctx = sws_getContext(inputFormatContext->streams[0]->codecpar->width,
										 inputFormatContext->streams[0]->codecpar->height,
										 static_cast<AVPixelFormat>(inputFormatContext->streams[0]->codecpar->format),
										 outputFormatContext->streams[0]->codecpar->width,
										 outputFormatContext->streams[0]->codecpar->height,
										 static_cast<AVPixelFormat>(outputFormatContext->streams[0]->codecpar->format),
										 SWS_BICUBIC, NULL, NULL, NULL);

	// Allocate memory for encoded pre- and post-converted packets
	AVPacket *inputPacket = av_packet_alloc();
	AVPacket *outPacket = av_packet_alloc();

	// Allocate memory for decoded pre- and post-scaling frames
	AVFrame *inputFrame = av_frame_alloc();
	AVFrame *outputFrame = av_frame_alloc();

	// Init the output frame buffer according to the codec's parameters on pixel format, width and height
	// The input frame's buffer is initialized by avcodec_receive_frame()
	outputFrame->format = outputCodecContext->pix_fmt;
	outputFrame->width = outputCodecContext->width;
	outputFrame->height = outputCodecContext->height;
	av_frame_get_buffer(outputFrame, 0);

	// Determine distance between each presentation timestamp
	// No idea why this is the correct scale
	int ptsScale = 2 * av_q2d(outputCodecContext->time_base) / av_q2d(outputFormatContext->streams[0]->time_base);
	for (int i = 0; i < 1000; i++)
	{
		// Get packet from input
		av_read_frame(inputFormatContext, inputPacket);

		// Send packet to decoder
		avcodec_send_packet(inputCodecContext, inputPacket);
		// Fetch frame from decoder
		avcodec_receive_frame(inputCodecContext, inputFrame);

		// Scale to output dimensions
		sws_scale(sws_ctx, inputFrame->data, inputFrame->linesize, 0, inputFormatContext->streams[0]->codecpar->height, outputFrame->data, outputFrame->linesize);

		// Write presentation timestamp
		outputFrame->pts = i * ptsScale;

		// Encode frame into packet
		avcodec_send_frame(outputCodecContext, outputFrame);
		if (avcodec_receive_packet(outputCodecContext, outPacket) < 0)
			continue;

		// Write packet to output
		av_interleaved_write_frame(outputFormatContext, outPacket);

		// Free the packet buffers since they are not reused
		av_packet_unref(inputPacket);
		av_packet_unref(outPacket);
	}

	// Write end of stream
	av_write_trailer(outputFormatContext);

	// Free memory
	sws_freeContext(sws_ctx);
	av_packet_free(&inputPacket);
	av_packet_free(&outPacket);
	av_frame_free(&inputFrame);
	av_frame_free(&outputFrame);
	avcodec_free_context(&inputCodecContext);
	avcodec_free_context(&outputCodecContext);
	avformat_close_input(&outputFormatContext);
	avformat_free_context(outputFormatContext);
	avformat_close_input(&inputFormatContext);
	avformat_free_context(inputFormatContext);

	return 0;
}
