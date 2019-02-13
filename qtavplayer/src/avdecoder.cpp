#include "avdecoder.h"
#include <QDebug>

#include <QDateTime>

//AVFormatContext *pFormatCtx;
//AVCodecContext *pCodecCtx;
//AVCodec *pCodec;
//AVFrame *pFrame, *pFrameRGB;
//AVPacket *packet;
//uint8_t *out_buffer;
//AVPicture  pAVPicture;
//AVDictionary* options = NULL;

//static struct SwsContext *img_convert_ctx;

//int videoStream, i, numBytes;
//int ret, got_picture;

static FILE *output_file = NULL;

AVPixelFormat AVDecoder::get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
{

    AVDecoder *opaque = (AVDecoder *) ctx->opaque;
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == opaque->mHWPixFormat)
        {
            qDebug() << (*p) << ";" << opaque->mHWPixFormat;
            qDebug() << "Successed to get HW surface format.";
            return *p;
        }
    }
    qDebug() << "Failed to get HW surface format.";
    return AV_PIX_FMT_NONE;
}

void AVCodecTask::run(){
    //    qDebug() << "-------------------- yuanlei command : " << command;
    switch(command){
    case AVCodecTaskCommand_Init:
        mCodec->init();
        break;
    case AVCodecTaskCommand_SetPlayRate: //播放速率
        break;
    case AVCodecTaskCommand_Seek:        //跳转
        break;
    case AVCodecTaskCommand_Decodec:     //获取数据  + 渲染成帧
        mCodec->decodec();
        break;
    case AVCodecTaskCommand_SetFileName: //设置播放对象 -- url
        mCodec->setFilenameImpl(param2);
        break;
    case AVCodecTaskCommand_DecodeToRender : //+ 渲染成帧
        break;
    case AVCodecTaskCommand_SetDecodecMode :
        break;

    case AVCodecTaskCommand_ShowFrameByPosition :   //按位置显示帧
        break;
    }
    //    qDebug() << "-------------------- yuanlei end command : " << command;
}



AVDecoder::AVDecoder(QObject *parent) : QObject(parent)
{
    av_log_set_callback(NULL);//不打印日志
    av_register_all();
    //    av_lockmgr_register(lockmgr);
    initRenderList();

    //    setFilenameImpl("udp://@227.70.80.90:2000");
}



void AVDecoder::load(){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Init));
}

void AVDecoder::setFilename(const QString &source){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_SetFileName,0,source));
}

/*
static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
    enum AVPixelFormat fmt;

    switch (type) {
    case AV_HWDEVICE_TYPE_VAAPI:
        fmt = AV_PIX_FMT_VAAPI;
        break;
    case AV_HWDEVICE_TYPE_DXVA2:
        fmt = AV_PIX_FMT_DXVA2_VLD;
        break;
    case AV_HWDEVICE_TYPE_D3D11VA:
        fmt = AV_PIX_FMT_D3D11;
        break;
    case AV_HWDEVICE_TYPE_VDPAU:
        fmt = AV_PIX_FMT_VDPAU;
        break;
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        break;
    default:
//        SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "find_fmt_by_hw_type with invalid type:%d", type);
        fmt = AV_PIX_FMT_NONE;
        break;
    }

    return fmt;
}
int mf_hw_decode_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&m_hwDeviceCtx, type, NULL, NULL, 0)) < 0)
    {
//        SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Failed to create specified HW device(%d)", type);
        print_ffmpeg_error(err);
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);

    return err;
}

int avcodec_decode_video_hw(AVCodecContext *avctx, AVFrame *sw_frame, int *got_picture_ptr, const AVPacket *packet)
{
    AVFrame *frame = NULL;
    int ret = 0;

    *got_picture_ptr = 0;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0)
    {
//        SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error during decoding ret:%d", ret);
        print_ffmpeg_error(ret);
        return ret;
    }

    while (ret >= 0)
    {
        if (!(frame = av_frame_alloc()))
        {
//            SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Can not alloc frame");
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            av_frame_free(&frame);
            return 0;
        }
        else if (ret < 0)
        {
//            SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error while decoding ret:%d", ret);
            print_ffmpeg_error(ret);
            goto fail;
        }

        if (frame->format == m_hwPixFmt)
        {
            //从显存拷贝到内存
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0)
            {
//                SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error transferring the data to system memory, ret:%d", ret);
                print_ffmpeg_error(ret);
                goto fail;
            }
            *got_picture_ptr = 1;
        }

fail:
        av_frame_free(&frame);
        if (ret < 0)
            return ret;
    }

    return 0;
}
*/




