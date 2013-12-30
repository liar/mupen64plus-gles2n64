
#include <string.h>
#include "winlnxdefs.h"
#include "m64p_types.h"
#include "m64p_plugin.h"
#include "gles2N64.h"
#include "Debug.h"
#include "OpenGL.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "VI.h"
#include "Config.h"
#include "Textures.h"
#include "ShaderCombiner.h"

#include <dlfcn.h>

#include "m64p_types.h"
#include "m64p_plugin.h"

HWND        hWnd = 0;
char        *screenDirectory;
u32         last_good_ucode = (u32) -1;
void        (*CheckInterrupts)( void );
char        configdir[PATH_MAX] = {0};
void        (*renderCallback)() = NULL;

ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = NULL;

ptr_VidExt_Init                  CoreVideo_Init = NULL;
ptr_VidExt_Quit                  CoreVideo_Quit = NULL;
ptr_VidExt_ListFullscreenModes   CoreVideo_ListFullscreenModes = NULL;
ptr_VidExt_SetVideoMode          CoreVideo_SetVideoMode = NULL;
ptr_VidExt_SetCaption            CoreVideo_SetCaption = NULL;
ptr_VidExt_ToggleFullScreen      CoreVideo_ToggleFullScreen = NULL;
ptr_VidExt_GL_GetProcAddress     CoreVideo_GL_GetProcAddress = NULL;
ptr_VidExt_GL_SetAttribute       CoreVideo_GL_SetAttribute = NULL;
ptr_VidExt_GL_GetAttribute       CoreVideo_GL_GetAttribute = NULL;
ptr_VidExt_GL_SwapBuffers        CoreVideo_GL_SwapBuffers = NULL;

extern "C" {

EXPORT void CALL DllAbout ( HWND hParent )
{
    puts( "gles2N64 by Adventus / Orkin\nThanks to Clements, Rice, Gonetz, Malcolm, Dave2001, cryhlove, icepir8, zilmar, Azimer, and StrmnNrmn\nported by blight" );
}

EXPORT void CALL DllConfig ( HWND hParent )
{
    Config_DoConfig(hParent);
}

EXPORT void CALL DllTest ( HWND hParent )
{
}

EXPORT void CALL DrawScreen (void)
{

}

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle,
        void *Context, void (*DebugCallback)(void *, int, const char *))
{
    ConfigGetSharedDataFilepath = (ptr_ConfigGetSharedDataFilepath)
            dlsym(CoreLibHandle, "ConfigGetSharedDataFilepath");

    CoreVideo_Init = (ptr_VidExt_Init) dlsym(CoreLibHandle, "VidExt_Init");
    CoreVideo_Quit = (ptr_VidExt_Quit) dlsym(CoreLibHandle, "VidExt_Quit");
    CoreVideo_ListFullscreenModes = (ptr_VidExt_ListFullscreenModes) dlsym(CoreLibHandle, "VidExt_ListFullscreenModes");
    CoreVideo_SetVideoMode = (ptr_VidExt_SetVideoMode) dlsym(CoreLibHandle, "VidExt_SetVideoMode");
    CoreVideo_SetCaption = (ptr_VidExt_SetCaption) dlsym(CoreLibHandle, "VidExt_SetCaption");
    CoreVideo_ToggleFullScreen = (ptr_VidExt_ToggleFullScreen) dlsym(CoreLibHandle, "VidExt_ToggleFullScreen");
    CoreVideo_GL_GetProcAddress = (ptr_VidExt_GL_GetProcAddress) dlsym(CoreLibHandle, "VidExt_GL_GetProcAddress");
    CoreVideo_GL_SetAttribute = (ptr_VidExt_GL_SetAttribute) dlsym(CoreLibHandle, "VidExt_GL_SetAttribute");
    CoreVideo_GL_GetAttribute = (ptr_VidExt_GL_GetAttribute) dlsym(CoreLibHandle, "VidExt_GL_GetAttribute");
    CoreVideo_GL_SwapBuffers = (ptr_VidExt_GL_SwapBuffers) dlsym(CoreLibHandle, "VidExt_GL_SwapBuffers");
    if (!CoreVideo_Init || !CoreVideo_Quit || !CoreVideo_ListFullscreenModes || !CoreVideo_SetVideoMode |   !CoreVideo_SetCaption || !CoreVideo_ToggleFullScreen || !CoreVideo_GL_GetProcAddress ||
                !CoreVideo_GL_SetAttribute || !CoreVideo_GL_GetAttribute || !CoreVideo_GL_SwapBuffers)
    {
        //DebugMessage(M64MSG_ERROR,"Error initializing vidext function pointers");
    }

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType,
        int *PluginVersion, int *APIVersion, const char **PluginNamePtr,
        int *Capabilities)
{
    /* set version info */
    if (PluginType != NULL)
        *PluginType = M64PLUGIN_GFX;

    if (PluginVersion != NULL)
        *PluginVersion = PLUGIN_VERSION;

    if (APIVersion != NULL)
        *APIVersion = PLUGIN_API_VERSION;
    
    if (PluginNamePtr != NULL)
        *PluginNamePtr = PLUGIN_NAME;

    if (Capabilities != NULL)
    {
        *Capabilities = 0;
    }
                    
    return M64ERR_SUCCESS;
}

