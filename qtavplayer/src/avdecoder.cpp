#include "avdecoder.h"
#include <QDebug>

#include <QDateTime>

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
}



AVDecoder::AVDecoder(QObject *parent) : QObject(parent)
{
    av_log_set_callback(NULL);//不打印日志
    av_register_all();
    avcodec_register_all();
    //    av_lockmgr_register(lockmgr);
    initRenderList();
}

void AVDecoder::setMediaCallback(AVMediaCallback *media){
    this->mCallback = media;
}

void AVDecoder::load(){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Init));
}

void AVDecoder::setFilename(const QString &source){
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_SetFileName,0,source));
}

static int hw_decoder_init(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, const enum AVHWDeviceType type)
{
    int err = 0;
    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    //    mVideoCodecCtx->hw_device_ctx = av_buffer_ref(mHWDeviceCtx); //创建对avbuffer的新引用
    //                        mHWFrame = av_frame_alloc(); //分配空间
    //                        changeRenderItemSize(mVideoCodecCtx->width,mVideoCodecCtx->height,(AVPixelFormat)mHWPixFormat); //更改呈现项大小
    return err;
}

void AVDecoder::init(){
    if(mIsInit)
        return;
    mIsInit = true;
    statusChanged(AVDefine::AVMediaStatus_Loading);

    static QStringList codecs;
    AVCodec* c = NULL;
    while ((c=av_codec_next(c))) {
        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
            continue;
        codecs.append(QString::fromLatin1(c->name));
    }
    qDebug() << "-----------------------codecs.size" << codecs.size() << codecs;

    AVDictionary* options = NULL;
    av_dict_set(&options, "buffer_size", "102400", 0);  //增大“buffer_size”
//    av_dict_set(&options, "max_delay", "500000", 0);
//    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
//    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp

    //寻找视频
//    mFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&mFormatCtx, mFilename.toStdString().c_str(), NULL, &options) != 0) //打开视频
    {
        qDebug() << "media open error : " << mFilename.toStdString().data();
        statusChanged(AVDefine::AVMediaStatus_NoMedia);
        mIsInit = false;
        return;
    }
    mIsInit = false;

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
        qDebug() << "---------------------------fps  mVideoCodec->codec_id"  << _fps << mVideoCodec->id;

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
        if (!mVideoCodecCtx){
            qDebug() << "create video context fail!" << mFormatCtx->filename;
            statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        }else{

            if (avcodec_parameters_to_context(mVideoCodecCtx, mVideo->codecpar) < 0)
                return;
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
                    if ( hw_decoder_init(mVideoCodecCtx, mHWDeviceCtx,config->device_type) < 0) {
                        mUseHw = false;
                    }else{
                        mUseHw = true;
                        mVideoCodecCtx->thread_count = 0;
                        mVideoCodecCtx->opaque = (void *) this;
                    }
                if( mUseHw && avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0){
                    mIsOpenVideoCodec = true;
                    break;
                }else
                    qDebug() << "-----------------------config->device_type NG" << config->device_type;
                }
            }else{
                mVideoCodecCtx->thread_count = 8; //线程数
                mIsOpenVideoCodec = avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0;
                mUseHw = false;
            }

            if(mIsOpenVideoCodec){
                    AVDictionaryEntry *tag = NULL;
                    tag = av_dict_get(mFormatCtx->streams[mVideoIndex]->metadata, "rotate", tag, 0);
                    if(tag != NULL)
                        vFormat.rotate = QString(tag->value).toInt();
                    av_free(tag);
                    vFormat.width = mVideoCodecCtx->width;
                    vFormat.height = mVideoCodecCtx->height;
//                    vFormat.format = mVideoCodecCtx->pix_fmt;
            }
            mPacket = av_packet_alloc(); // new 空间

            statusChanged(AVDefine::AVMediaStatus_Buffering);
            mIsInit = true;
            clearRenderList();
            mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));

            qDebug()<<"jie ma qi de name======    "<<mVideoCodec->name;
            qDebug("init play_video is ok!!!!!!!");
        }
    }
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

    //    mStatus = AVDefine::AVMediaStatus_UnknownStatus;

    clearRenderList(isDeleted);
}

