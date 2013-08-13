#include "HanoiDefs.h"
#include "resource.h"

HDC			hDC=NULL;		// ������ɫ��������
HGLRC		hRC=NULL;		// OpenGL��Ⱦ��������
HWND		hWnd=NULL;		// �������ǵĴ��ھ��
HINSTANCE	hInstance;		// ��������ʵ��
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
bool	keys[256];			// ������̰���������
bool	active=TRUE;		// ���ڵĻ��־��ȱʡΪTRUE
bool	fullscreen=TRUE;	// ȫ����־ȱʡ��ȱʡ�趨��ȫ��ģʽ
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// WndProc�Ķ���
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


GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// ����OpenGL���ڴ�С
{
	if (height==0)										// ��ֹ�����
	{
		height=1;										// ��Height��Ϊ1
	}

	glViewport(0,0,width,height);						// ���õ�ǰ���ӿ�

	glMatrixMode(GL_PROJECTION);						// ѡ��ͶӰ����
	glLoadIdentity();									// ����ͶӰ����

	// �����ӿڵĴ�С
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// ѡ��ģ�͹۲����
	glLoadIdentity();									// ����ģ�͹۲����
}

int InitGL(GLvoid)										// �˴���ʼ��OpenGL������������
{
	GLfloat light_pos[]={6.0,3.0,6.0,0.0};
	GLfloat white_light[]={1.0,1.0,1.0,1.0};
	GLfloat mat_ambient[]={0.5,0.5,0.5,1.0};

	glShadeModel(GL_SMOOTH);							// ������Ӱƽ��
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);				// ��ɫ����
	glClearDepth(1.0f);									// ������Ȼ���
	glEnable(GL_DEPTH_TEST);							// ������Ȳ���
	glDepthFunc(GL_LEQUAL);								// ������Ȳ��Ե�����
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// ����ϵͳ��͸�ӽ�������

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

	return TRUE;										// ��ʼ�� OK
}

int DrawGLScene(GLvoid)									// �����￪ʼ�������еĻ���
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// �����Ļ����Ȼ���
	glMatrixMode(GL_MODELVIEW);                         //ѡ��ǰ����Ϊģ�͹۲����
	glLoadIdentity();									// ���õ�ǰ��ģ�͹۲����

	gluLookAt(0.0,7.0,10.0,
		      0.0,0.0,0.0,
			  0.0,1.0,0.0);

	if (bAI)
	{
		MoveAI(sequence);
	}

	DrawDisks();

	return TRUE;										// һ�� OK
}

