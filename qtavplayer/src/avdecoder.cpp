#include "avdecoder.h"
#include <QDebug>

#include <QDateTime>
#include <QTimer>

//输出的地址
const char *outUrl =  "rtmp://192.168.5.138:1936/live/test";
AVFormatContext *outputContext = nullptr;
AVFormatContext *inputContext = nullptr;

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

static int lockmgr(void **mtx, enum AVLockOp op)
{
   switch(op) {
      case AV_LOCK_CREATE:{
           QMutex *mutex = new QMutex;
           *mtx = mutex;
           return 0;
      }
      case AV_LOCK_OBTAIN:{
           QMutex *mutex = (QMutex*)*mtx;
           mutex->lock();
           return 0;
       }
      case AV_LOCK_RELEASE:{
           QMutex *mutex = (QMutex*)*mtx;
           mutex->unlock();
           return 0;
      }
      case AV_LOCK_DESTROY:{
           QMutex *mutex = (QMutex*)*mtx;
           delete mutex;
           return 0;
      }
   }
   return 1;
}

void AVCodecTask::run(){
    switch(command){
    case AVCodecTaskCommand_Init:
        mCodec->init();
        break;
    case AVCodecTaskCommand_SetPlayRate: //播放速率
        break;
    case AVCodecTaskCommand_Seek:        //跳转
        break;
    case AVCodecTaskCommand_GetPacket:     //获取数据
        mCodec->getPacket();
        break;
    case AVCodecTaskCommand_SetFileName: //设置播放对象 -- url
        mCodec->setFilenameImpl(param2);
        break;
    case AVCodecTaskCommand_DecodeToRender : //+ 渲染成帧
        mCodec->decodec();
        break;
    case AVCodecTaskCommand_SetDecodecMode :
        break;
    case AVCodecTaskCommand_ShowFrameByPosition :   //按位置显示帧
        break;
    case AVCodecTaskCommand_RePlay :   //重新加载
        mCodec->_rePlay();
        break;

    }
}

AVDecoder::AVDecoder(QObject *parent) : QObject(parent), videoq(new PacketQueue)
{
#ifdef LIBAVUTIL_VERSION_MAJOR
#if (LIBAVUTIL_VERSION_MAJOR < 56)
    avcodec_register_all();
    avfilter_register_all();
    av_register_all();
    avformat_network_init();
#endif
#endif

    av_log_set_callback(NULL);//不打印日志
    av_lockmgr_register(lockmgr);

    _fpsTimer = new QTimer(this);
    _fpsTimer->setInterval(999);
    connect(_fpsTimer, &QTimer::timeout, this, &AVDecoder::onFpsTimeout);
    _fpsTimer->start();

    videoq->release();
    initRenderList();
}

AVDecoder::~AVDecoder()
{
    mProcessThread.stop();
    mDecodeThread.stop();
    int i = 0;
    while(!mProcessThread.isRunning() && i++ < 200){
        QThread::msleep(1);
    }
    i = 0;
    while(!mDecodeThread.isRunning() && i++ < 200){
        QThread::msleep(1);
    }

    release(true);
    if(videoq){
        videoq->release();
        delete videoq;
    }
}

void AVDecoder::setMediaCallback(AVMediaCallback *media){
    this->mCallback = media;
}

/* ---------------------------线程任务操作-------------------------------------- */
void AVDecoder::load(){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Init));
}

void AVDecoder::setFilename(const QString &source){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_SetFileName,0,source));
}

void AVDecoder::rePlay()
{
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_RePlay));
}

void AVDecoder::getPacketTask()
{
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_GetPacket));
}

void AVDecoder::decodecTask()
{
   mDecodeThread /*mProcessThread*/.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_DecodeToRender));
}

static int hw_decoder_init(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, const AVCodecHWConfig* config)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, config->device_type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    ctx->pix_fmt = config->pix_fmt;
    return err;
}

