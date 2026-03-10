#include <config.h>
#include <audio_manager.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <espeak-ng/speak_lib.h>


/* 全局变量，用于管理 ALSA 声卡句柄 */
static snd_pcm_t *g_pcm_handle = NULL;
static pthread_t g_tSpeakThread;
static char *g_strPendingText = NULL;
static pthread_mutex_t g_tEspeakMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_tEspeakConVar = PTHREAD_COND_INITIALIZER;
//static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

static int g_bIsEspeakBusy = 0;


/**
 * SynthCallback: espeak-ng 合成音频数据后的回调函数
 * 我们在这里手动将数据写入 ALSA 声卡
 */
static int SynthCallback(short *wav, int numsamples, espeak_EVENT *events) 
{
    if (numsamples > 0 && g_pcm_handle) {
        /* 将 espeak 产生的 wav 数据写入声卡 */
        snd_pcm_sframes_t frames = snd_pcm_writei(g_pcm_handle, wav, numsamples);
        if (frames < 0) {
            /* 发生错误（如 Xrun），尝试恢复声卡状态 */
            snd_pcm_prepare(g_pcm_handle);
        }
    }
    return 0; /* 返回 0 继续合成 */
}


/**
 * SpeakThreadFunction: 独立的朗读线程
 */
static void *SpeakThreadFunction(void *pVoid)
{
    char *strText;
    
    while (1) {
        /* 等待信号：只有当有文字需要朗读时才唤醒 */
        pthread_mutex_lock(&g_tEspeakMutex);
        while (g_strPendingText == NULL) {
            pthread_cond_wait(&g_tEspeakConVar, &g_tEspeakMutex);
        }
        strText = g_strPendingText;
        g_strPendingText = NULL;
        g_bIsEspeakBusy = 1;
        pthread_mutex_unlock(&g_tEspeakMutex);

        /* 执行合成（同步模式在此线程阻塞，不影响主线程按键） */
        espeak_Synth(strText, strlen(strText) + 1, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);
        
        free(strText);
        g_bIsEspeakBusy = 0;
    }
    return NULL;
}


/**
 * EspeakInitDevice: 初始化 espeak 引擎并打开 ALSA 硬件
 */
static int EspeakInitDevice(void)
{
    int ret;

    /* 1. 初始化 espeak 引擎为同步模式 */
    /* 同步模式会确保音频数据通过 SynthCallback 返回，而不是由引擎尝试自行播放 */
    if (espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 500, NULL, 0) < 0) {
        printf("Espeak engine initialize failed!\n");
        return -1;
    }
    
    espeak_SetSynthCallback(SynthCallback);

    /* 2. 打开 ALSA 默认播放设备 */
    ret = snd_pcm_open(&g_pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        printf("ALSA: Cannot open audio device 'default' (%s)\n", snd_strerror(ret));
        return -1;
    }

    /* 3. 设置声卡参数 */
    /* espeak-ng 默认输出通常为 22050Hz, 单声道, 16bit */
    ret = snd_pcm_set_params(g_pcm_handle,
                             SND_PCM_FORMAT_S16_LE,          /* 16位小端 */
                             SND_PCM_ACCESS_RW_INTERLEAVED,  /* 交错访问 */
                             1,                              /* 单声道 */
                             22050,                          /* 采样率 */
                             1,                              /* 允许软重采样 */
                             500000);                        /* 500ms 延迟 */
    if (ret < 0) {
        printf("ALSA: Hardware parameters set failed (%s)\n", snd_strerror(ret));
        return -1;
    }

	/* 4. 创建朗读线程 */
    pthread_create(&g_tSpeakThread, NULL, SpeakThreadFunction, NULL);

    return 0;
}

/**
 * EspeakSpeak: 执行朗读任务
 */
static int EspeakSpeak(PT_SpeakerOpr ptSpeakerOpr, char *strText)
{
    /* 先停止当前的朗读 */
    espeak_Cancel();

    /* 将文字拷贝并传递给线程 */
    pthread_mutex_lock(&g_tEspeakMutex);
    if (g_strPendingText) free(g_strPendingText);
    g_strPendingText = strdup(strText);

    /* 设置语种，确保包含中文支持 */
    espeak_SetVoiceByName(ptSpeakerOpr->voice_name);
    
    /* 设置音量和语速参数 */
    espeak_SetParameter(espeakVOLUME, ptSpeakerOpr->volume, 0);
    espeak_SetParameter(espeakRATE, ptSpeakerOpr->speed, 0);
    
    /* 唤醒线程开始工作 */
    pthread_cond_signal(&g_tEspeakConVar);
    pthread_mutex_unlock(&g_tEspeakMutex);
    
    return 0;
}

/**
 * EspeakStop: 停止当前朗读
 */
static int EspeakStop(void)
{
    /* 取消合成任务，这会停止回调的触发 */
    return espeak_Cancel();
}

/* 定义 espeak 操作结构体 */
static T_SpeakerOpr g_tEspeakOpr = {
    .name        = "espeak",
    .voice_name  = "cmn",       /* 默认使用中文 */
    .volume      = 100,
    .speed       = 160,
    .SpeakerInit = EspeakInitDevice,
    .Speak       = EspeakSpeak,
    .StopSpeak   = EspeakStop,
};

/**
 * EspeakInit: 注册 espeak 驱动到音频管理器
 */
int EspeakInit(void)
{
    return RegisterSpeakerOpr(&g_tEspeakOpr);
}