GLvoid KillGLWindow(GLvoid)								// �������ٴ���
{
	if (fullscreen)										// ���Ǵ���ȫ��ģʽ��?
	{
		ChangeDisplaySettings(NULL,0);					// �ǵĻ����л�������
		ShowCursor(TRUE);								// ��ʾ���ָ��
	}

	if (hRC)											//����ӵ��OpenGL��������?
	{
		if (!wglMakeCurrent(NULL,NULL))					// �����ܷ��ͷ�DC��RC������?
		{
			MessageBox(NULL,"�ͷ�DC��RCʧ�ܡ�","�رմ���",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// �����ܷ�ɾ��RC?
		{
			MessageBox(NULL,"�ͷ�RCʧ�ܡ�","�رմ���",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// ��RC��Ϊ NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// �����ܷ��ͷ� DC?
	{
		MessageBox(NULL,"�ͷ�DCʧ�ܡ�","�رմ���",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// �� DC ��Ϊ NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// �ܷ����ٴ���?
	{
		MessageBox(NULL,"�ͷŴ��ھ��ʧ�ܡ�","�رմ���",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// �� hWnd ��Ϊ NULL
	}

	if (!UnregisterClass("OpenG",hInstance))			// �ܷ�ע����?
	{
		MessageBox(NULL,"����ע�������ࡣ","�رմ���",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// �� hInstance ��Ϊ NULL
	}
}

/*	���������������OpenGL���ڣ�����Ϊ��									*
 *	title			- ���ڱ���												*
 *	width			- ���ڿ��												*
 *	height			- ���ڸ߶�												*
 *	bits			- ��ɫ��λ�8/16/32��									*
 *	fullscreenflag	- �Ƿ�ʹ��ȫ��ģʽ��ȫ��ģʽ(TRUE)������ģʽ(FALSE)		*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	WNDCLASS	wc;						// ������ṹ
	DWORD		dwExStyle;				// ��չ���ڷ��
	DWORD		dwStyle;				// ���ڷ��
	RECT		WindowRect;				// ȡ�þ��ε����ϽǺ����½ǵ�����ֵ
	WindowRect.left=(long)0;			// ��Left   ��Ϊ 0
	WindowRect.right=(long)width;		// ��Right  ��ΪҪ��Ŀ��
	WindowRect.top=(long)0;				// ��Top    ��Ϊ 0
	WindowRect.bottom=(long)height;		// ��Bottom ��ΪҪ��ĸ߶�

	fullscreen=fullscreenflag;			// ����ȫ��ȫ����־

	hInstance			= GetModuleHandle(NULL);				// ȡ�����Ǵ��ڵ�ʵ��
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// �ƶ�ʱ�ػ�����Ϊ����ȡ��DC
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc������Ϣ
	wc.cbClsExtra		= 0;									// �޶��ⴰ������
	wc.cbWndExtra		= 0;									// �޶��ⴰ������
	wc.hInstance		= hInstance;							// ����ʵ��
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// װ��ȱʡͼ��
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// װ�����ָ��
	wc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);									// GL����Ҫ����
	wc.lpszMenuName		= NULL;									// ����Ҫ�˵�
	wc.lpszClassName	= "OpenG";								// �趨������

	if (!RegisterClass(&wc))									// ����ע�ᴰ����
	{
		MessageBox(NULL,"ע�ᴰ��ʧ��","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// �˳�������FALSE
	}
	
	if (fullscreen)												// Ҫ����ȫ��ģʽ��?
	{
		DEVMODE dmScreenSettings;								// �豸ģʽ
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// ȷ���ڴ����Ϊ��
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Devmode �ṹ�Ĵ�С
		dmScreenSettings.dmPelsWidth	= width;				// ��ѡ��Ļ���
		dmScreenSettings.dmPelsHeight	= height;				// ��ѡ��Ļ�߶�
		dmScreenSettings.dmBitsPerPel	= bits;					// ÿ������ѡ��ɫ�����
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// ����������ʾģʽ�����ؽ����ע: CDS_FULLSCREEN ��ȥ��״̬��
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// ��ģʽʧ�ܣ��ṩ����ѡ��˳����ڴ��������С�
			if (MessageBox(NULL,"ȫ��ģʽ�ڵ�ǰ�Կ�������ʧ�ܣ�\nʹ�ô���ģʽ��","NeHe G",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				//����û�ѡ�񴰿�ģʽ������fullscreen ��ֵ��ΪFALSE,�����������
				fullscreen=FALSE;		// ѡ�񴰿�ģʽ(Fullscreen=FALSE)
			}
			else
			{
				//����û�ѡ���˳���������Ϣ���ڸ�֪�û����򽫽�����������FALSE���߳��򴰿�δ�ܳɹ������������˳���
				MessageBox(NULL,"���򽫱��ر�","����",MB_OK|MB_ICONSTOP);
				return FALSE;									// �˳������� FALSE
			}
		}
	}

	if (fullscreen)												// �Դ���ȫ��ģʽ��?
	{
		dwExStyle=WS_EX_APPWINDOW;								// ��չ������
		dwStyle=WS_POPUP;										// ������
		ShowCursor(FALSE);										// �������ָ��
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// ��չ������
		dwStyle=WS_OVERLAPPEDWINDOW;							// ������
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// �������ڴﵽ����Ҫ��Ĵ�С

	// ��������
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// ��չ������
								"OpenG",							// ������
								title,								// ���ڱ���
								(dwStyle |							// ����Ĵ���������
								WS_CLIPSIBLINGS |					// ����Ĵ���������
								WS_CLIPCHILDREN),					// ����Ĵ���������
								0, 0,								// ����λ��
								WindowRect.right-WindowRect.left,	// ��������õĴ��ڿ��
								WindowRect.bottom-WindowRect.top,	// ��������õĴ��ڸ߶�
								NULL,								// �޸�����
								NULL,								// �޲˵�
								hInstance,							// ʵ��
								NULL)))								// ����WM_CREATE�����κζ���
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"���ڴ�������","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// ��ʾ����
	SetForegroundWindow(hWnd);						// ����������ȼ�
	SetFocus(hWnd);									// ���ü��̵Ľ������˴���

	return TRUE;									// �ɹ�
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// ���ڵľ��	
							UINT	uMsg,			// ���ڵ���Ϣ
							WPARAM	wParam,			// ���ӵ���Ϣ����
							LPARAM	lParam)			// ���ӵ���Ϣ����
{
	switch (uMsg)									// ���Windows��Ϣ
	{
	    case WM_CREATE:
			{
				pBackground=new Bitmap(L"background.jpg");
				break;
			}
		case WM_ACTIVATE:							// ���Ӵ��ڼ�����Ϣ
		{
			if (!HIWORD(wParam))					// �����С��״̬
			{
				active=TRUE;						// �����ڼ���״̬
			}
			else
			{
				active=FALSE;						// �����ټ���
			}

			return 0;								// ������Ϣѭ��
		}

		case WM_SYSCOMMAND:							// ϵͳ�ж�����
		{
			switch (wParam)							// ���ϵͳ����
			{
				case SC_SCREENSAVE:					// ����Ҫ����?
				case SC_MONITORPOWER:				// ��ʾ��Ҫ����ڵ�ģʽ?
				return 0;							// ��ֹ����
			}
			break;									// �˳�
		}

		case WM_CLOSE:								// �յ�Close��Ϣ?
		{
			if (pBackground!=NULL)
			{
				delete pBackground;
			}
			PostQuitMessage(0);						// �����˳���Ϣ
			return 0;								// ����
		}

		case WM_KEYDOWN:							// �м�����ô?
		{
			keys[wParam] = TRUE;					// ����ǣ���ΪTRUE
			return 0;								// ����
		}

		case WM_KEYUP:								// �м��ſ�ô?
		{
			keys[wParam] = FALSE;					// ����ǣ���ΪFALSE
			return 0;								// ����
		}

		case WM_SIZE:								// ����OpenGL���ڴ�С
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width,HiWord=Height
			return 0;								// ����
		}
	}

	// �� DefWindowProc��������δ�������Ϣ��
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE	hInstance,			// ��ǰ����ʵ��
					HINSTANCE	hPrevInstance,		// ǰһ������ʵ��
					LPSTR		lpCmdLine,			// �����в���
					int			nCmdShow)			// ������ʾ״̬
{
	ULONG_PTR token=0;
	GdiplusStartupInput input;
	MSG		msg;									// Windowsx��Ϣ�ṹ
	BOOL	done=FALSE;								// �����˳�ѭ����Bool ����

	if(GdiplusStartup(&token,&input,NULL) != Ok)
		return 0;

	InitHanoi();
	//ʼ�ղ�ȫ��
	fullscreen=FALSE;
	// ����OpenGL����
	if (!CreateGLWindow("Hanoi",WINDOW_WIDTH,WINDOW_HEIGHT,16,fullscreen))
	{
		return 0;									// ʧ���˳�
	}

	while(!done)									// ����ѭ��ֱ�� done=TRUE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// ����Ϣ�ڵȴ���?
		{
			if (msg.message==WM_QUIT)				// �յ��˳���Ϣ?
			{
				done=TRUE;							// �ǣ���done=TRUE
			}
			else									// ���ǣ���������Ϣ
			{
				TranslateMessage(&msg);				// ������Ϣ
				DispatchMessage(&msg);				// ������Ϣ
			}
		}
		else										// ���û����Ϣ
		{
			// ���Ƴ���������ESC��������DrawGLScene()���˳���Ϣ
			if (active)								// ���򼤻��ô?
			{
				if (keys[VK_ESCAPE])				// ESC ������ô?
				{
					done=TRUE;						// ESC �����˳��ź�
				}
				else								// �����˳���ʱ��ˢ����Ļ
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
						DrawGLScene();					// ���Ƴ���
						SwapBuffers(hDC);				// �������� (˫����)
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

			if (keys[VK_F1])						// F1��������ô?
			{
				keys[VK_F1]=FALSE;					// ���ǣ�ʹ��Ӧ��Key�����е�ֵΪ FALSE
				KillGLWindow();						// ���ٵ�ǰ�Ĵ���
				fullscreen=!fullscreen;				// �л� ȫ�� / ���� ģʽ
				// �ؽ� OpenGL ����
				if (!CreateGLWindow("Hanoi",640,480,16,fullscreen))
				{
					return 0;						// �������δ�ܴ����������˳�
				}
			}
		}
	}
	
	UnInitHanoi();
	GdiplusShutdown(token);
	// �رճ���
	KillGLWindow();									// ���ٴ���
	return (msg.wParam);							// �˳�����
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
		MessageBox(NULL,"�㲻�������ƶ���������","Υ������",MB_OK|MB_ICONERROR);
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
		MessageBox(NULL,"�Ѿ������Ҷˣ�������","Υ������",MB_OK|MB_ICONERROR);
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
	GLuint		PixelFormat;			// �������ƥ��Ľ��
	static	PIXELFORMATDESCRIPTOR pfd=				//pfd ���ߴ���������ϣ���Ķ�����������ʹ�õ����ظ�ʽ
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// ������ʽ�������Ĵ�С
		1,											// �汾��
		PFD_DRAW_TO_WINDOW |						// ��ʽ֧�ִ���
		PFD_SUPPORT_OPENGL |						// ��ʽ����֧��OpenGL
		PFD_DOUBLEBUFFER,							// ����֧��˫����
		PFD_TYPE_RGBA,								// ���� RGBA ��ʽ
		bits,										// ѡ��ɫ�����
		0, 0, 0, 0, 0, 0,							// ���Ե�ɫ��λ
		0,											// ��Alpha����
		0,											// ����Shift Bit
		0,											// ���ۼӻ���
		0, 0, 0, 0,									// ���Ծۼ�λ
		16,											// 16λ Z-���� (��Ȼ���) 
		0,											// ���ɰ建��
		0,											// �޸�������
		PFD_MAIN_PLANE,								// ����ͼ��
		0,											// ��ʹ���ص���
		0, 0, 0										// ���Բ�����
	};

	if (!(hDC=GetDC(hWnd)))							// ȡ���豸��������ô?
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"���ܴ���һ�������豸������","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Windows �ҵ���Ӧ�����ظ�ʽ����?
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"���ܴ���һ����ƥ������ظ�ʽ","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// �ܹ��������ظ�ʽô?
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"�����������ظ�ʽ","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// �ܷ�ȡ��OpenGL��Ⱦ������?
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"���ܴ���OpenGL��Ⱦ������","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// ���Լ�����ɫ������
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"���ܼ��ǰ��OpenGL��Ȼ������","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	ReSizeGLScene(width, height);					// ����͸�� GL ��Ļ

	if (!InitGL())									// ��ʼ���½���GL����
	{
		KillGLWindow();								// ������ʾ��
		MessageBox(NULL,"��ʼ��ʧ��","����",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// ���� FALSE
	}

	return TRUE;
}