void AVDecoder::init(){
    if(mIsInit)
        return;
    mIsInit = true;
    statusChanged(AVDefine::AVMediaStatus_Loading);

    /* ----------------------------只是打印出来看看--------------------------- */
//    static QStringList codecs;
//    AVCodec* c = NULL;
//    while ((c=av_codec_next(c))) {
//        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
//            continue;
//        codecs.append(QString::fromLatin1(c->name));
//    }
//    qDebug() << "-----------------------codecs.size" << codecs.size() << codecs;

    AVDictionary* options = NULL;
//    av_dict_set(&options, "buffer_size", "10240", 0);  //增大“buffer_size”
//    av_dict_set(&options, "max_delay", "500000", 0);
//    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
    av_dict_set(&options, "preset", "ultrafast ", 0); // av_opt_set(pCodecCtx->priv_data,"preset","fast",0);
    av_dict_set(&options, "tune", "zerolatency", 0);
//    av_dict_set(&options, "rtsp_transport", "udp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp
    //寻找视频
    mFormatCtx = avformat_alloc_context();
    lastReadPacktTime = av_gettime();
    mFormatCtx->interrupt_callback.opaque = this;
    mFormatCtx->interrupt_callback.callback = interrupt_cb;

    if(avformat_open_input(&mFormatCtx, mFilename.toStdString().c_str(), NULL,&options) != 0) //打开视频
    {
        qDebug() << "media open error : " << mFilename.toStdString().data();
        statusChanged(AVDefine::AVMediaStatus_NoMedia);
        mIsInit = false;
        return;
    }
    mIsInit = false;

   mFormatCtx->probesize = 500000;  //
   qDebug() << "------------mFormatCtx->probesize  mFormatCtx->max_analyze_duration2"<< mFormatCtx->probesize << mFormatCtx->max_analyze_duration;

    //     qDebug()<<"file name is ==="<< QString(mFormatCtx->filename);
    //     qDebug()<<"the length is ==="<<mFormatCtx->duration/1000000;

    if(avformat_find_stream_info(mFormatCtx, NULL) < 0) //判断是否找到视频流信息
    {
        qDebug() << "media find stream error : " << mFilename.toStdString().data();
        statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }

    /* 寻找视频流 */
    int ret = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mVideoCodec, 0); //查找视频流信息

    if (ret < 0) {
        qDebug() << "Cannot find a video stream in the input file";
    }else{
        mVideoIndex = ret;
        //fps
        int fps_num = mFormatCtx->streams[mVideoIndex]->r_frame_rate.num;
        int fps_den = mFormatCtx->streams[mVideoIndex]->r_frame_rate.den;
        if (fps_den > 0) {
            _fps = fps_num / fps_den;
            if(mCallback)
                mCallback->mediaUpdateFps(_fps);
        }

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
//        mVideoCodecCtx = avcodec_alloc_context3(mVideoCodec);
        mVideo = mFormatCtx->streams[mVideoIndex];
        if (!mVideoCodecCtx){
            qDebug() << "create video context fail!" << mFormatCtx->filename;
            statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        }else{
            if (avcodec_parameters_to_context(mVideoCodecCtx, mVideo->codecpar) < 0)
                return;

            clearRenderList();
            if(mVideoCodecCtx != NULL){
                if(avcodec_is_open(mVideoCodecCtx))
                    avcodec_close(mVideoCodecCtx);
            }
//            mHWConfigList.clear();
            mVideoCodecCtxMutex.lockForWrite();
            if(mHWConfigList.size() > 0){
                for(int i = 0; i < mHWConfigList.size(); i++){
                    if(mVideoCodecCtx != NULL){
                        if(avcodec_is_open(mVideoCodecCtx))
                            avcodec_close(mVideoCodecCtx);
                    }

                    const AVCodecHWConfig *config = mHWConfigList.at(mHWConfigList.size() - 1- i); //mHWConfigList.front();
                    mHWPixFormat = config->pix_fmt;
                    mVideoCodecCtx->get_format = get_hw_format; //回调 &mHWDeviceCtx
                    if ( hw_decoder_init(mVideoCodecCtx, mHWDeviceCtx,config) < 0) {
                        mUseHw = false;
                    }else{
                        mUseHw = true;
                        mVideoCodecCtx->thread_count = 0;
                        mVideoCodecCtx->opaque = (void *) this;
                    }
                if( mUseHw && avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0){
                    mIsOpenVideoCodec = true;
                    qDebug() << "-----------------------config->device_type OK" << config->device_type << config->pix_fmt;
                    break;
                }else
                    qDebug() << "-----------------------config->device_type NG" << config->device_type << config->pix_fmt;
                }
            }else{
                mVideoCodecCtx->thread_count = 0; //线程数
//                mVideoCodecCtx->opaque = (void *) this;
                mIsOpenVideoCodec = avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0;
                mUseHw = false;
            }
            mVideoCodecCtxMutex.unlock();

            if(mIsOpenVideoCodec){
                AVDictionaryEntry *tag = NULL;
                tag = av_dict_get(mFormatCtx->streams[mVideoIndex]->metadata, "rotate", tag, 0);
                if(tag != NULL)
                    vFormat.rotate = QString(tag->value).toInt();
                av_free(tag);
                vFormat.width = mVideoCodecCtx->width;
                vFormat.height = mVideoCodecCtx->height;
                //                    vFormat.format = mVideoCodecCtx->pix_fmt;

                if(mCallback){
                    mCallback->mediaHasVideoChanged();
                }
                videoq->setTimeBase(mFormatCtx->streams[mVideoIndex]->time_base);
                videoq->init();
                //-- 启动抓包线程 和 解码线程
                getPacketTask();
                decodecTask();
            } else{
                statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
            }

            if(!mUseHw) //软解 -- 初始化转换器
            {
                if(mPixFormat != mVideoCodecCtx->pix_fmt)
                    mPixFormat = mVideoCodecCtx->pix_fmt;
                if(mSize != QSize(mVideoCodecCtx->width, mVideoCodecCtx->height) )
                    mSize = QSize(mVideoCodecCtx->width, mVideoCodecCtx->height);

                mVideoCodecCtx->pix_fmt =AV_PIX_FMT_D3D11VA_VLD;
                switch (mVideoCodecCtx->pix_fmt) {
                case AV_PIX_FMT_YUV420P :
                case AV_PIX_FMT_YUVJ420P :
                case AV_PIX_FMT_YUV422P :
                case AV_PIX_FMT_YUVJ422P :
                case AV_PIX_FMT_YUV444P :
                case AV_PIX_FMT_YUVJ444P :
                case AV_PIX_FMT_GRAY8 :
                case AV_PIX_FMT_UYVY422 :
                case AV_PIX_FMT_YUYV422 :
                case AV_PIX_FMT_YUV420P10LE :
                case AV_PIX_FMT_BGR24 :
                case AV_PIX_FMT_RGB24 :
                case AV_PIX_FMT_YUV410P :
                case AV_PIX_FMT_YUV411P :
                case AV_PIX_FMT_MONOWHITE :
                case AV_PIX_FMT_MONOBLACK :
                case AV_PIX_FMT_PAL8 :
                case AV_PIX_FMT_UYYVYY411 :
                case AV_PIX_FMT_BGR8 :
                case AV_PIX_FMT_RGB8 :
                case AV_PIX_FMT_NV12 :
                case AV_PIX_FMT_NV21 :
                case AV_PIX_FMT_ARGB :
                case AV_PIX_FMT_RGBA :
                case AV_PIX_FMT_ABGR :
                case AV_PIX_FMT_BGRA :
                case AV_PIX_FMT_GRAY16LE :
                case AV_PIX_FMT_YUV440P :
                case AV_PIX_FMT_YUVJ440P :
                case AV_PIX_FMT_YUVA420P :
                case AV_PIX_FMT_YUV420P16LE :
                case AV_PIX_FMT_YUV422P16LE :
                case AV_PIX_FMT_YUV444P16LE :
                case AV_PIX_FMT_YUVA420P16LE :
                case AV_PIX_FMT_YUVA422P16LE :
                case AV_PIX_FMT_YUVA444P16LE :
                case AV_PIX_FMT_YUV444P10LE :
                case AV_PIX_FMT_RGB565LE :
                case AV_PIX_FMT_RGB555LE :
                case AV_PIX_FMT_BGR565LE :
                case AV_PIX_FMT_BGR555LE :
                case AV_PIX_FMT_RGB444LE :
                case AV_PIX_FMT_BGR444LE :
                case AV_PIX_FMT_YUV422P10LE :
                case AV_PIX_FMT_YUVA420P10LE :
                case AV_PIX_FMT_YUVA422P10LE :
                case AV_PIX_FMT_YUVA444P10LE :
                case AV_PIX_FMT_NV16 :
                case AV_PIX_FMT_NV20LE :
                    break;
                default: //AV_PIX_FMT_YUV420P  如果上面的格式不支持直接渲染，则转换成通用AV_PIX_FMT_YUV420P格式
                    //                        vFormat.format = AV_PIX_FMT_YUV420P;
                    if(mVideoSwsCtx != NULL){
                        sws_freeContext(mVideoSwsCtx);
                        mVideoSwsCtx = NULL;
                    }
                    mVideoSwsCtx = sws_getContext(
                                mVideoCodecCtx->width,
                                mVideoCodecCtx->height,
                                mVideoCodecCtx->pix_fmt,
                                mVideoCodecCtx->width,
                                mVideoCodecCtx->height,
                                (AVPixelFormat)AV_PIX_FMT_YUV420P,// (AVPixelFormat)vFormat.format,
                                SWS_BICUBIC,NULL,NULL,NULL);
                    break;
                }
            }

            initEncodec();

            statusChanged(AVDefine::AVMediaStatus_Buffering);
            mIsInit = true;

            qDebug()<<"jie ma qi de name======    "<<mVideoCodec->name;
            qDebug("init play_video is ok!!!!!!!");
        }
    }
}