static int hw_decoder_init(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, const enum AVHWDeviceType type)
{
    int err = 0;
    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    return err;
}

void AVDecoder::init(){
    if(mIsInit)
        return;
    mIsInit = true;
    //     statusChanged(AVDefine::AVMediaStatus_Loading);/**/


    static QStringList codecs;
//    if (!codecs.isEmpty())
//        return codecs;
    avcodec_register_all();
    AVCodec* c = NULL;
    while ((c=av_codec_next(c))) {
        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
            continue;
        codecs.append(QString::fromLatin1(c->name));
    }
    qDebug() << "-----------------------codecs.size" << codecs.size() << codecs;
//    return codecs;

//av_q2d


    AVDictionary* options = NULL;
    av_dict_set(&options, "buffer_size", "1024000", 0);  //增大“buffer_size”
//    av_dict_set(&options, "max_delay", "500000", 0);
//    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
//    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp



    //寻找视频
//    mFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&mFormatCtx, mFilename.toStdString().c_str(), NULL, &options) != 0) //打开视频
    {
        qDebug() << "media open error : " << mFilename.toStdString().data();
        //         statusChanged(AVDefine::AVMediaStatus_NoMedia);
        mIsInit = false;
        return;
    }
    mIsInit = false;

    //     qDebug()<<"file name is ==="<< QString(mFormatCtx->filename);
    //     qDebug()<<"the length is ==="<<mFormatCtx->duration/1000000;

    if(avformat_find_stream_info(mFormatCtx, NULL) < 0) //判断是否找到视频流信息
    {
        qDebug() << "media find stream error : " << mFilename.toStdString().data();
        //         statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }

    /* 寻找视频流 */
    int ret = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mVideoCodec, 0); //查找视频流信息

    if (ret < 0) {
        qDebug() << "Cannot find a video stream in the input file";
    }else{
        mVideoIndex = ret;
        //duration
        //        int64_t duration = mFormatCtx->duration / AV_TIME_BASE;
        //fps
        int fps_num = mFormatCtx->streams[mVideoIndex]->r_frame_rate.num;
        int fps_den = mFormatCtx->streams[mVideoIndex]->r_frame_rate.den;
        double fps = 0.0;
        if (fps_den > 0) {
            fps = fps_num / fps_den;
        }
        qDebug() << "---------------------------fps  mVideoCodec->codec_id"  << fps << mVideoCodec->id;
        //--------------------AV_CODEC_ID_H264


        //选择硬解

        mHWConfigList.clear();
        for (int i = 0;; i++) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(mVideoCodec, i);
            if (!config) {
                break;
            }
            if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) && config->device_type != AV_HWDEVICE_TYPE_NONE) {
                mHWConfigList.push_back((AVCodecHWConfig *)config);
                qDebug() << "------------------------config->device_type" << config->device_type << config->pix_fmt;
                continue;
            }
        }

            if (!(mVideoCodecCtx = avcodec_alloc_context3(mVideoCodec)))
                return; // AVERROR(ENOMEM);
            mVideo = mFormatCtx->streams[mVideoIndex];