EXPORT void CALL ChangeWindow (void)
{
}


EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{

    DMEM = Gfx_Info.DMEM;
    IMEM = Gfx_Info.IMEM;
    RDRAM = Gfx_Info.RDRAM;

    REG.MI_INTR = (u32*) Gfx_Info.MI_INTR_REG;
    REG.DPC_START = (u32*) Gfx_Info.DPC_START_REG;
    REG.DPC_END = (u32*) Gfx_Info.DPC_END_REG;
    REG.DPC_CURRENT = (u32*) Gfx_Info.DPC_CURRENT_REG;
    REG.DPC_STATUS = (u32*) Gfx_Info.DPC_STATUS_REG;
    REG.DPC_CLOCK = (u32*) Gfx_Info.DPC_CLOCK_REG;
    REG.DPC_BUFBUSY = (u32*) Gfx_Info.DPC_BUFBUSY_REG;
    REG.DPC_PIPEBUSY = (u32*) Gfx_Info.DPC_PIPEBUSY_REG;
    REG.DPC_TMEM = (u32*) Gfx_Info.DPC_TMEM_REG;

    REG.VI_STATUS = (u32*) Gfx_Info.VI_STATUS_REG;
    REG.VI_ORIGIN = (u32*) Gfx_Info.VI_ORIGIN_REG;
    REG.VI_WIDTH = (u32*) Gfx_Info.VI_WIDTH_REG;
    REG.VI_INTR = (u32*) Gfx_Info.VI_INTR_REG;
    REG.VI_V_CURRENT_LINE = (u32*) Gfx_Info.VI_V_CURRENT_LINE_REG;
    REG.VI_TIMING = (u32*) Gfx_Info.VI_TIMING_REG;
    REG.VI_V_SYNC = (u32*) Gfx_Info.VI_V_SYNC_REG;
    REG.VI_H_SYNC = (u32*) Gfx_Info.VI_H_SYNC_REG;
    REG.VI_LEAP = (u32*) Gfx_Info.VI_LEAP_REG;
    REG.VI_H_START = (u32*) Gfx_Info.VI_H_START_REG;
    REG.VI_V_START = (u32*) Gfx_Info.VI_V_START_REG;
    REG.VI_V_BURST = (u32*) Gfx_Info.VI_V_BURST_REG;
    REG.VI_X_SCALE = (u32*) Gfx_Info.VI_X_SCALE_REG;
    REG.VI_Y_SCALE = (u32*) Gfx_Info.VI_Y_SCALE_REG;

    CheckInterrupts = Gfx_Info.CheckInterrupts;

    Config_LoadConfig();
    Config_LoadRomConfig(Gfx_Info.HEADER);

    OGL_Start();

    return TRUE;
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
}


EXPORT void CALL ProcessDList(void)
{
    OGL.frame_dl++;

    if (config.autoFrameSkip)
    {
        OGL_UpdateFrameTime();

        if (OGL.consecutiveSkips < 1)
        {
            unsigned t = 0;
            for(int i = 0; i < OGL_FRAMETIME_NUM; i++) t += OGL.frameTime[i];
            t *= config.targetFPS;
            if (config.romPAL) t = (t * 5) / 6;
            if (t > (OGL_FRAMETIME_NUM * 1000))
            {
                OGL.consecutiveSkips++;
                OGL.frameSkipped++;
                RSP.busy = FALSE;
                RSP.DList++;
                return;
            }
        }
    }
    else if ((OGL.frame_vsync % config.frameRenderRate) != 0)
    {
        OGL.frameSkipped++;
        RSP.busy = FALSE;
        RSP.DList++;
        return;
    }

    OGL.consecutiveSkips = 0;
    RSP_ProcessDList();
    OGL.mustRenderDlist = true;
}