void AVDecoder::initEncodec()
{
    string output = outUrl;
    mIsInitEC = OpenOutput(output) == 0;
}

int AVDecoder::OpenOutput(string outUrl)
{
    inputContext = mFormatCtx;
    int ret  = avformat_alloc_output_context2(&outputContext, nullptr, "flv"/*flv"*/, outUrl.c_str());
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
        goto Error;
    }

    ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_READ_WRITE,nullptr, nullptr);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }
    for(int i = 0; i < inputContext->nb_streams; i++)
    {
        AVStream *in_stream  = inputContext->streams[i];
        if(in_stream->codec->codec_type != AVMEDIA_TYPE_VIDEO) //只解视频数据
            continue;

        AVStream *out_stream = avformat_new_stream(outputContext, in_stream->codec->codec);
        //法一
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        //法二
        //            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        //            out_stream->codecpar->codec_tag = 0;
        if(ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
            goto Error;
        }
        out_stream->codec->codec_tag = 0;  //这个值一定要为0
        if (outputContext->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    //av_dump_format(outputContext, 0, outUrl.c_str(), 1);

//    AVDictionary* options = NULL;
//    av_dict_set(&options,"rtbufsize","5000",0);
//    av_dict_set(&options,"start_time_realtime",0,0);
//    av_opt_set(outputContext->priv_data, "preset", "superfast", 0);
//    av_opt_set(outputContext->priv_data, "tune", "zerolatency", 0);
    ret = avformat_write_header(outputContext, NULL); //写入参数
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "format write header failed");
        goto Error;
    }

    av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n",outUrl.c_str());
    return ret ;
