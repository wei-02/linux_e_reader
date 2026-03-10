
#include <config.h>
#include <audio_manager.h>
#include <string.h>

static PT_SpeakerOpr g_ptSpeakerOprHead;

int RegisterSpeakerOpr(PT_SpeakerOpr ptSpeakerOpr)
{
	PT_SpeakerOpr ptTmp;

	if (!g_ptSpeakerOprHead)
	{
		g_ptSpeakerOprHead   = ptSpeakerOpr;
		ptSpeakerOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptSpeakerOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	  = ptSpeakerOpr;
		ptSpeakerOpr->ptNext = NULL;
	}

	return 0;
}


void ShowSpeakerOpr(void)
{
	int i = 0;
	PT_SpeakerOpr ptTmp = g_ptSpeakerOprHead;

	while (ptTmp)
	{
		printf("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

PT_SpeakerOpr GetSpeakerOpr(char *pcName)
{
	PT_SpeakerOpr ptTmp = g_ptSpeakerOprHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}

int SpeakerInit(void)
{
	int iError;
	
	iError = EspeakInit();

	return iError;
}