//        mVideoCodecCtx = avcodec_alloc_context3(mVideoCodec);
        if (!mVideoCodecCtx){
            qDebug() << "create video context fail!" << mFormatCtx->filename;
            //            qDebug() << "avformat_open_input 3";
            //            statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        }else{

            if (avcodec_parameters_to_context(mVideoCodecCtx, mFormatCtx->streams[mVideoIndex]->codecpar) < 0)
                return;


//            decoder_ctx->get_format  = get_hw_format;
          //    if (hw_decoder_init(decoder_ctx, type) < 0)
          //        return -1;
          //    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
          //        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
          //        return -1;
          //    }

            if(mHWConfigList.size() > 0){
                if(mVideoCodecCtx != NULL){
                    if(avcodec_is_open(mVideoCodecCtx))
                        avcodec_close(mVideoCodecCtx);
                }
                mVideoCodecCtx->thread_count = 0;
                mVideoCodecCtx->opaque = (void *) this;

                for(int i = 0; i < mHWConfigList.size(); i++){
                    const AVCodecHWConfig *config = mHWConfigList.at(mHWConfigList.size() - 1- i); //mHWConfigList.front();

                    mHWPixFormat = config->pix_fmt;
                    mVideoCodecCtx->get_format = get_hw_format; //回调 &mHWDeviceCtx
                    if ( hw_decoder_init(mVideoCodecCtx, mHWDeviceCtx,config->device_type) < 0    /*av_hwdevice_ctx_create(&mHWDeviceCtx, config->device_type,NULL, NULL, 0) < 0*/) {
                        mUseHw = false;
                    }else{
    /*
    //                    硬解码
                        avcodec_decode_video_hw(m_ctx, m_picture, &got_picture, &avpkt);
    */

                        mUseHw = true;
//                        mVideoCodecCtx->hw_device_ctx = av_buffer_ref(mHWDeviceCtx); //创建对avbuffer的新引用

                        //                        mHWFrame = av_frame_alloc(); //分配空间
                        //                        changeRenderItemSize(mVideoCodecCtx->width,mVideoCodecCtx->height,(AVPixelFormat)mHWPixFormat); //更改呈现项大小
                    }
                if( mUseHw && avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0){
                    mIsOpenVideoCodec = true;
                    break;
                }else
                    qDebug() << "-----------------------config->device_type NG" << config->device_type;



                }
            }else{
                mUseHw = false;
            }





            clearRenderList();
            //            mVideoCodecCtx->thread_count = 8; //线程数
//            mIsOpenVideoCodec = avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0;
            mFrame = av_frame_alloc();
            mPacket = av_packet_alloc(); // new 空间
            mIsInit = true;




            //    /* open the file to dump raw data */
                output_file = fopen("E:/vedio.ts", "w+");



            //            statusChanged(AVDefine::AVMediaStatus_Buffering);
            mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));




//            //            mdecodecMode = AVDefine::AVDecodeMode_HW;
//            if(mHWConfigList.size() > 0){
//                if(mVideoCodecCtx != NULL){
//                    if(avcodec_is_open(mVideoCodecCtx))
//                        avcodec_close(mVideoCodecCtx);
//                }
//                mVideoCodecCtx->thread_count = 0;
//                mVideoCodecCtx->opaque = (void *) this;

//                for(int i = 0; i < mHWConfigList.size(); i++){
//                    const AVCodecHWConfig *config = mHWConfigList.at(mHWConfigList.size() - 1- i); //mHWConfigList.front();

//                    mHWPixFormat = config->pix_fmt;

//                    if (av_hwdevice_ctx_create(&mHWDeviceCtx, config->device_type,NULL, NULL, 0) < 0) {
//                        mUseHw = false;
//                    }else{
//    /*
//    //                    硬解码
//                        avcodec_decode_video_hw(m_ctx, m_picture, &got_picture, &avpkt);
//    */

//                        mUseHw = true;
//                        mVideoCodecCtx->hw_device_ctx = av_buffer_ref(mHWDeviceCtx); //创建对avbuffer的新引用
//                        mVideoCodecCtx->get_format = get_hw_format; //回调
//                        //                        mHWFrame = av_frame_alloc(); //分配空间
//                        //                        changeRenderItemSize(mVideoCodecCtx->width,mVideoCodecCtx->height,(AVPixelFormat)mHWPixFormat); //更改呈现项大小
//                    }
//                if( mUseHw && avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0)
//                    break;
//                else
//                    qDebug() << "-----------------------config->device_type NG" << config->device_type;



