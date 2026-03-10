#ifndef _AUDIO_MANAGER_H
#define _AUDIO_MANAGER_H

#include <espeak-ng/speak_lib.h>

struct SpeakerOpr;

typedef struct SpeakerOpr T_SpeakerOpr, *PT_SpeakerOpr;

struct SpeakerOpr {
	char *name;
	char *voice_name;
	int volume;
	int speed;
	int (*SpeakerInit)(void);
	int (*Speak)(PT_SpeakerOpr ptSpeakerOpr, char *strText);
	int (*StopSpeak)(void);
	struct SpeakerOpr *ptNext;
};

int RegisterSpeakerOpr(PT_SpeakerOpr ptSpeakerOpr);
void ShowSpeakerOpr(void);
PT_SpeakerOpr GetSpeakerOpr(char *pcName);
int SpeakerInit(void);
int EspeakInit(void);

#endif /* _AUDIO_MANAGER_H */

