// ImGui Marmalade binding with IwGx
// Copyright (C) 2015 by Giovanni Zito
// This file is part of ImGui
// https://github.com/ocornut/imgui

#include <imgui.h>
#include "imgui_impl_marmalade.h"

#include <s3eClipboard.h>
#include <s3ePointer.h> 
#include <s3eKeyboard.h>
#include <IwTexture.h>
#include <IwGx.h>

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static CIwTexture*  g_FontTexture = 0;
static int			  g_KeyboardGetCharAvailable = 0 ;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
void ImGui_Marmalade_RenderDrawLists(ImDrawData* draw_data)
{
	// Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
	ImGuiIO& io = ImGui::GetIO();
	float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// Render command lists
	for(int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
		const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
		int nVert = cmd_list->VtxBuffer.size() ;
		CIwFVec2* pVertStream = IW_GX_ALLOC(CIwFVec2,nVert) ;
		CIwFVec2* pUVStream = IW_GX_ALLOC(CIwFVec2,nVert) ;
		CIwColour* pColStream = IW_GX_ALLOC(CIwColour,nVert) ;

		for( int i=0; i < nVert; i++ ) {
			pVertStream[i].x = cmd_list->VtxBuffer[i].pos.x ;
			pVertStream[i].y = cmd_list->VtxBuffer[i].pos.y ;
			pUVStream[i].x = cmd_list->VtxBuffer[i].uv.x ;
			pUVStream[i].y = cmd_list->VtxBuffer[i].uv.y ;
			pColStream[i] = cmd_list->VtxBuffer[i].col ;
		}

		IwGxSetVertStreamScreenSpace(pVertStream,nVert) ;
		IwGxSetUVStream(pUVStream) ;
		IwGxSetColStream(pColStream,nVert) ;
		IwGxSetNormStream(0) ;

		for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if(pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list,pcmd);
			}
			else
			{
				CIwMaterial* pCurrentMaterial = IW_GX_ALLOC_MATERIAL() ;
				pCurrentMaterial->SetShadeMode(CIwMaterial::SHADE_FLAT) ;
				pCurrentMaterial->SetCullMode(CIwMaterial::CULL_NONE) ;
				pCurrentMaterial->SetFiltering(false) ;
				pCurrentMaterial->SetAlphaMode(CIwMaterial::ALPHA_BLEND) ;
				pCurrentMaterial->SetDepthWriteMode(CIwMaterial::DEPTH_WRITE_NORMAL);
				pCurrentMaterial->SetAlphaTestMode(CIwMaterial::ALPHATEST_DISABLED);
				pCurrentMaterial->SetTexture((CIwTexture*)pcmd->TextureId) ;
				IwGxSetMaterial(pCurrentMaterial) ;

				IwGxDrawPrims(IW_GX_TRI_LIST,(uint16*)idx_buffer,pcmd->ElemCount) ;
			}
			idx_buffer += pcmd->ElemCount;
		}
		IwGxFlush() ;
	}

	// TODO restore modified state (i.e. mvp matrix)
}


static const char* ImGui_Marmalade_GetClipboardText()
{
	static char clipBuf[512] ;
	if(s3eClipboardAvailable()) {
		s3eClipboardGetText( clipBuf, 512 );
	}
	else {
		clipBuf[0] = '\0' ;
	}

	return clipBuf ;
}

static void ImGui_Marmalade_SetClipboardText(const char* text)
{
	if( s3eClipboardAvailable() ) {
		s3eClipboardSetText(text);
	}
}

int32 ImGui_Marmalade_PointerButtonEventCallback(void* SystemData,void* pUserData)
{
	// pEvent->m_Button is of type s3ePointerButton and indicates which mouse
	// button was pressed.  For touchscreens this should always have the value
	// S3E_POINTER_BUTTON_SELECT
	s3ePointerEvent* pEvent = (s3ePointerEvent*)SystemData ;

	if(pEvent->m_Pressed==1) {
		if(pEvent->m_Button == S3E_POINTER_BUTTON_LEFTMOUSE) {
			g_MousePressed[0] = true;
		}
		if(pEvent->m_Button == S3E_POINTER_BUTTON_RIGHTMOUSE) {
			g_MousePressed[1] = true;
		}
		if(pEvent->m_Button == S3E_POINTER_BUTTON_MIDDLEMOUSE) {
			g_MousePressed[2] = true;
		}
		if(pEvent->m_Button == S3E_POINTER_BUTTON_MOUSEWHEELUP) {
			g_MouseWheel += pEvent->m_y ;
		}
		if(pEvent->m_Button == S3E_POINTER_BUTTON_MOUSEWHEELDOWN) {
			g_MouseWheel += pEvent->m_y ;
		}
	}

	return 0;
}

//void ImGui_Marmalade_ScrollCallback(GLFWwindow*,double /*xoffset*/,double yoffset)
//{
//	g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
//}

int32 ImGui_Marmalade_KeyCallback(void* SystemData, void* userData)
{
	ImGuiIO& io = ImGui::GetIO();
	s3eKeyboardEvent* e = (s3eKeyboardEvent*)SystemData ;
	if( e->m_Pressed == 1 ) {
		io.KeysDown[e->m_Key] = true;
	}
	if(e->m_Pressed == 0) {
		io.KeysDown[e->m_Key] = false;
	}
	
	io.KeyCtrl = s3eKeyboardGetState(s3eKeyLeftControl) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightControl) == S3E_KEY_STATE_DOWN ;
	io.KeyShift = s3eKeyboardGetState(s3eKeyLeftShift) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightShift) == S3E_KEY_STATE_DOWN ;
	io.KeyAlt = s3eKeyboardGetState(s3eKeyLeftAlt) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightAlt) == S3E_KEY_STATE_DOWN ;

	return 0 ;
}