//                }
//            }else{
//                mUseHw = false;
//            }










            //Windows平台使用DXVA2接口





            qDebug()<<"jie ma qi de name======    "<<mVideoCodec->name;
            qDebug("init play_video is ok!!!!!!!");
        }
    }



    //    pFormatCtx = avformat_alloc_context();
    //    if (avformat_open_input(&pFormatCtx, mFilename.toStdString().c_str(), NULL, &options) != 0) {//寻找视频
    //        printf("can't open the file.");
    //        mIsInit = false;
    //        return;
    //    }
    //    mIsInit = false;
    //    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    //        printf("Could't find stream infomation.");
    //        return;
    //    }
    //    videoStream = -1;
    //    for (i = 0; i < pFormatCtx->nb_streams; i++) {
    //        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
    //            videoStream = i;
    //        }
    //    }
    //    if (videoStream == -1) {
    //        printf("Didn't find a video stream.");
    //        return;
    //    }
    //    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    //    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //    if (pCodec == NULL) {
    //        printf("Codec not found.");
    //        return;
    //    }
    //    ///测试
    //    avpicture_alloc(&pAVPicture,AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height);
    //    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    //        printf("Could not open codec.");
    //        return;
    //    }
    //    mIsOpenVideoCodec = true;
    //    mIsInit = true;
    //    pFrame = av_frame_alloc();
    //    pFrameRGB = av_frame_alloc();
    //    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
    //                                     pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
    //                                     AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    //    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);
    //    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    //    avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
    //                   pCodecCtx->width, pCodecCtx->height);

    //    int y_size = pCodecCtx->width * pCodecCtx->height;
    //    packet = (AVPacket *) malloc(sizeof(AVPacket));
    //    av_new_packet(packet, y_size);
    //    packet->pts=10;
    //    packet->dts=10;

    //    av_dump_format(pFormatCtx, 0, mFilename.toStdString().c_str(), 0);

    //    qDebug("init play_video is ok!!!!!!!");

    //mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
}

void AVDecoder::release(bool isDeleted){
    if(mFormatCtx != NULL){
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }

    if(mVideoCodecCtx != NULL){
        avcodec_close(mVideoCodecCtx);
        avcodec_free_context(&mVideoCodecCtx);
        mVideoCodecCtx = NULL;
    }

    if(mVideoCodec != NULL){
        av_free(mVideoCodec);
        mVideoCodec = NULL;
    }
    if(mPacket != NULL){
        av_packet_free(&mPacket);
        mPacket = NULL;
    }

    if(mVideoSwsCtx != NULL){
        sws_freeContext(mVideoSwsCtx);
        mVideoSwsCtx = NULL;
    }

    if(mFrame != NULL){
        av_frame_unref(mFrame);
        av_frame_free(&mFrame);
        mFrame = nullptr;
    }

    //    mStatus = AVDefine::AVMediaStatus_UnknownStatus;

    clearRenderList(isDeleted);
}

//#include "ffmpeg_dxva2.h"
#include <d3d9.h>
static int dxva2_retrieve_data(AVCodecContext *s, AVFrame *frame)
{
//    LPDIRECT3DSURFACE9 surface = (LPDIRECT3DSURFACE9)frame->data[3];
//    InputStream        *ist = (InputStream *)s->opaque;
//    DXVA2Context       *ctx = (DXVA2Context *)ist->hwaccel_ctx;

//    EnterCriticalSection(&cs);
//    //直接渲染
//    ctx->d3d9device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
//    ctx->d3d9device->BeginScene();
//    if (m_pBackBuffer)
//    {
//        m_pBackBuffer->Release();
//        m_pBackBuffer = NULL;
//    }
//    ctx->d3d9device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
//    GetClientRect(d3dpp.hDeviceWindow, &m_rtViewport);
//    ctx->d3d9device->StretchRect(surface, NULL, m_pBackBuffer, &m_rtViewport, D3DTEXF_LINEAR);
//    ctx->d3d9device->EndScene();
//    ctx->d3d9device->Present(NULL, NULL, NULL, NULL);

//    LeaveCriticalSection(&cs);

    return 0;
}


