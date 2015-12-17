#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib,"gdiplus")
#pragma comment(lib,"glew32s")
#pragma comment(lib,"shlwapi")
#pragma comment(lib,"rpcrt4")
#pragma comment(lib,"vfw32.lib")

#define GLEW_STATIC

#include <vector>
#include <string>
#include <windows.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <richedit.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <stdint.h>
#include "glmakeavi.h"

#define PREVIEW_WIDTH 64
#define PREVIEW_HEIGHT 64
#define ID_SELECTALL 1001
#define WM_CREATED WM_APP

HDC hDC;
BOOL active;
GLuint program;
GLuint vao;
GLuint vbo;
const TCHAR szClassName[] = TEXT("Window");
const GLfloat position[][2] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
const int vertices = sizeof position / sizeof position[0];
const GLchar vsrc[] = "in vec4 position;void main(void){gl_Position = position;}";
GLuint texture;

inline GLint GetShaderInfoLog(GLuint shader)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == 0) OutputDebugString(TEXT("Compile Error\n"));
	GLsizei bufSize;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufSize);
	if (bufSize > 1)
	{
		LPSTR infoLog = (LPSTR)GlobalAlloc(0, bufSize);
		GLsizei length;
		glGetShaderInfoLog(shader, bufSize, &length, infoLog);
		OutputDebugStringA(infoLog);
		GlobalFree(infoLog);
	}
	return status;
}

inline GLint GetProgramInfoLog(GLuint program)
{
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == 0) OutputDebugString(TEXT("Link Error\n"));
	GLsizei bufSize;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufSize);
	if (bufSize > 1)
	{
		LPSTR infoLog = (LPSTR)GlobalAlloc(0, bufSize);
		GLsizei length;
		glGetProgramInfoLog(program, bufSize, &length, infoLog);
		OutputDebugStringA(infoLog);
		GlobalFree(infoLog);
	}
	return status;
}

inline GLuint CreateProgram(LPCSTR vsrc, LPCSTR fsrc)
{
	const GLuint vobj = glCreateShader(GL_VERTEX_SHADER);
	if (!vobj) return 0;
	glShaderSource(vobj, 1, &vsrc, 0);
	glCompileShader(vobj);
	if (GetShaderInfoLog(vobj) == 0)
	{
		glDeleteShader(vobj);
		return 0;
	}
	const GLuint fobj = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fobj)
	{
		glDeleteShader(vobj);
		return 0;
	}
	glShaderSource(fobj, 1, &fsrc, 0);
	try
	{
		glCompileShader(fobj);
	}
	catch (...)
	{
		glDeleteShader(vobj);
		glDeleteShader(fobj);
		return 0;
	}
	if (GetShaderInfoLog(fobj) == 0)
	{
		glDeleteShader(vobj);
		glDeleteShader(fobj);
		return 0;
	}
	GLuint program = glCreateProgram();
	if (program)
	{
		glAttachShader(program, vobj);
		glAttachShader(program, fobj);
		glLinkProgram(program);
		if (GetProgramInfoLog(program) == 0)
		{
			glDetachShader(program, fobj);
			glDetachShader(program, vobj);
			glDeleteProgram(program);
			program = 0;
		}
	}
	glDeleteShader(vobj);
	glDeleteShader(fobj);
	return program;
}

inline BOOL InitGL(GLvoid)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 *
		vertices, position, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return TRUE;
}

inline VOID DrawGLScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glUseProgram(program);
	glUniform1f(glGetUniformLocation(program, "time"), GetTickCount() / 1000.0f);

	if (texture)
	{
		glOrtho(-PREVIEW_WIDTH / 2.0f, PREVIEW_WIDTH / 2.0f, PREVIEW_HEIGHT / 2.0f, -PREVIEW_HEIGHT / 2.0f, -0.1, 0.1);
		glEnable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef((GLfloat)0, (GLfloat)0, 0.0f);
		glRotatef((GLfloat)0, 0, 0, 1.0f);
		glScalef((GLfloat)1.0f, (GLfloat)1.0f, (GLfloat)1.0f);
		glColor4f(1.0f, 1.0f, 1.0f, (GLfloat)1.0f);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_POLYGON);
		glTexCoord2f(0, 0); glVertex2f((GLfloat)(-PREVIEW_WIDTH / 2.0f), (GLfloat)(-PREVIEW_HEIGHT / 2.0f));
		glTexCoord2f(0, 1); glVertex2f((GLfloat)(-PREVIEW_WIDTH / 2.0f), (GLfloat)(PREVIEW_HEIGHT / 2.0f));
		glTexCoord2f(1, 1); glVertex2f((GLfloat)(PREVIEW_WIDTH / 2.0f), (GLfloat)(PREVIEW_HEIGHT / 2.0f));
		glTexCoord2f(1, 0); glVertex2f((GLfloat)(PREVIEW_WIDTH / 2.0f), (GLfloat)(-PREVIEW_HEIGHT / 2.0f));
		glEnd();
		glPopMatrix();
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_QUADS, 0, vertices);
		glBindVertexArray(0);
	}
	glUseProgram(0);
	glFlush();
	SwapBuffers(hDC);
}