Error:
    if(outputContext)
    {
        for(int i = 0; i < outputContext->nb_streams; i++)
        {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }
    return ret ;
}

void AVDecoder::getPacket()
{
//    qDebug() << "------------------------------getPacket" <<  mIsInit << mIsOpenVideoCodec;
    if(!mIsInit || !mIsOpenVideoCodec){ //必须初始化
        statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }
    AVPacket *pkt = av_packet_alloc();
    mVideoCodecCtxMutex.lockForRead();
    lastReadPacktTime = av_gettime();
    int ret = av_read_frame(mFormatCtx, pkt);
    mVideoCodecCtxMutex.unlock();
    if ((ret == AVERROR_EOF || avio_feof(mFormatCtx->pb))) { //读取完成
        av_packet_unref(pkt);
        av_freep(pkt);
        getPacketTask();
        return;
    }

    static uchar errorSum = 0;
    if (mFormatCtx->pb && mFormatCtx->pb->error)
    {
        av_packet_unref(pkt);
        av_freep(pkt);
        getPacketTask();
        if(errorSum++ > 10) //一直报错 重启
            rePlay();
        return;
    }else{
        errorSum = 0;
    }
    if(ret < 0)
    {
        av_packet_unref(pkt);
        av_freep(pkt);
        getPacketTask();
        return;
    }

//    qDebug() << "-----------------------pkt->stream_index" << pkt->stream_index <<   mVideoIndex;
    if (pkt->stream_index == mVideoIndex && mIsOpenVideoCodec){
        //计算真实的渲染FPS
        _fpsFrameSum++;

        if(mIsInitEC){
            AVPacket *videopacket_t = av_packet_alloc();
            av_copy_packet(videopacket_t, pkt);
            if (av_dup_packet(videopacket_t) < 0)
            {
                av_free_packet(videopacket_t);
            }
            videopacket_t->pos = 0;
            videopacket_t->flags = 1;
            videopacket_t->convergence_duration = 0;
            videopacket_t->side_data_elems = 0;
            videopacket_t->stream_index = 0;
            videopacket_t->duration = 0;

            auto inputStream = inputContext->streams[videopacket_t->stream_index];
            auto outputStream = outputContext->streams[0];  //只有视屏
            av_packet_rescale_ts(videopacket_t,inputStream->time_base,outputStream->time_base);

            int ret = av_interleaved_write_frame(outputContext, videopacket_t);  //推流
            av_freep(videopacket_t);
//            qDebug() << "-----------------------------stream_index" << ret;
        }

        pkt->pts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        videoq->put(pkt);   //显示
//       qDebug() << "------------------------------getPacket" << videoq->size() << QDateTime::currentMSecsSinceEpoch();
    }else {
        av_packet_unref(pkt);
        av_freep(pkt);
    }
    getPacketTask();
}

