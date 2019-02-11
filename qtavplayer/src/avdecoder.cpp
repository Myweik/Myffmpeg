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

void AVDecoder::init(){
    if(mIsInit)
        return;
    mIsInit = true;
//     statusChanged(AVDefine::AVMediaStatus_Loading);/**/

        //寻找视频
     mFormatCtx = avformat_alloc_context();
     if(avformat_open_input(&mFormatCtx, mFilename.toStdString().c_str(), NULL, NULL) != 0)
     {
         qDebug() << "media open error : " << mFilename.toStdString().data();
//         statusChanged(AVDefine::AVMediaStatus_NoMedia);
         mIsInit = false;
         return;
     }
     mIsInit = false;

//     qDebug()<<"file name is ==="<< QString(mFormatCtx->filename);
//     qDebug()<<"the length is ==="<<mFormatCtx->duration/1000000;

     if(avformat_find_stream_info(mFormatCtx, NULL) < 0)
     {
         qDebug() << "media find stream error : " << mFilename.toStdString().data();
//         statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
         return;
     }

    /* 寻找视频流 */
    int ret = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mVideoCodec, 0);
    if (ret < 0) {
        qDebug() << "Cannot find a video stream in the input file";
    }else{
        mVideoIndex = ret;
        mVideoCodecCtx = avcodec_alloc_context3(mVideoCodec);
        if (!mVideoCodecCtx){
            qDebug() << "create video context fail!" << mFormatCtx->filename;
            //            qDebug() << "avformat_open_input 3";
//            statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        }else{
//            avcodec_parameters_to_context(mVideoCodecCtx, mFormatCtx->streams[mVideoIndex]->codecpar);
            clearRenderList();
            mVideoCodecCtx->thread_count = 0;
            mIsOpenVideoCodec = avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) >= 0;
            mFrame = av_frame_alloc();
            mPacket = av_packet_alloc(); // new 空间
            mIsInit = true;
//            statusChanged(AVDefine::AVMediaStatus_Buffering);
            mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));

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

void AVDecoder::decodec(){
    if(!mIsInit || !mIsOpenVideoCodec){ //必须初始化
//        statusChanged(AVDefine::AVMediaStatus_InvalidMedia);
        return;
    }
    av_free_packet(mPacket);

    if (av_read_frame(mFormatCtx, mPacket) < 0)
    {
        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
        return;
    }
    if (mPacket->stream_index == mVideoIndex) {
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

//        qDebug() << "-----------------4" <<  got_picture<< getRenderListSize() ;
        if (got_picture) {

            if(mSize != QSize(mFrame->width, mFrame->height) || mVideoSwsCtx == NULL){
                if(mVideoSwsCtx != NULL){
                    sws_freeContext(mVideoSwsCtx);
                    mVideoSwsCtx = NULL;
                }

                if(mSize != QSize(mFrame->width, mFrame->height) ){
                    mSize = QSize(mFrame->width, mFrame->height);
                }

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
//    av_free_packet(mPacket);
    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));





//    if (av_read_frame(pFormatCtx, packet) < 0)
//    {
//        mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
//        return;
//    }
//    if (packet->stream_index == videoStream) {
//        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);

//        if (ret < 0) {
//            printf("decode error.");
//            mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
//            return;
//        }

//        if (got_picture) {
//            qDebug() << "-----------------pFrame" <<pFrame->format;
//            sws_scale(img_convert_ctx,
//                      (uint8_t const * const *) pFrame->data,
//                      pFrame->linesize, 0, pCodecCtx->height,
//                      pAVPicture.data,
//                      pAVPicture.linesize);
//            QImage imag = QImage(pAVPicture.data[0],pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB888);;

//            if(imag.isNull())
//            {
//                qDebug()<<"image =======    ";
//                mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
//                return;
//            }
//            send_img(imag);
//        }
//    }
//    mProcessThread.addTask(new AVCodecTask(this,AVCodecTask::AVCodecTaskCommand_Decodec));
//    return;

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