inline VOID DrawGLScene(GLfloat time)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	glUniform1f(glGetUniformLocation(program, "time"), time);
	glBindVertexArray(vao);
	glDrawArrays(GL_QUADS, 0, vertices);
	glBindVertexArray(0);
	glUseProgram(0);
	glFlush();
	SwapBuffers(hDC);
}

// OpenGLの描画をAVI形式として出力
inline BOOL CreateAVI(HWND hWnd, LPCTSTR lpszOutputFilePath, DWORD dwSecond)
{
	CVideoSaver vidshot;
	int bit = 30;
	vidshot.openAVI(lpszOutputFilePath, PREVIEW_WIDTH, PREVIEW_HEIGHT, bit, TEXT("a"), "DIB ", 0, hWnd);
	for (DWORD i = 0; i < bit * dwSecond; i++)
	{
		DrawGLScene(i / 30.f);
		vidshot.saveFrame();
	}
	vidshot.closeAVI();
	return TRUE;
}

inline GLuint LoadImage(LPCTSTR lpszFilePath)
{
	GLuint texture = 0;
	Gdiplus::Bitmap bitmap(lpszFilePath);
	if (bitmap.GetLastStatus() == Gdiplus::Ok)
	{
		Gdiplus::BitmapData data;
		bitmap.LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D,
			0, GL_RGBA, data.Width, data.Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0);
		glBindTexture(GL_TEXTURE_2D, 0);
		bitmap.UnlockBits(&data);
	}
	return texture;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	static GLuint PixelFormat;
	static HWND hStatic;
	static HWND hEdit;
	static HWND hButton;
	static HFONT hFont;
	static HINSTANCE hRtLib;
	static BOOL bEditVisible = TRUE;
	static HGLRC hRC;
	switch (msg)
	{
	case WM_CREATE:
		hRtLib = LoadLibrary(TEXT("RICHED32"));
		hFont = CreateFont(24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Consolas"));
		hStatic = CreateWindow(TEXT("STATIC"), 0, WS_VISIBLE | WS_CHILD | SS_SIMPLE,
			10, 10, PREVIEW_WIDTH, PREVIEW_HEIGHT, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, 0, WS_VISIBLE | WS_CHILD | WS_HSCROLL |
			WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
			0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("AVI出力..."), WS_VISIBLE | WS_CHILD,
			PREVIEW_WIDTH / 2 - 54, PREVIEW_HEIGHT + 20, 128, 32, hWnd, (HMENU)100, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit, EM_SETTEXTMODE, TM_PLAINTEXT, 0);
		SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		if (!(hDC = GetDC(hStatic)) ||
			!(PixelFormat = ChoosePixelFormat(hDC, &pfd)) ||
			!SetPixelFormat(hDC, PixelFormat, &pfd) ||
			!(hRC = wglCreateContext(hDC)) ||
			!wglMakeCurrent(hDC, hRC) ||
			glewInit() != GLEW_OK ||
			!InitGL()) return -1;
		SetWindowText(hEdit,
			TEXT("#define pi 3.14159265358979\r\n")
			TEXT("uniform sampler2D image;\r\n")
			TEXT("uniform float time;\r\n")
			TEXT("void main()\r\n")
			TEXT("{\r\n")
			TEXT("	vec2 texCoord = vec2(gl_FragCoord.x / 512, -gl_FragCoord.y / 384);\r\n")
			TEXT("	vec4 col = texture2D(image, texCoord);\r\n")
			TEXT("	vec2 p = gl_FragCoord;\r\n")
			TEXT("	float c = 0.0;\r\n")
			TEXT("	for (float i = 0.0; i < 5.0; i++)\r\n")
			TEXT("	{\r\n")
			TEXT("		vec2 b = vec2(\r\n")
			TEXT("		sin(time + i * pi / 7) * 128 + 256,\r\n")
			TEXT("		cos(time + i * pi / 2) * 128 + 192\r\n")
			TEXT("		);\r\n")
			TEXT("		c += 16 / distance(p, b);\r\n")
			TEXT("	}\r\n")
			TEXT("	gl_FragColor = col + c;\r\n")
			TEXT("}\r\n")
			);
		DragAcceptFiles(hWnd, TRUE);
		PostMessage(hWnd, WM_CREATED, 0, 0);
		break;
	case WM_CREATED:
		SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(0, EN_CHANGE), (long)hEdit);
		SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM)(SendMessage(hEdit, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE));
		SetFocus(hEdit);
		break;
	case WM_DROPFILES:
		{
			const HDROP hDrop = (HDROP)wParam;
			TCHAR szFilePath[MAX_PATH];
			DragQueryFile(hDrop, 0, szFilePath, sizeof(szFilePath));
			LPCTSTR lpExt = PathFindExtension(szFilePath);
			if (texture)
			{
				glDeleteTextures(1, &texture);
				texture = 0;
			}
			if (
				PathMatchSpec(lpExt, TEXT("*.jpg")) ||
				PathMatchSpec(lpExt, TEXT("*.jpeg")) ||
				PathMatchSpec(lpExt, TEXT("*.gif")) ||
				PathMatchSpec(lpExt, TEXT("*.png")) ||
				PathMatchSpec(lpExt, TEXT("*.bmp")) ||
				PathMatchSpec(lpExt, TEXT("*.tiff")) ||
				PathMatchSpec(lpExt, TEXT("*.tif"))
				)
			{
				texture = LoadImage(szFilePath);
			}
			DragFinish(hDrop);
		}
		break;
	case WM_SIZE:
		MoveWindow(hEdit, PREVIEW_WIDTH + 20, 10, LOWORD(lParam) - PREVIEW_WIDTH - 30, HIWORD(lParam) - 20, 1);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 0:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				const DWORD dwSize = GetWindowTextLengthA(hEdit);
				if (!dwSize) return 0;
				LPSTR lpszText = (LPSTR)GlobalAlloc(0, (dwSize + 1)*sizeof(CHAR));
				if (!lpszText) return 0;
				GetWindowTextA(hEdit, lpszText, dwSize + 1);
				const GLuint newProgram = CreateProgram(vsrc, lpszText);
				if (newProgram)
				{
					if (program) glDeleteProgram(program);
					program = newProgram;
					SetWindowText(hWnd, TEXT("フラグメントシェーダ [コンパイル成功]"));
				}
				else
				{
					SetWindowText(hWnd, TEXT("フラグメントシェーダ [コンパイル失敗]"));
				}
				GlobalFree(lpszText);
			}
			break;
		case ID_SELECTALL:
			if (IsWindowVisible(hEdit))
			{
				SendMessage(hEdit, EM_SETSEL, 0, -1);
				SetFocus(hEdit);
			}
			break;
		case 100:
		{
			TCHAR szFileName[MAX_PATH] = { 0 };
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = TEXT("MP4(*.mp4)\0*.mp4\0すべてのファイル(*.*)\0*.*\0\0");
			ofn.lpstrFile = szFileName;
			ofn.nMaxFile = sizeof(szFileName);
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
			if (GetSaveFileName(&ofn))
			{
				CreateAVI(hStatic, szFileName, 10); // 今は固定で10秒出力
				MessageBox(hWnd, TEXT("完了しました。"), TEXT("確認"), MB_ICONINFORMATION);
			}
		}
		break;
		}
		break;
	case WM_ACTIVATE:
		active = !HIWORD(wParam);
		break;
	case WM_DESTROY:
		if (texture)
		{
			glDeleteTextures(1, &texture);
			texture = 0;
		}
		if (program) glDeleteProgram(program);
		if (vbo) glDeleteBuffers(1, &vbo);
		if (vao) glDeleteVertexArrays(1, &vao);
		if (hRC)
		{
			wglMakeCurrent(0, 0);
			wglDeleteContext(hRC);
		}
		if (hDC) ReleaseDC(hStatic, hDC);
		DestroyWindow(hEdit);
		FreeLibrary(hRtLib);
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	const WNDCLASS wndclass = { 0, WndProc, 0, 0, hInstance, 0, LoadCursor(0, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), 0, szClassName };
	RegisterClass(&wndclass);
	const HWND hWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY | FCONTROL, 'A', ID_SELECTALL } };
	const HACCEL hAccel = CreateAcceleratorTable(Accel, sizeof(Accel) / sizeof(ACCEL));
	BOOL done = 0;
	while (!done)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = TRUE;
			}
			else if (!TranslateAccelerator(hWnd, hAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (active)
		{
			DrawGLScene();
		}
	}
	DestroyAcceleratorTable(hAccel);
	Gdiplus::GdiplusShutdown(gdiToken);
	return msg.wParam;
}
