#pragma once
/*  __ |  \   /   __|
 *    /     /    |
 *  ___|  _/ _\ \___|  
 */
#ifndef ASSERTEXW_LEVEL
#define ASSERTEXW_LEVEL 0
#endif // !ASSERTEXW_LEVEL

const static struct{
	int _ASSERTEXW_P;
	int _ASSERTEXW_Q;
}_assertexw_bool2 = {1,1};
#include<wchar.h>
#define _ASSERTEXW_Q(format, ...) _ASSERTEXW_Q&&fwprintf(stderr,format,##__VA_ARGS__)&&_assertexw_bool2._ASSERTEXW_P
#define _ASSERTEXW_P(format, ...) _ASSERTEXW_P&&fwprintf(stderr,format,##__VA_ARGS__)&&_assertexw_bool2._ASSERTEXW_Q

#define ASSERTEXW_LV(lv) ((lv)<(ASSERTEXW_LEVEL))||
#define ASSERTEXW(x) (x)||fwprintf(stderr,L"!!!%S(%S: %i)''%S''",__func__,__FILE__,__LINE__,#x)&&_assertexw_bool2._ASSERTEXW_Q