int AVDecoder::decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL, *sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    uint8_t *buffer = NULL;
    int size;
    int ret = 0;
    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }
    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        if (frame->format == mHWPixFormat) {
            /* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                fprintf(stderr, "Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
        } else
            tmp_frame = frame;
        size = av_image_get_buffer_size((AVPixelFormat)tmp_frame->format, tmp_frame->width,
                                        tmp_frame->height, 1);
        buffer = (uint8_t *)av_malloc(size);
        if (!buffer) {
            fprintf(stderr, "Can not alloc buffer\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = av_image_copy_to_buffer(buffer, size,
                                      (const uint8_t * const *)tmp_frame->data,
                                      (const int *)tmp_frame->linesize, (AVPixelFormat)tmp_frame->format,
                                      tmp_frame->width, tmp_frame->height, 1);
        if (ret < 0) {
            fprintf(stderr, "Can not copy image to buffer\n");
            goto fail;
        }
        qDebug() << "---------------------------size" << sw_frame->format << mHWPixFormat << (AVPixelFormat)frame->format << tmp_frame->width << tmp_frame->height << size;
//        if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
//            fprintf(stderr, "Failed to dump raw data.\n");
//            goto fail;
//        }
    fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_freep(&buffer);
        if (ret < 0)
            return ret;
    }
}

//qreal getDAR(AVFrame *f)
//{
//    // lavf 54.5.100 av_guess_sample_aspect_ratio: stream.sar > frame.sar
//    qreal dar = 0;
//    if (f->height > 0)
//        dar = (qreal)f->width/(qreal)f->height;
//    // prefer sar from AVFrame if sar != 1/1
//    if (f->sample_aspect_ratio.num > 1)
//        dar *= av_q2d(f->sample_aspect_ratio);
//    else if (codec_ctx && codec_ctx->sample_aspect_ratio.num > 1) // skip 1/1
//        dar *= av_q2d(codec_ctx->sample_aspect_ratio);
//    return dar;
//}

void AVDecoder::decodec(){
    if(!mIsInit || !mIsOpenVideoCodec){ //必须初始化
        //        statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }
    av_free_packet(mPacket);
    if (av_read_frame(mFormatCtx, mPacket) < 0) //读取数据
    {
        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
        return;
    }
    if (mPacket->stream_index == mVideoIndex) {


        if(mUseHw){ //硬解
            int ret = avcodec_send_packet(mVideoCodecCtx, mPacket);


            if(ret != 0){
                mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                return;
            }else{
//                ret = decode_write(mVideoCodecCtx, mPacket);
                AVFrame *tempFrame = av_frame_alloc();
                while(avcodec_receive_frame(mVideoCodecCtx, tempFrame) == 0){

                    static uchar flag = 0;
                    if(tempFrame->pict_type == AV_PICTURE_TYPE_P && flag%2 && getRenderListSize() >= 5) {  //丢帧
                        qDebug() << "---------------------=========>>>lost frame" << tempFrame->pict_type;
                        av_frame_unref(tempFrame);
                        av_frame_free(&tempFrame);
                        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                        return;
                    }



                    if (tempFrame->format == mHWPixFormat) {

                        if ((ret = av_hwframe_transfer_data(mFrame, tempFrame, 0)) < 0){ // 从显存考到内存

                        }else{
                            av_frame_copy_props(mFrame, tempFrame);
                        }

//                         qDebug() << "--------------------------tempFrame" << mFrame->format << tempFrame->format << mHWPixFormat << QString::number( double(mFrame->pkt_pts)/1000.0, 'f', 4) <<QString::number( double(mFrame->pkt_dts)/1000.0, 'f', 4) <<QString::number( double(mFrame->pts), 'f', 4); // AV_PIX_FMT_DXVA2_VLD
                    }

                    if(mSize != QSize(mFrame->width, mFrame->height) || mVideoSwsCtx == NULL){
                        if(mVideoSwsCtx != NULL){
                            sws_freeContext(mVideoSwsCtx);
                            mVideoSwsCtx = NULL;
                        }

                        if(mSize != QSize(mFrame->width, mFrame->height) ){
                            mSize = QSize(mFrame->width, mFrame->height);
                        }
                        //软解设置
                        mVideoSwsCtx = sws_getContext(
                                    mFrame->width,
                                    mFrame->height,
                                    (AVPixelFormat)mFrame->format,
                                    960, 540,
//                                    mFrame->width,
//                                    mFrame->height,
                                    AV_PIX_FMT_RGB24,
                                    SWS_BICUBIC,NULL,NULL,NULL);

                        avpicture_alloc(&mAVPicture,AV_PIX_FMT_RGB24,/*mFrame->width,mFrame->height*/960, 540);

                    }

                    sws_scale(mVideoSwsCtx,
                              (uint8_t const * const *) mFrame->data,
                              mFrame->linesize, 0, mFrame->height,//         mVideoCodecCtx->height,
                              mAVPicture.data,
                              mAVPicture.linesize);

