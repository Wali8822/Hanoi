#include "HanoiDefs.h"
#include "resource.h"

HDC			hDC=NULL;		// 窗口着色描述表句柄
HGLRC		hRC=NULL;		// OpenGL渲染描述表句柄
HWND		hWnd=NULL;		// 保存我们的窗口句柄
HINSTANCE	hInstance;		// 保存程序的实例
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 400;
const int MENU_WIDTH=200;
const int MENU_HEIGHT=50;

Type_Move_Sequence sequence;

Type_Column  columnA,columnB,columnC;
Type_Column* columns[]={&columnA,&columnB,&columnC};
int current_select_column=0;
int current_select_menu_item=0;
int disk_count=5;
GLuint texForDisk,texForColumn;

wchar_t * menuTexts[]={L"Begin",L"Show AI",L"About",L"Exit"};
RectF * menuRects[4];
StringFormat * pFormat=NULL;
SolidBrush * pFontBrush=NULL;
SolidBrush * pBlackBrush=NULL;
SolidBrush * pGrayBrush=NULL;
Color  *  pColorTransparencyBlack=NULL;
Color  *  pColorMenuGray=NULL;
Color  *  pColorMenuText=NULL;
Font   *  pFont=NULL;
Graphics * gBuffer=NULL;
Image * pBuffer = NULL;
Bitmap *pBackground=NULL;

bool    bDrawAbout=false;
bool    bAI=false;
bool    bGLInit=false;
bool    bStarted=false;
bool	keys[256];			// 保存键盘按键的数组
bool	active=TRUE;		// 窗口的活动标志，缺省为TRUE
bool	fullscreen=TRUE;	// 全屏标志缺省，缺省设定成全屏模式
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// WndProc的定义
BOOL CALLBACK DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL SetFormat(int bits,int width,int height);
GLvoid DrawStartMenu(HWND hWnd);
GLboolean IsFinish();
GLvoid MoveDiskLeft();
GLvoid MoveDiskRight();
GLvoid MoveAI(Type_Move_Sequence &seq);
GLvoid DrawDisks();
GLvoid InitHanoi();
GLvoid UnInitHanoi();


GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// 重置OpenGL窗口大小
{
	if (height==0)										// 防止被零除
	{
		height=1;										// 将Height设为1
	}

	glViewport(0,0,width,height);						// 重置当前的视口

	glMatrixMode(GL_PROJECTION);						// 选择投影矩阵
	glLoadIdentity();									// 重置投影矩阵

	// 设置视口的大小
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// 选择模型观察矩阵
	glLoadIdentity();									// 重置模型观察矩阵
}

int InitGL(GLvoid)										// 此处开始对OpenGL进行所有设置
{
	GLfloat light_pos[]={6.0,3.0,6.0,0.0};
	GLfloat white_light[]={1.0,1.0,1.0,1.0};
	GLfloat mat_ambient[]={0.5,0.5,0.5,1.0};

	glShadeModel(GL_SMOOTH);							// 启用阴影平滑
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);				// 黑色背景
	glClearDepth(1.0f);									// 设置深度缓存
	glEnable(GL_DEPTH_TEST);							// 启用深度测试
	glDepthFunc(GL_LEQUAL);								// 所作深度测试的类型
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// 告诉系统对透视进行修正

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0,GL_POSITION,light_pos);
	glLightfv(GL_LIGHT0,GL_AMBIENT,white_light);
	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,mat_ambient);


	glEnable(GL_TEXTURE_2D);
	texForColumn = SOIL_load_OGL_texture("wood1.jpg",SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,SOIL_FLAG_DDS_LOAD_DIRECT );
	//texForDisk = SOIL_load_OGL_texture("stone.jpg",SOIL_LOAD_AUTO,
	//	SOIL_CREATE_NEW_ID,SOIL_FLAG_DDS_LOAD_DIRECT );

	texForDisk=texForColumn;
	if (texForColumn == 0 || texForDisk==0)
	{
		MessageBox(NULL,SOIL_last_result(),"Error",MB_OK|MB_ICONERROR);
	}

	return TRUE;										// 初始化 OK
}

int DrawGLScene(GLvoid)									// 从这里开始进行所有的绘制
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// 清除屏幕和深度缓存
	glMatrixMode(GL_MODELVIEW);                         //选择当前矩阵为模型观察矩阵
	glLoadIdentity();									// 重置当前的模型观察矩阵

	gluLookAt(0.0,7.0,10.0,
		      0.0,0.0,0.0,
			  0.0,1.0,0.0);

	if (bAI)
	{
		MoveAI(sequence);
	}

	DrawDisks();

	return TRUE;										// 一切 OK
}

