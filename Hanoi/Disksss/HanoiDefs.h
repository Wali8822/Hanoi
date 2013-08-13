#include <windows.h>		// Windows的头文件
#include <gl/gl.h>		// 包含OpenGL实用库
#include <gl/GLU.h>
#include <gl/glaux.h>
#include "SOIL.h"
#include <vector>
#include <algorithm>
#include <GdiPlus.h>

#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"SOIL.lib")
#pragma comment(lib,"glaux.lib")
#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;
using namespace std;


const GLfloat MAX_DISK_RADIUS = 2.0f;
const INT MAX_DISK_COUNT = 64;
const GLfloat DISK_HEIGHT = 0.15f;
const GLfloat COLUMN_RADIUS = 0.1f;
const GLfloat COLUMN_HEIGHT = 5.0f;
const GLfloat MIN_DISK_RADIUS = 0.3f;

typedef struct _DISK 
{
	INT nNumber;
	GLfloat fRadius;
}DISK,*PDISK;

typedef struct _STEP 
{
	char from;
	char to;
}STEP,*PSTEP;


typedef vector<PDISK>  Type_Column;
typedef vector<PDISK>::iterator Type_Column_Iterator;

typedef vector<STEP> Type_Move_Sequence;
typedef vector<STEP>::iterator Type_Move_Sequence_Iterator;

long N=0;

void move(int m,Type_Move_Sequence & seq)      //第m次移动，对应于移动序列1,2,1,3,1,2,1,4,1,2,1….的第m项
{
	int from,to;
	int p = m;
	int k = 1;
	for(k=1;p%2 != 1;k++)
		p /= 2;
	p /= 2;
	m=1;
	if((N+k+1)%2) m=-1;
	if((from=(1+p*m)%3) <=0)from +=3;
	if((to = (1+(p+1)*m)%3) <= 0) to +=3;

	STEP s;
	s.from=from-1;
	s.to=to-1;

	seq.push_back(s);
}

void hanoi(int n,Type_Move_Sequence & seq)
{
	int s,m,i;
	s = 1;
	for(i=1;i<=n;i++)s *= 2;
	s -= 1;
	for(m=1; m<s; m++)
		move(m,seq);
}