//                    QImage img = QImage(mAVPicture.data[0],/*mFrame->width,mFrame->height*/960, 540,QImage::Format_RGB888);
//                    if(!img.isNull())
//                        send_img(img);

                    RenderItem* item = getInvalidRenderItem();
                    if(!item) {
                        av_frame_unref(tempFrame);
                        av_frame_free(&tempFrame);
                        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                        return;
                    }

                    item->mutex.lock();
                    item->frame     =  new QImage(mAVPicture.data[0],/*mFrame->width,mFrame->height*/960, 540,QImage::Format_RGB888);

                    //            qDebug() << "-----------------" << item->valid << item->isShowing << item->pts ;
                    if(item->frame->isNull())
                    {
                        qDebug()<<"image =======    " << item->frame->isNull();
                        item->mutex.unlock();
                        av_frame_unref(tempFrame);
                        av_frame_free(&tempFrame);
                        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                        return;
                    }

                    item->pts =   mFrame->pts; // QDateTime::currentMSecsSinceEpoch();           //render->frame->pts;
                    item->valid = true;
                    //            emit send_img(img);
                    item->mutex.unlock();

                    av_frame_unref(tempFrame);
                }
                av_frame_free(&tempFrame);
            }
            qDebug() << "-------------------------34-tempFrame";
        }else{
            int ret, got_picture;
            ret = avcodec_decode_video2(mVideoCodecCtx, mFrame, &got_picture, mPacket);
            if (ret < 0) {
                printf("decode error.");
                mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                return;
            }
            RenderItem* item = getInvalidRenderItem();

            if(!item) {
                mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                return;
            }
            if (got_picture) {

                if(mSize != QSize(mFrame->width, mFrame->height) || mVideoSwsCtx == NULL){
                    if(mVideoSwsCtx != NULL){
                        sws_freeContext(mVideoSwsCtx);
                        mVideoSwsCtx = NULL;
                    }

                    if(mSize != QSize(mFrame->width, mFrame->height) ){
                        mSize = QSize(mFrame->width, mFrame->height);
                    }
                    //软解设置
                    mVideoSwsCtx = sws_getContext(
                                mFrame->width,
                                mFrame->height,
                                (AVPixelFormat)mFrame->format,
                                960, 540,
                                //                            mFrame->width,
                                //                            mFrame->height,
                                AV_PIX_FMT_RGB24,
                                SWS_BICUBIC,NULL,NULL,NULL);

                    avpicture_alloc(&mAVPicture,AV_PIX_FMT_RGB24,/*mFrame->width,mFrame->height*/960, 540);

                }

                sws_scale(mVideoSwsCtx,
                          (uint8_t const * const *) mFrame->data,
                          mFrame->linesize, 0, mFrame->height,//         mVideoCodecCtx->height,
                          mAVPicture.data,
                          mAVPicture.linesize);

                item->mutex.lock();
                item->frame     =  new QImage(mAVPicture.data[0],/*mFrame->width,mFrame->height*/960, 540,QImage::Format_RGB888);

                //            qDebug() << "-----------------" << item->valid << item->isShowing << item->pts ;
                if(item->frame->isNull())
                {
                    qDebug()<<"image =======    " << item->frame->isNull();
                    item->mutex.unlock();
                    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
                    return;
                    //                break;
                }

                item->pts =  QDateTime::currentMSecsSinceEpoch();           //render->frame->pts;
                item->valid = true;
                //            emit send_img(img);
                item->mutex.unlock();
            }
        }

    }
    //    av_free_packet(mPacket);
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));

}

