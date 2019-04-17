/**
 * 屏幕录制
 * 
*/

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{

    FILE *pFile;
    char szFilename[32];
    int y;

    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");

    if (pFile == NULL)
    {
        return;
    }

    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    for (y = 0; y < height; y++)
    {
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
    }
    fclose(pFile);
}

int main(int argc, char const *argv[])
{
    AVFormatContext *pFormatCtx;
    int i, videoStream;

    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    AVPacket packet;
    int frameFinished;
    int numBytes;
    uint8_t *buffer;

    if (argc < 2)
    {
        printf("please provide a movie file\n");
        return -1;
    }

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        printf("couldn't open file \n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        fprintf(stderr, "avformat_find_stream_info \n");
        return -1;
    }

    av_dump_format(pFormatCtx, -1, argv[1], 0);

    videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1)
    {
        fprintf(stderr, "videoStream not found \n");
        return -1;
    }

    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL)
    {
        fprintf(stderr, "unsupported codec \n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        perror("avcodec_open2 failed");
        return -1;
    }

    pFrame = av_frame_alloc();

    if (pFrame == NULL)
    {
        perror("av_frame_alloc failed");
        return -1;
    }

    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
    {
        return -1;
    }

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    i = 0;

    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if (packet.stream_index == videoStream)
        {
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if (frameFinished)
            {
                struct SwsContext *img_convert_ctx = NULL;
                img_convert_ctx = sws_getCachedContext(img_convert_ctx, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
                if (!img_convert_ctx)
                {
                    perror("Cannot initialize sws conversion context\n");
                    exit(1);
                }

                sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,i++);
            }
        }
        av_free_packet(&packet);
    }

    av_free(buffer);
    av_free(pFrameRGB);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}