int AVDecoder::decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL;
    int ret = 0;
    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    RenderItem* item = getInvalidRenderItem();
    if(!item) {
        qDebug() << "-------------------------decode_write lost one AVFrame" << getRenderListSize();
        return AVERROR(ENOMEM);
    }
    frame = av_frame_alloc();

    while (1) {
        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        if(mSize != QSize(frame->width, frame->height) || mVideoSwsCtx == NULL){
            if(mVideoSwsCtx != NULL){
                sws_freeContext(mVideoSwsCtx);
                mVideoSwsCtx = NULL;
            }

            if(mSize != QSize(frame->width, frame->height) ){
                mSize = QSize(frame->width, frame->height);
            }
            //软解设置
            mVideoSwsCtx = sws_getContext(
                        frame->width,
                        frame->height,
                        (AVPixelFormat)frame->format,
//                        960, 540,
                        frame->width,
                        frame->height,
                        AV_PIX_FMT_YUV420P,
                        SWS_BICUBIC,NULL,NULL,NULL);
//            avpicture_alloc(&mAVPicture,AV_PIX_FMT_RGB24,frame->width,frame->height/*960, 540*/);
        }
        item->mutex.lock();
        if (frame->format == mHWPixFormat) { // 硬解拷贝
            /* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(item->frame, frame, 0)) < 0) {
                fprintf(stderr, "Error transferring the data to system memory\n");
                item->mutex.unlock();
                goto fail;
            }else
                av_frame_copy_props(item->frame, frame);
        } else{ //软解 转格式
            ret = sws_scale(mVideoSwsCtx,
                            frame->data,
                            frame->linesize,
                            0,
                            frame->height,
                            item->frame->data,
                            item->frame->linesize);

            if(ret < 0){
                fprintf(stderr, "Error sws_scale\n");
                item->mutex.unlock();
                goto fail;
            }
        }

        item->pts =   item->frame->pts;
        item->valid = true;
        if(item->frame->pts != AV_NOPTS_VALUE)
            item->pts = av_q2d(mFormatCtx->streams[mVideoIndex]->time_base ) * item->frame->pts * 1000;
        item->mutex.unlock();
    fail:
        av_frame_unref(frame);
//        av_frame_free(&frame);
        if (ret < 0)
            return ret;
    }
}

void AVDecoder::decodec(){
    if(!mIsInit || !mIsOpenVideoCodec){ //必须初始化
        statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }
    if (av_read_frame(mFormatCtx, mPacket) < 0) //读取数据
    {
        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
        return;
    }

    if (mPacket->stream_index == mVideoIndex) {
        if(mUseHw){ //硬解
            int ret = decode_write(mVideoCodecCtx, mPacket);
//            qDebug() << "----------------------------decode_write" << ret << getRenderListSize();
        }
    }
    av_packet_unref(mPacket);
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

        /* 清理上次显示的数据 */
        RenderItem *render =  getShowRenderItem();
        if(render != nullptr){
            render->mutex.lock();
            render->clear();
            render->mutex.unlock();
        }

        /* 显示本次数据 */
        render = getMinRenderItem();
        if(render != nullptr){
            render->mutex.lock();
            render->isShowing = true;
            vFormat.width  =  render->frame->width;
            vFormat.height =  render->frame->height;
            vFormat.format =  render->frame->format;
            vFormat.renderFrame = render->frame;
            vFormat.renderFrameMutex = &render->mutex;
            time = render->pts;
            render->mutex.unlock();
            if(mCallback)
                mCallback->mediaUpdateVideoFrame((void *)&vFormat);
        }
    }
    return time;
}


/* ------------------------------------------------------私有函数-------------------------------------------------------------- */

void AVDecoder::statusChanged(AVDefine::AVMediaStatus status){
    if(mStatus == status)
        return;
    mStatus = status;
    if(mCallback)
        mCallback->mediaStatusChanged(status);
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
    int len = mRenderList.size();
    for( int i = 0;i < len;i++){
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

RenderItem *AVDecoder::getShowRenderItem()
{
    bool getOk = false;
    RenderItem *render = NULL;
    mRenderListMutex.lockForRead();
    for(int i = 0,len = mRenderList.size();i < len;i++){
        RenderItem *item = mRenderList[i];
        item->mutex.lock();
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