void AVDecoder::decodec()
{
//    return;
//    qDebug() << "------------------------------decodec1" << videoq->size() << getRenderListSize();

    if(videoq->size() > 20) //清空
        videoq->release();

    if(videoq->size() <= 0 || getRenderListSize() >= maxRenderListSize) {
        decodecTask();
        return;
    }

    AVPacket *pkt = videoq->get();
    if (pkt->stream_index == mVideoIndex) {
        int ret = decode_write(mVideoCodecCtx, pkt);
//        qDebug() << "------------------------------decodec2" << ret <<videoq->size() << getRenderListSize();
    }
    av_packet_unref(pkt);
    av_freep(pkt);
    decodecTask();
}

int AVDecoder::decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL, *sw_frame = NULL;;
    int ret = 0;
    mVideoCodecCtxMutex.lockForRead();

    RenderItem* item = getInvalidRenderItem();
    if(!item) {
        mVideoCodecCtxMutex.unlock();
        qDebug() << "-------------------------decode_write lost one AVFrame" << getRenderListSize();
        return AVERROR(ENOMEM);
    }

    ret = avcodec_send_packet(avctx, packet);  // ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);
    if (ret < 0) {
        qWarning() << "Error during decoding";
        mVideoCodecCtxMutex.unlock();
//        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    sw_frame = av_frame_alloc();
    frame = sw_frame;

    while (avcodec_receive_frame(avctx, frame) == 0) {
        ret = 0;
//        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_unref(sw_frame);
            av_frame_free(&sw_frame);
            mVideoCodecCtxMutex.unlock();
            return 0;
        } else if (ret < 0) {
            qWarning() << "Error while decoding";
            goto fail;
        }

        item->mutex.lockForWrite();
        if (mUseHw) { // 硬解拷贝
            /* retrieve data from GPU to CPU */
            if(frame->format == mHWPixFormat ){
                if ((ret = av_hwframe_transfer_data(item->frame, frame, 0)) < 0) {
                    qWarning() << "Error transferring the data to system memory";
                    item->mutex.unlock();
                    goto fail;
                }else
                    av_frame_copy_props(item->frame, frame); 
            }
        } else{ //软解 转格式  --- [软解还没调好]  >> BUG 函数结束时 item->frame被清空 导致无数据
            if(mSize != QSize(frame->width, frame->height) || mPixFormat != mVideoCodecCtx->pix_fmt){
                /// 重要信息变更 需要转码时重启
                rePlay();
            }

            if(mVideoSwsCtx){
                ret = sws_scale(mVideoSwsCtx,
                                frame->data,
                                frame->linesize,
                                0,
                                frame->height,
                                item->frame->data,
                                item->frame->linesize);

                if(ret < 0){
                    qWarning() << "Error sws_scale";
                    item->mutex.unlock();
                    goto fail;
                }
            }else{  //内存移动 -- 不然会被释放
                av_frame_move_ref(item->frame, frame);
            }
        }

        item->pts   = item->frame->pts;
        item->valid = true;

        if(item->frame->pts != AV_NOPTS_VALUE){
                item->pts = av_q2d(mFormatCtx->streams[mVideoIndex]->time_base ) * item->frame->pts * 1000;
        }
        item->mutex.unlock();
    }