int32 ImGui_Marmalade_CharCallback(void* SystemData,void* userData)
{
	ImGuiIO& io = ImGui::GetIO();

	s3eKeyboardCharEvent* e = (s3eKeyboardCharEvent*)SystemData ;
	if( (e->m_Char > 0 && e->m_Char < 0x10000) && io.WantTextInput ) {
		io.AddInputCharacter((unsigned short)e->m_Char);
	}

	return 0 ;
}

bool ImGui_Marmalade_CreateDeviceObjects()
{
    ImGuiIO& io = ImGui::GetIO();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture
	 g_FontTexture = new CIwTexture() ;
	 g_FontTexture->SetModifiable(true) ;
	 CIwImage& image = g_FontTexture->GetImage() ;
	 image.SetFormat(CIwImage::Format::ARGB_8888) ;
	 image.SetWidth(width) ;
	 image.SetHeight(height) ;
	 image.SetBuffers();									// allocates and own buffers
	 image.ReadTexels(pixels) ;
	 g_FontTexture->SetMipMapping(false) ;
	 g_FontTexture->SetFiltering(false) ;
	 g_FontTexture->Upload() ;

    // Store the pointer
    io.Fonts->TexID = (void *)g_FontTexture;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    return true;
}

void    ImGui_Marmalade_InvalidateDeviceObjects()
{
    if (g_FontTexture)
    {
        delete g_FontTexture ;
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool    ImGui_Marmalade_Init( bool install_callbacks)
{
	IwGxInit() ;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = s3eKeyTab;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = s3eKeyLeft;
    io.KeyMap[ImGuiKey_RightArrow] = s3eKeyRight;
    io.KeyMap[ImGuiKey_UpArrow] = s3eKeyUp;
    io.KeyMap[ImGuiKey_DownArrow] = s3eKeyDown;
    io.KeyMap[ImGuiKey_PageUp] = s3eKeyPageUp;
    io.KeyMap[ImGuiKey_PageDown] = s3eKeyPageDown;
    io.KeyMap[ImGuiKey_Home] = s3eKeyHome;
    io.KeyMap[ImGuiKey_End] = s3eKeyEnd;
    io.KeyMap[ImGuiKey_Delete] = s3eKeyDelete;
    io.KeyMap[ImGuiKey_Backspace] = s3eKeyBackspace;
    io.KeyMap[ImGuiKey_Enter] = s3eKeyEnter;
    io.KeyMap[ImGuiKey_Escape] = s3eKeyEsc;
    io.KeyMap[ImGuiKey_A] = s3eKeyA;
    io.KeyMap[ImGuiKey_C] = s3eKeyC;
    io.KeyMap[ImGuiKey_V] = s3eKeyV;
    io.KeyMap[ImGuiKey_X] = s3eKeyX;
    io.KeyMap[ImGuiKey_Y] = s3eKeyY;
    io.KeyMap[ImGuiKey_Z] = s3eKeyZ;

    io.RenderDrawListsFn = ImGui_Marmalade_RenderDrawLists;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_Marmalade_SetClipboardText;
    io.GetClipboardTextFn = ImGui_Marmalade_GetClipboardText;

    if (install_callbacks)
    {
		s3ePointerRegister(S3E_POINTER_BUTTON_EVENT,ImGui_Marmalade_PointerButtonEventCallback,0) ;
 		s3eKeyboardRegister(S3E_KEYBOARD_KEY_EVENT,ImGui_Marmalade_KeyCallback,0) ;
		
		// enable KEYBOARD_GET_CHAR
		s3eKeyboardSetInt(S3E_KEYBOARD_GET_CHAR,1) ;
		g_KeyboardGetCharAvailable = s3eKeyboardGetInt(S3E_KEYBOARD_GET_CHAR) ;
		if( g_KeyboardGetCharAvailable ) {
			s3eKeyboardRegister(S3E_KEYBOARD_CHAR_EVENT,ImGui_Marmalade_CharCallback,0) ;
		}
	 }

    return true;
}

void ImGui_Marmalade_Shutdown()
{
    ImGui_Marmalade_InvalidateDeviceObjects();
    ImGui::Shutdown();
	 IwGxTerminate();
}

void ImGui_Marmalade_NewFrame()
{
    if (!g_FontTexture)
        ImGui_Marmalade_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w = IwGxGetScreenWidth(), h = IwGxGetScreenHeight() ;
    io.DisplaySize = ImVec2((float)w, (float)h);
	 // For retina display or other situations where window coordinates are different from framebuffer coordinates. User storage only, presently not used by ImGui.
    io.DisplayFramebufferScale = ImVec2((float)1.0f, (float)1.0f);

    // Setup time step
    double current_time =  s3eTimerGetUST() / 1000.0f ;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    //if ( gui_has_focus() )
    //{
    	double mouse_x, mouse_y;
		mouse_x = s3ePointerGetX();
		mouse_y = s3ePointerGetY();
    	io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    //}
    //else
    //{
    //	io.MousePos = ImVec2(-1,-1);
    //}
   
    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || s3ePointerGetState((s3ePointerButton)i) != S3E_POINTER_STATE_UP;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
//	 s3ePointerSetInt(S3E_POINTER_HIDE_CURSOR,(io.MouseDrawCursor ? 0 : 1));

    // Start the frame
    ImGui::NewFrame();
}