EXPORT void CALL ProcessRDPList(void)
{

}

EXPORT void CALL RomClosed (void)
{
    OGL_Stop();
}

EXPORT void CALL RomOpen (void)
{
    RSP_Init();
    OGL.frame_vsync = 0;
    OGL.frame_dl = 0;
    OGL.frame_prevdl = -1;
    OGL.mustRenderDlist = false;
}

EXPORT void CALL RomResumed(void)
{
    //frameSkipper.start();
}

EXPORT void CALL ResizeVideoOutput(int Width, int Height)
{
}

/******************************************************************
  Function: FrameBufferRead
  Purpose:  This function is called to notify the dll that the
            frame buffer memory is beening read at the given address.
            DLL should copy content from its render buffer to the frame buffer
            in N64 RDRAM
            DLL is responsible to maintain its own frame buffer memory addr list
            DLL should copy 4KB block content back to RDRAM frame buffer.
            Emulator should not call this function again if other memory
            is read within the same 4KB range

            Since depth buffer is also being watched, the reported addr
            may belong to depth buffer
  input:    addr        rdram address
            val         val
            size        1 = uint8, 2 = uint16, 4 = uint32
  output:   none
*******************************************************************/ 

EXPORT void CALL FBRead(u32 addr)
{
}

/******************************************************************
  Function: FrameBufferWrite
  Purpose:  This function is called to notify the dll that the
            frame buffer has been modified by CPU at the given address.

            Since depth buffer is also being watched, the reported addr
            may belong to depth buffer

  input:    addr        rdram address
            val         val
            size        1 = uint8, 2 = uint16, 4 = uint32
  output:   none
*******************************************************************/ 

EXPORT void CALL FBWrite(u32 addr, u32 size)
{
}

/************************************************************************
Function: FBGetFrameBufferInfo
Purpose:  This function is called by the emulator core to retrieve frame
          buffer information from the video plugin in order to be able
          to notify the video plugin about CPU frame buffer read/write
          operations

          size:
            = 1     byte
            = 2     word (16 bit) <-- this is N64 default depth buffer format
            = 4     dword (32 bit)

          when frame buffer information is not available yet, set all values
          in the FrameBufferInfo structure to 0

input:    FrameBufferInfo pinfo[6]
          pinfo is pointed to a FrameBufferInfo structure which to be
          filled in by this function
output:   Values are return in the FrameBufferInfo structure
          Plugin can return up to 6 frame buffer info
 ************************************************************************/

EXPORT void CALL FBGetFrameBufferInfo(void *p)
{
}

// paulscode, API changed this to "ReadScreen2" in Mupen64Plus 1.99.4
EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
/* TODO: 'int front' was added in 1.99.4.  What to do with this here? */
    OGL_ReadScreen(dest, width, height);
}

EXPORT void CALL SetRenderingCallback(void (*callback)())
{
    renderCallback = callback;
}

EXPORT void CALL SetFrameSkipping(bool autoSkip, int maxSkips)
{
 /*   frameSkipper.setSkips(
            autoSkip ? FrameSkipper::AUTO : FrameSkipper::MANUAL,
            maxSkips);*/
}

EXPORT void CALL ShowCFB (void)
{
}

EXPORT void CALL SetStretchVideo(bool stretch)
{
    //config.stretchVideo = stretch;
}

EXPORT void CALL StartGL()
{
	printf("Hit StartGL\n");
    OGL_Start();
}

EXPORT void CALL StopGL()
{
    OGL_Stop();
}

EXPORT void CALL UpdateScreen (void)
{
    //has there been any display lists since last update
    if (OGL.frame_prevdl == OGL.frame_dl) return;

    OGL.frame_prevdl = OGL.frame_dl;

    if (OGL.frame_dl > 0) OGL.frame_vsync++;

    if (OGL.mustRenderDlist)
    {
        OGL.screenUpdate=true;
        VI_UpdateScreen();
        OGL.mustRenderDlist = false;
    }
}

EXPORT void CALL ViStatusChanged (void)
{
}

EXPORT void CALL ViWidthChanged (void)
{
}

EXPORT void CALL ReadScreen (void **dest, int *width, int *height)
{
    OGL_ReadScreen( dest[0], width, height );
}

EXPORT void CALL SetConfigDir (char *configDir)
{
    strncpy(configdir, configDir, PATH_MAX);
}


} // extern "C"