fail:
    av_frame_unref(sw_frame);
    av_frame_free(&sw_frame);
    mVideoCodecCtxMutex.unlock();

    return ret;
}

void AVDecoder::setFilenameImpl(const QString &source){

    if(mFilename == source) return;
    if(mFilename.size() > 0){
        release();//释放所有资源
    }
    mFilename = source;
    mIsOpenVideoCodec = false;
    mIsInit = false;
    if(mFilename.size() > 0)
        load();
}

void AVDecoder::_rePlay()
{
    if(mFilename.size() > 0){
        release();//释放所有资源
    }
    mIsOpenVideoCodec = false;
    mIsInit = false;
    if(mFilename.size() > 0)
        load();
}

/* ------------------------------------------------------私有函数-------------------------------------------------------------- */

void AVDecoder::statusChanged(AVDefine::AVMediaStatus status){
    if(mStatus == status)
        return;
    mStatus = status;
    if(mCallback)
        mCallback->mediaStatusChanged(status);
}

void AVDecoder::release(bool isDeleted){

    mDecodeThread.clearAllTask(); //清除所有任务
    mProcessThread.clearAllTask(); //清除所有任务

    mVideoCodecCtxMutex.lockForWrite();
    if(mVideoCodecCtx != NULL){
        if(avcodec_is_open(mVideoCodecCtx))
            avcodec_close(mVideoCodecCtx);
        avcodec_free_context(&mVideoCodecCtx);
        mVideoCodecCtx = NULL;
    }
    mVideoCodecCtxMutex.unlock();
    if(mVideoCodec != NULL){
        av_free(mVideoCodec);
        mVideoCodec = NULL;
    }

    if(mFormatCtx != NULL){
        if( mIsOpenVideoCodec){
            av_read_pause(mFormatCtx);
            avformat_close_input(&mFormatCtx);
        }
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }

    if(mVideoSwsCtx != NULL){
        sws_freeContext(mVideoSwsCtx);
        mVideoSwsCtx = NULL;
    }

    if(outputContext != NULL){
        avformat_free_context(outputContext);
        outputContext = NULL;
    }


    if(videoq)
        videoq->release();
    clearRenderList(isDeleted);
    statusChanged(AVDefine::AVMediaStatus_UnknownStatus);
}

