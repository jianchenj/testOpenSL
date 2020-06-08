#include <jni.h>
#include <string>
#include <SLES/OpenSLES.h>
#include <android/log.h>
#include <SLES/OpenSLES_Android.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testOpenSL",__VA_ARGS__)

static SLObjectItf engineSL = NULL;

SLEngineItf CreateSL() {
    SLresult re;

//
    SLEngineItf eng;
    re = slCreateEngine(&engineSL, 0, 0, 0, 0, 0);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    re = (*engineSL)->Realize(engineSL, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    re = (*engineSL)->GetInterface(engineSL, SL_IID_ENGINE, &eng);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    return eng;
}

void PcmCall(SLAndroidSimpleBufferQueueItf bf, void *context) {
    LOGW("PcmCall");
    static FILE *fp = nullptr;
    static char *buf = nullptr;
    fp = fopen("/sdcard/test.pcm", "rb");
    if (!buf) {
        buf = new char[1024 * 1024];
    }
    if (!fp) return;
    if (feof(fp) == 0) {//没有读取到文件尾部继续读
        int len = fread(buf, 1, 1024, fp);
        if (len > 0) {
            (*bf)->Enqueue(bf, buf, len);
        }
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_jchen_testopensl_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    SLresult re = 0;

    //1.创建引擎
    SLEngineItf eng = CreateSL();
    if (eng) {
        LOGW("CreateSL success");
    } else {
        LOGW("CreateSL failed");
    }

    //2.创建混音器
    SLObjectItf mix = nullptr;
    re = (*eng)->CreateOutputMix(eng, &mix, 0, 0, 0);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("CreateOutputMix failed");
    }
    re = (*mix)->Realize(mix, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("Realize mix failed");
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mix};
    SLDataSink audioSink = {&outputMix, 0};

    //3 配置音频信息
    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,
                            2, //声道数
                            SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN //字节序，小端
    };
    SLDataSource ds = {&que, &pcm};

    //4 创建播放器
    SLObjectItf player = nullptr;
    SLPlayItf iplayer = nullptr;
    SLAndroidSimpleBufferQueueItf pcmQue = nullptr;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_FALSE};
    re = (*eng)->CreateAudioPlayer(eng, &player, &ds, &audioSink,
                                   sizeof(ids) / sizeof(SLInterfaceID), ids, req);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("CreateAudioPlayer failed");
    } else {
        LOGW("CreateAudioPlayer success");
    }
    (*player)->Realize(player, SL_BOOLEAN_FALSE);
    //获取play接口
    re = (*player)->GetInterface(player, SL_IID_PLAY, &iplayer);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("GetInterface SL_IID_PLAY failed");
    } else {
        LOGW("GetInterface SL_IID_PLAY success");
    }
    re = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &pcmQue);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("GetInterface SL_IID_BUFFERQUEUE failed");
    } else {
        LOGW("GetInterface SL_IID_BUFFERQUEUE success");
    }
    //设置回调函数，在播放队列空时调用
    (*pcmQue)->RegisterCallback(pcmQue, PcmCall, 0);
    //设置为播放中状态
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PLAYING);
    //启动队列回调
    (*pcmQue)->Enqueue(pcmQue, "", 1);
    return env->NewStringUTF(hello.c_str());
}
