#include <input_manager.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


static int StdinDeviceInit(void)
{
	 struct termios tTTYState;
 
    //get the terminal state
    tcgetattr(STDIN_FILENO, &tTTYState);
 
    //turn off canonical mode
    tTTYState.c_lflag &= ~ICANON;
    //minimum of number input read.
    tTTYState.c_cc[VMIN] = 1;  /* 有一个数据时，立刻返回 */
		
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &tTTYState);

	return 0;
}
static int StdinDeviceExit(void)
{
	struct termios tTTYState;
 
    //get the terminal state
    tcgetattr(STDIN_FILENO, &tTTYState);
 
    //turn on canonical mode
    tTTYState.c_lflag |= ICANON;
		
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &tTTYState);

	return 0;
}
static int StdinGetInputEvent(PT_InputEvent ptInputEvent)
{
	/* 如果有数据就读取、处理、返回
	 * 如果没有数据，立刻返回，不等待
	 */
	 /* select，poll可以参考 UNIX环境高级编程 */
	char c;
	
	/* 处理数据 */
	ptInputEvent->iType = INPUT_TYPE_STDIN;
	
	/* 会休眠直到有输入 */
	c = fgetc(stdin);
	gettimeofday(&ptInputEvent->tTime, NULL);
	
	if (c == 'u')
	{
		ptInputEvent->iVal = INPUT_VALUE_UP;
	}
	else if (c == 'n')
	{
		ptInputEvent->iVal = INPUT_VALUE_DOWN;
	}
	else if (c == 'q')
	{
		ptInputEvent->iVal = INPUT_VALUE_EXIT;
	}
	else if (c == 's')
	{
		ptInputEvent->iVal = INPUT_VALUE_READ;
	}
	else
	{
		ptInputEvent->iVal = INPUT_VALUE_UNKNOWN;
	}
	return 0;
}


static T_InputOpr g_tStdinOpr = {
	.name 			= "stdin",
	.DeviceInit 	= StdinDeviceInit,
	.DeviceExit 	= StdinDeviceExit,
	.GetInputEvent  = StdinGetInputEvent,
};


int StdinInit(void)
{
	return RegisterInputOpr(&g_tStdinOpr);
}