GLvoid KillGLWindow(GLvoid)								// 正常销毁窗口
{
	if (fullscreen)										// 我们处于全屏模式吗?
	{
		ChangeDisplaySettings(NULL,0);					// 是的话，切换回桌面
		ShowCursor(TRUE);								// 显示鼠标指针
	}

	if (hRC)											//我们拥有OpenGL描述表吗?
	{
		if (!wglMakeCurrent(NULL,NULL))					// 我们能否释放DC和RC描述表?
		{
			MessageBox(NULL,"释放DC或RC失败。","关闭错误",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// 我们能否删除RC?
		{
			MessageBox(NULL,"释放RC失败。","关闭错误",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// 将RC设为 NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// 我们能否释放 DC?
	{
		MessageBox(NULL,"释放DC失败。","关闭错误",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// 将 DC 设为 NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// 能否销毁窗口?
	{
		MessageBox(NULL,"释放窗口句柄失败。","关闭错误",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// 将 hWnd 设为 NULL
	}

	if (!UnregisterClass("OpenG",hInstance))			// 能否注销类?
	{
		MessageBox(NULL,"不能注销窗口类。","关闭错误",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// 将 hInstance 设为 NULL
	}
}

/*	这个函数创建我们OpenGL窗口，参数为：									*
 *	title			- 窗口标题												*
 *	width			- 窗口宽度												*
 *	height			- 窗口高度												*
 *	bits			- 颜色的位深（8/16/32）									*
 *	fullscreenflag	- 是否使用全屏模式，全屏模式(TRUE)，窗口模式(FALSE)		*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	WNDCLASS	wc;						// 窗口类结构
	DWORD		dwExStyle;				// 扩展窗口风格
	DWORD		dwStyle;				// 窗口风格
	RECT		WindowRect;				// 取得矩形的左上角和右下角的坐标值
	WindowRect.left=(long)0;			// 将Left   设为 0
	WindowRect.right=(long)width;		// 将Right  设为要求的宽度
	WindowRect.top=(long)0;				// 将Top    设为 0
	WindowRect.bottom=(long)height;		// 将Bottom 设为要求的高度

	fullscreen=fullscreenflag;			// 设置全局全屏标志

	hInstance			= GetModuleHandle(NULL);				// 取得我们窗口的实例
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// 移动时重画，并为窗口取得DC
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc处理消息
	wc.cbClsExtra		= 0;									// 无额外窗口数据
	wc.cbWndExtra		= 0;									// 无额外窗口数据
	wc.hInstance		= hInstance;							// 设置实例
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// 装入缺省图标
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// 装入鼠标指针
	wc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);									// GL不需要背景
	wc.lpszMenuName		= NULL;									// 不需要菜单
	wc.lpszClassName	= "OpenG";								// 设定类名字

	if (!RegisterClass(&wc))									// 尝试注册窗口类
	{
		MessageBox(NULL,"注册窗口失败","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// 退出并返回FALSE
	}
	
	if (fullscreen)												// 要尝试全屏模式吗?
	{
		DEVMODE dmScreenSettings;								// 设备模式
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// 确保内存清空为零
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Devmode 结构的大小
		dmScreenSettings.dmPelsWidth	= width;				// 所选屏幕宽度
		dmScreenSettings.dmPelsHeight	= height;				// 所选屏幕高度
		dmScreenSettings.dmBitsPerPel	= bits;					// 每象素所选的色彩深度
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// 尝试设置显示模式并返回结果。注: CDS_FULLSCREEN 移去了状态条
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// 若模式失败，提供两个选项：退出或在窗口内运行。
			if (MessageBox(NULL,"全屏模式在当前显卡上设置失败！\n使用窗口模式？","NeHe G",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				//如果用户选择窗口模式，变量fullscreen 的值变为FALSE,程序继续运行
				fullscreen=FALSE;		// 选择窗口模式(Fullscreen=FALSE)
			}
			else
			{
				//如果用户选择退出，弹出消息窗口告知用户程序将结束。并返回FALSE告诉程序窗口未能成功创建。程序退出。
				MessageBox(NULL,"程序将被关闭","错误",MB_OK|MB_ICONSTOP);
				return FALSE;									// 退出并返回 FALSE
			}
		}
	}

	if (fullscreen)												// 仍处于全屏模式吗?
	{
		dwExStyle=WS_EX_APPWINDOW;								// 扩展窗体风格
		dwStyle=WS_POPUP;										// 窗体风格
		ShowCursor(FALSE);										// 隐藏鼠标指针
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// 扩展窗体风格
		dwStyle=WS_OVERLAPPEDWINDOW;							// 窗体风格
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// 调整窗口达到真正要求的大小

	// 创建窗口
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// 扩展窗体风格
								"OpenG",							// 类名字
								title,								// 窗口标题
								(dwStyle |							// 必须的窗体风格属性
								WS_CLIPSIBLINGS |					// 必须的窗体风格属性
								WS_CLIPCHILDREN),					// 必须的窗体风格属性
								0, 0,								// 窗口位置
								WindowRect.right-WindowRect.left,	// 计算调整好的窗口宽度
								WindowRect.bottom-WindowRect.top,	// 计算调整好的窗口高度
								NULL,								// 无父窗口
								NULL,								// 无菜单
								hInstance,							// 实例
								NULL)))								// 不向WM_CREATE传递任何东东
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"窗口创建错误","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// 显示窗口
	SetForegroundWindow(hWnd);						// 略略提高优先级
	SetFocus(hWnd);									// 设置键盘的焦点至此窗口

	return TRUE;									// 成功
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// 窗口的句柄	
							UINT	uMsg,			// 窗口的消息
							WPARAM	wParam,			// 附加的消息内容
							LPARAM	lParam)			// 附加的消息内容
{
	switch (uMsg)									// 检查Windows消息
	{
	    case WM_CREATE:
			{
				pBackground=new Bitmap(L"background.jpg");
				break;
			}
		case WM_ACTIVATE:							// 监视窗口激活消息
		{
			if (!HIWORD(wParam))					// 检查最小化状态
			{
				active=TRUE;						// 程序处于激活状态
			}
			else
			{
				active=FALSE;						// 程序不再激活
			}

			return 0;								// 返回消息循环
		}

		case WM_SYSCOMMAND:							// 系统中断命令
		{
			switch (wParam)							// 检查系统调用
			{
				case SC_SCREENSAVE:					// 屏保要运行?
				case SC_MONITORPOWER:				// 显示器要进入节电模式?
				return 0;							// 阻止发生
			}
			break;									// 退出
		}

		case WM_CLOSE:								// 收到Close消息?
		{
			if (pBackground!=NULL)
			{
				delete pBackground;
			}
			PostQuitMessage(0);						// 发出退出消息
			return 0;								// 返回
		}

		case WM_KEYDOWN:							// 有键按下么?
		{
			keys[wParam] = TRUE;					// 如果是，设为TRUE
			return 0;								// 返回
		}

		case WM_KEYUP:								// 有键放开么?
		{
			keys[wParam] = FALSE;					// 如果是，设为FALSE
			return 0;								// 返回
		}

		case WM_SIZE:								// 调整OpenGL窗口大小
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width,HiWord=Height
			return 0;								// 返回
		}
	}

	// 向 DefWindowProc传递所有未处理的消息。
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE	hInstance,			// 当前窗口实例
					HINSTANCE	hPrevInstance,		// 前一个窗口实例
					LPSTR		lpCmdLine,			// 命令行参数
					int			nCmdShow)			// 窗口显示状态
{
	ULONG_PTR token=0;
	GdiplusStartupInput input;
	MSG		msg;									// Windowsx消息结构
	BOOL	done=FALSE;								// 用来退出循环的Bool 变量

	if(GdiplusStartup(&token,&input,NULL) != Ok)
		return 0;

	InitHanoi();
	//始终不全屏
	fullscreen=FALSE;
	// 创建OpenGL窗口
	if (!CreateGLWindow("Hanoi",WINDOW_WIDTH,WINDOW_HEIGHT,16,fullscreen))
	{
		return 0;									// 失败退出
	}

	while(!done)									// 保持循环直到 done=TRUE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// 有消息在等待吗?
		{
			if (msg.message==WM_QUIT)				// 收到退出消息?
			{
				done=TRUE;							// 是，则done=TRUE
			}
			else									// 不是，处理窗口消息
			{
				TranslateMessage(&msg);				// 翻译消息
				DispatchMessage(&msg);				// 发送消息
			}
		}
		else										// 如果没有消息
		{
			// 绘制场景。监视ESC键和来自DrawGLScene()的退出消息
			if (active)								// 程序激活的么?
			{
				if (keys[VK_ESCAPE])				// ESC 按下了么?
				{
					done=TRUE;						// ESC 发出退出信号
				}
				else								// 不是退出的时候，刷新屏幕
				{
					if (!bStarted)
					{
						DrawStartMenu(hWnd);
					}
					else
					{
						if (!bGLInit)
						{
							SetFormat(16,WINDOW_WIDTH,WINDOW_HEIGHT);
							bGLInit=true;
						}
						DrawGLScene();					// 绘制场景
						SwapBuffers(hDC);				// 交换缓存 (双缓存)
					}
				}

				if (keys[VK_SPACE])
				{
					if (current_select_menu_item==0)
					{
						bStarted=true;
					}
					if (current_select_menu_item==1)
					{
						bStarted=true;
						bAI=true;
					}
					if (current_select_menu_item==3)
					{
						break;
					}
					keys[VK_SPACE]=FALSE;
				}

				if ( keys['D'])
				{
					if (bStarted && !bAI)
					{
						(++current_select_column)%=3;
						keys['D']=FALSE;
					}
				}
				if ( keys['A'])
				{
					if (bStarted && !bAI)
					{
						if(--current_select_column == -1)
							current_select_column=2;
						keys['A']=FALSE;
					}
				}
				if ( keys[VK_LEFT])
				{
					if (bStarted && !bAI)
					{
						MoveDiskLeft();
						if (IsFinish())
						{
							MessageBox(NULL,"Congratulations!!!You win!!!!",
								"Message",MB_OK|MB_ICONINFORMATION);
						}
						keys[VK_LEFT]=FALSE;
					}
				}
				if ( keys[VK_RIGHT])
				{
					if (bStarted && !bAI)
					{
						MoveDiskRight();
						if (IsFinish())
						{
							MessageBox(NULL,"Congratulations!!!You win!!!!",
								"Message",MB_OK|MB_ICONINFORMATION);
						}
						keys[VK_RIGHT]=FALSE;
					}
				}
				if (keys[VK_UP] && !bStarted)
				{
					if(--current_select_menu_item == -1)
						current_select_menu_item=3;
					keys[VK_UP]=FALSE;
				}
				if (keys[VK_DOWN] && !bStarted)
				{
					(++current_select_menu_item)%=4;
					keys[VK_DOWN]=FALSE;
				}
			}

			if (keys[VK_F1])						// F1键按下了么?
			{
				keys[VK_F1]=FALSE;					// 若是，使对应的Key数组中的值为 FALSE
				KillGLWindow();						// 销毁当前的窗口
				fullscreen=!fullscreen;				// 切换 全屏 / 窗口 模式
				// 重建 OpenGL 窗口
				if (!CreateGLWindow("Hanoi",640,480,16,fullscreen))
				{
					return 0;						// 如果窗口未能创建，程序退出
				}
			}
		}
	}
	
	UnInitHanoi();
	GdiplusShutdown(token);
	// 关闭程序
	KillGLWindow();									// 销毁窗口
	return (msg.wParam);							// 退出程序
}

GLvoid DrawDisks()
{
	GLUquadric * objDisk=NULL,*objColumn=NULL;

	objDisk=gluNewQuadric();
	objColumn=gluNewQuadric();
	gluQuadricDrawStyle(objColumn,GL_FILL);
	gluQuadricDrawStyle(objDisk,GL_FILL);
	//gluQuadricTexture(obj,_texture);
	gluQuadricTexture(objDisk,texForDisk);
	gluQuadricTexture(objColumn,texForColumn);

	gluQuadricNormals(objColumn,GL_FLAT);
	gluQuadricNormals(objDisk,GL_FLAT);

	glPushMatrix();

	glColor3f(1.0,0.0,0.0);
	glTranslatef(0.0,-1.0,0.0);
	glScalef(15.0,1.0,5.5);
	auxSolidCube(1.0);

	glPopMatrix();
	//draw column 1 2 3

	for (int e=0;e<3;++e)
	{
		Type_Column * pColumn=columns[e];

		glPushMatrix();

		glTranslatef(-4+e*4,0.0,0.0);

	    for (size_t i=0;i<pColumn->size();++i)
		{
			PDISK disk=pColumn->at(i);
			glPushMatrix();

			if ((current_select_column == e) && (i == (pColumn->size()-1)))
			{
				glTranslatef(0.0,DISK_HEIGHT*i+0.3,0.0);
			}
			else
			{
				glTranslatef(0.0,DISK_HEIGHT*i,0.0);
			}

			glRotatef(90.0,1.0,0.0,0.0);

			glColor3f(1.0,1.0,1.0);
			gluDisk(objDisk,COLUMN_RADIUS,disk->fRadius,32,4);
			//glColor3f(0.0,0.0,0.0);
			gluCylinder(objDisk,disk->fRadius,
			     disk->fRadius,DISK_HEIGHT,50,50);

			glColor3f(1.0,1.0,1.0);
			glPushMatrix();
			glTranslatef(0.0,0.0,DISK_HEIGHT);
			gluDisk(objDisk,COLUMN_RADIUS,disk->fRadius,32,4);
			glPopMatrix();

			glPopMatrix();
		}

		glPushMatrix();
		glRotatef(-90.0,1.0,0.0,0.0);
		gluCylinder(objColumn,COLUMN_RADIUS,COLUMN_RADIUS,COLUMN_HEIGHT,64,64);
		glPopMatrix();

		glPopMatrix();
	}

	gluDeleteQuadric(objColumn);
	gluDeleteQuadric(objDisk);
}

GLvoid InitHanoi()
{
	for (int i=0;i<disk_count;++i)
	{
		PDISK pDisk=new DISK;
		pDisk->fRadius = MIN_DISK_RADIUS + i*0.09;
		pDisk->nNumber=i;
		columnA.push_back(pDisk);
	}
	reverse(columnA.begin(),columnA.end());

	N=disk_count;
	hanoi(disk_count,sequence);

	pFont=new Font(L"Arial",20,FontStyleBold);

	pGrayBrush=new SolidBrush(Color(255,128,128,128));
	pFontBrush=new SolidBrush(Color(Color::Black));
	pBlackBrush=new SolidBrush(Color(128,0,0,0));
	pFormat=new StringFormat;
	pFormat->SetAlignment(StringAlignmentCenter);
	pFormat->SetLineAlignment(StringAlignmentCenter);

	for (int i=0;i<4;++i)
	{
		menuRects[i]=new RectF(300,90+(MENU_HEIGHT+10)*i,MENU_WIDTH,MENU_HEIGHT);
	}
}

GLvoid UnInitHanoi()
{
	for (Type_Column_Iterator i=columnA.begin();i!=columnA.end();++i)
	{
		delete (*i);
	}
	for (Type_Column_Iterator i=columnB.begin();i!=columnB.end();++i)
	{
		delete (*i);
	}
	for (Type_Column_Iterator i=columnC.begin();i!=columnC.end();++i)
	{
		delete (*i);
	}

	columnA.clear();
	columnB.clear();
	columnC.clear();


	delete pFont;
	delete pBuffer;
	delete pFontBrush;
	delete pBlackBrush;
	delete pFormat;

	for (int i=0;i<4;++i)
	{
		delete menuRects[i];
	}
}

GLvoid MoveDiskLeft()
{
	if ((current_select_column==0) || (columns[current_select_column]->empty()))
	{
		MessageBox(NULL,"你不能这样移动！！！！","违法操作",MB_OK|MB_ICONERROR);
		return;
	}

	Type_Column * pCurrentColumn=columns[current_select_column];
	Type_Column * pLeft=columns[--current_select_column];

	//determine if radius of the disk
	if (!pLeft->empty())
	{	
		PDISK diskLeft=*(--(pLeft->end()));
		PDISK diskCurr=*(--(pCurrentColumn->end()));

		if (diskLeft->fRadius < diskCurr->fRadius)
		{
			//if can move jump the middle one
			if (current_select_column == 1)
			{		
				PDISK diskRightMost=*(--(columns[2]->end()));
				if (!columns[0]->empty())
				{				
					PDISK diskLeftMost=*(--(columns[0]->end()));
					if (diskLeftMost->fRadius < diskRightMost->fRadius)
					{
						return ;
					}
				}
				columns[0]->push_back(diskRightMost);
				columns[2]->pop_back();	
				return ;
			}
			return ;
		}
	}
	
	pLeft->push_back(*(--(pCurrentColumn->end())));
	pCurrentColumn->pop_back();
}

GLvoid MoveDiskRight()
{
	if ((current_select_column==2) || (columns[current_select_column]->empty()))
	{
		MessageBox(NULL,"已经到最右端！！！！","违法操作",MB_OK|MB_ICONERROR);
		return;
	}

	Type_Column * pCurrentColumn=columns[current_select_column];
	Type_Column * pRight=columns[++current_select_column];

	if (!pRight->empty())
	{
		PDISK diskRight=*(--(pRight->end()));
		PDISK diskCurr=*(--(pCurrentColumn->end()));

		if (diskRight->fRadius < diskCurr->fRadius)
		{
			if (current_select_column == 1)
			{	
				PDISK diskLeftMost=*(--(columns[0]->end()));
				if (!columns[2]->empty())
				{	
					PDISK diskRightMost=*(--(columns[2]->end()));	
					if (diskLeftMost->fRadius > diskRightMost->fRadius)
					{
						return ;
					}
				}
				columns[2]->push_back(diskLeftMost);
				columns[0]->pop_back();
				return ;
			}
			return ;
		}
	}

	pRight->push_back(*(--(pCurrentColumn->end())));
	pCurrentColumn->pop_back();
}

GLboolean IsFinish()
{
	return columnC.size()==disk_count;
}

GLvoid MoveAI( Type_Move_Sequence &seq )
{
	static unsigned long index=0;
	static long tick=GetTickCount();
		
	if (index >= seq.size())
	{
		return ;
	}
	STEP s=seq[index];
	if ((!columns[s.from]->empty()) && ((GetTickCount()-tick)>=400))
	{
		tick = GetTickCount();

		columns[s.to]->push_back(*(--(columns[s.from]->end())));
		columns[s.from]->pop_back();

		index++;
	}
}


GLvoid DrawStartMenu( HWND hWnd )
{
	Graphics g(hWnd);

	if (pBuffer==NULL)
	{
        pBuffer=new Bitmap(WINDOW_WIDTH,WINDOW_HEIGHT);
		gBuffer=Graphics::FromImage(pBuffer);
	}

	gBuffer->DrawImage(pBackground,Rect(0,0,
		WINDOW_WIDTH,WINDOW_HEIGHT));

	for (int i=0;i<4;++i)
	{
		if (current_select_menu_item==i)
		{
			gBuffer->FillRectangle(pGrayBrush,*(menuRects[i]));
		}
		else
		{
			gBuffer->FillRectangle(pBlackBrush,*(menuRects[i]));
		}	
		gBuffer->DrawString(menuTexts[i],lstrlenW(menuTexts[i]),pFont,*(menuRects[i]),
			pFormat,pFontBrush);
	}

	g.DrawImage(pBuffer,0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
}

BOOL SetFormat( int bits ,int width,int height)
{
	GLuint		PixelFormat;			// 保存查找匹配的结果
	static	PIXELFORMATDESCRIPTOR pfd=				//pfd 告诉窗口我们所希望的东东，即窗口使用的像素格式
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// 上述格式描述符的大小
		1,											// 版本号
		PFD_DRAW_TO_WINDOW |						// 格式支持窗口
		PFD_SUPPORT_OPENGL |						// 格式必须支持OpenGL
		PFD_DOUBLEBUFFER,							// 必须支持双缓冲
		PFD_TYPE_RGBA,								// 申请 RGBA 格式
		bits,										// 选定色彩深度
		0, 0, 0, 0, 0, 0,							// 忽略的色彩位
		0,											// 无Alpha缓存
		0,											// 忽略Shift Bit
		0,											// 无累加缓存
		0, 0, 0, 0,									// 忽略聚集位
		16,											// 16位 Z-缓存 (深度缓存) 
		0,											// 无蒙板缓存
		0,											// 无辅助缓存
		PFD_MAIN_PLANE,								// 主绘图层
		0,											// 不使用重叠层
		0, 0, 0										// 忽略层遮罩
	};

	if (!(hDC=GetDC(hWnd)))							// 取得设备描述表了么?
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"不能创建一个窗口设备描述表","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Windows 找到相应的象素格式了吗?
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"不能创建一种相匹配的像素格式","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// 能够设置象素格式么?
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"不能设置像素格式","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// 能否取得OpenGL渲染描述表?
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"不能创建OpenGL渲染描述表","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// 尝试激活着色描述表
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"不能激活当前的OpenGL渲然描述表","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	ReSizeGLScene(width, height);					// 设置透视 GL 屏幕

	if (!InitGL())									// 初始化新建的GL窗口
	{
		KillGLWindow();								// 重置显示区
		MessageBox(NULL,"初始化失败","错误",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// 返回 FALSE
	}

	return TRUE;
}