void AVDecoder::setFilenameImpl(const QString &source){
    mProcessThread.clearAllTask(); //清除所有任务
    if(mFilename.size() > 0){
        release();//释放所有资源
    }
    mFilename = source;
    mIsOpenVideoCodec = false;
    mIsInit = false;
    load();
}

qint64 AVDecoder::getNextFrameTime()
{
    qint64 minTime = 0;
    RenderItem *render = NULL;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lock();
        if(item->valid){
            if(minTime == 0){
                minTime = item->pts;
            }

            if(item->pts <= minTime){
                minTime = item->pts;
                render = item;
            }
        }
        item->mutex.unlock();
    }
    mRenderListMutex.unlock();
    return minTime;
}

qint64 AVDecoder::requestRenderNextFrame(){
    qint64 time = 0;
    if(getRenderListSize() > 0){
        RenderItem *render = getMinRenderItem();
        //        qDebug() << "------------- "<< getRenderListSize() << render;
        if(render != nullptr){
            render->mutex.lock();
            render->isShowing = true;
            emit send_img(*render->frame);
            time = render->pts;
            //                        qDebug() << "--------------------------------img"  << render->frame->size();
            //                        static uchar flag1 = 0;
            //                        /*vFrame.*/render->frame->save(QString("C:/img%1.jpg").arg(flag1++%11));
            render->clear();
            render->mutex.unlock();
        }
    }
    return time;
}

//----------------------------------------------------------------------------
//---------------队列操作
void AVDecoder::initRenderList()
{
    mRenderListMutex.lockForWrite();
    if(mRenderList.size() < maxRenderListSize){
        for(int i = mRenderList.size();i < maxRenderListSize;i++){
            mRenderList.push_back(new RenderItem);
        }
    }
    mRenderListMutex.unlock();
}

int AVDecoder::getRenderListSize()
{
    int ret = 0;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lock();
        if(item->valid)
            ++ret;
        item->mutex.unlock();
    }
    mRenderListMutex.unlock();
    return ret;
}

void AVDecoder::clearRenderList(bool isDelete)
{
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lock();
        if(item->valid)
        {
            //            mVideoSwsCtxMutex.lock();
            mRenderList[i]->clear();
            //            mVideoSwsCtxMutex.unlock();
        }
        item->mutex.unlock();
        if(isDelete){
            delete item;
        }
    }
    if(isDelete){
        mRenderList.clear();
    }
    mRenderListMutex.unlock();
}

RenderItem *AVDecoder::getInvalidRenderItem()
{
    RenderItem *item = NULL;
    mRenderListMutex.lockForRead();
    static int i = 0;
    int len = mRenderList.size();
    if(i >= len) i = 0;

    for( ;i < len;i++){
        RenderItem *item2 = mRenderList[i];
        if(item2->isShowing)
            continue;
        item2->mutex.lock();
        if(!item2->valid){
            item = item2;
            item2->mutex.unlock();
            break;
        }
        item2->mutex.unlock();
    }
    //    if(item){
    //        item->mutex.lock();
    //        item->clear();
    //        item->mutex.unlock();
    //    }

    mRenderListMutex.unlock();
    return item;
}

RenderItem *AVDecoder::getMinRenderItem()
{
    qint64 minTime = 0;
    RenderItem *render = NULL;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lock();
        if(item->valid){
            if(minTime == 0){
                minTime = item->pts;
            }

            if(item->pts <= minTime){
                minTime = item->pts;
                render = item;
            }
        }
        if((i == len -1) && render)
            render->isShowing = true;
        item->mutex.unlock();
    }
    mRenderListMutex.unlock();
    return render;
}