/* ------------------------显示函数---------------------------------- */

qint64 AVDecoder::requestRenderNextFrame(){
    qint64 time = 0;
    if(getRenderListSize() > 0){
        /* 清理上次显示的数据 */
        RenderItem *render =  getShowRenderItem();
        if(render != nullptr){
            render->mutex.lockForWrite();
            render->mutexLock.lock();
            render->clear();
            render->mutexLock.unlock();
            render->mutex.unlock();
        }

        /* 显示本次数据 */
        render = getMinRenderItem();
        if(render != nullptr){
            render->mutex.lockForRead();
            render->isShowing = true;
            vFormat.width  =  render->frame->width;
            vFormat.height =  render->frame->height;
            vFormat.format =  render->frame->format;
            vFormat.renderFrame = render->frame;
            vFormat.renderFrameMutex = &render->mutexLock;
            time = render->pts;
            render->mutex.unlock();
            if(mCallback)
                mCallback->mediaUpdateVideoFrame((void *)&vFormat);
        }
    }
    return time;
}

/* ------------------------------------------------------队列操作-------------------------------------------------------------- */
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
        item->mutex.lockForRead();
        if(item->valid)
            ++ret;
        item->mutex.unlock();
    }
    mRenderListMutex.unlock();
    return ret;
}

qint64 AVDecoder::getNextFrameTime()
{
    qint64 minTime = 0;
    RenderItem *render = NULL;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        if(item->isShowing)
            continue;
        item->mutex.lockForRead();
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

void AVDecoder::clearRenderList(bool isDelete)
{
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lockForRead();
        if(item->valid)
        {
            mRenderList[i]->clear();
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
    int len = mRenderList.size();
    for( int i = 0;i < len;i++){
        RenderItem *item2 = mRenderList[i];
        item2->mutex.lockForRead();
        if(item2->isShowing){
            item2->mutex.unlock();
            continue;
        }
        if(!item2->valid){
            item = item2;
            item2->mutex.unlock();
            break;
        }
        item2->mutex.unlock();
    }
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
        item->mutex.lockForRead();
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
        if((i == len -1) && render){
            item->mutex.lockForWrite();
            render->isShowing = true;
            item->mutex.unlock();
        }

    }
    mRenderListMutex.unlock();
    return render;
}

RenderItem *AVDecoder::getShowRenderItem()
{
    bool getOk = false;
    RenderItem *render = NULL;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lockForRead();
        if(item->valid && item->isShowing){
            if(getOk){ //一种保护  把多余的清理掉
                item->clear();
                item->mutex.unlock();
                continue;
            }
            render = item;
            getOk = true;
        }
        item->mutex.unlock();
    }
    mRenderListMutex.unlock();
    return render;
}

void AVDecoder::changeRenderItemSize(int width, int height, AVPixelFormat format)
{
    mRenderListMutex.lockForWrite();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item2 = mRenderList[i];
        item2->mutex.lockForWrite();
        if(item2->valid){
            item2->release();
        }

        int numBytes = av_image_get_buffer_size( (AVPixelFormat)format, width,height, 1  );
        uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
        av_image_fill_arrays( item2->frame->data, item2->frame->linesize, buffer, (AVPixelFormat)format,width,height, 1 );

        item2->mutex.unlock();
    }
    mRenderListMutex.unlock();
}

void AVDecoder::onFpsTimeout()
{
    if(!mIsInit || !mIsOpenVideoCodec){ //必须初始化
        return;
    }
    if(getRenderListSize() <= 0) {
        return;
    }

    if(20 < _fpsFrameSum && _fpsFrameSum < 62){
        _fps = (_fpsFrameSum + _fps) / 2;
        if(mCallback)
            mCallback->mediaUpdateFps(_fps);
    }
    _fpsFrameSum = 0;
}
