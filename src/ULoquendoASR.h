/*
 * Kinect module based on OpenNI for URBI
 * Copyright (C) 2012  Lukasz Malek
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 * File:   UEigenfaces.h
 * Author: lmalek
 *
 * Created on 21 marzec 2012, 09:55
 */

#ifndef UEIGENFACES_H
#define	UEIGENFACES_H

#include <urbi/uobject.hh>
#include <boost/thread.hpp>
#include <cstdio>
#include <LoqASR.h>


#define LASRX_EX_EVENTS_LOG 0

/******************************************************************************/
/* Defines that must be changed by the user of this program                   */
/******************************************************************************/

/* Initialization file                                                        */
#define LASRX_EX_SESSION_FILE NULL

/* Source file in recognition from file                                       */
#define LASRX_EX_INIINFOBUFFER "[languages]\n\"load\" = \"en-us\"\n"

/* Audio mode                                                                 */
#define LASRX_EX_AUDIO_MODE LASRX_SAMPLES_LIN16

/* Type of audio source: file or multimedia                                   */
#define LASRX_EX_USE_MM_AUDIO_SOURCE

/* Source grammar and active rule (NULL = root rule)                          */
//#define LASRX_EX_GRAMMAR_SOURCE "./Grammars/simple.grxml"
#define LASRX_EX_GRAMMAR_SOURCE "./simple.grxml"
#define LASRX_EX_RO_RULE NULL

/* Recognition object name                                                    */
#define LASRX_EX_RO_NAME "dynamic"

/******************************************************************************/
/* OS dependent definitions                                                   */
/******************************************************************************/
#if defined WIN32
#include <windows.h>
#define strcasecmp _stricmp
#define LASRX_EX_DLL_LOAD(szDll)         LoadLibrary(szDll)
#define LASRX_EX_DLL_FREE(hDll)          FreeLibrary(hDll)
#define LASRX_EX_DLL_SYM(hDll, pszSym)   GetProcAddress(hDll, pszSym)
typedef HMODULE LASRX_EX_DLL_HND;
#elif defined linux
#include <dlfcn.h>
#include <semaphore.h>
#include <errno.h>
#include <stdarg.h>
#define LASRX_EX_DLL_LOAD(szDll)         dlopen(szDll, RTLD_LAZY | RTLD_GLOBAL)
#define LASRX_EX_DLL_FREE(hDll)          dlclose(hDll)
#define LASRX_EX_DLL_SYM(hDll, pszSym)   dlsym(hDll, pszSym)
typedef void* LASRX_EX_DLL_HND;
#else
#error "Loquendo ASR Recognition example supports only WIN32 and Linux platforms. Check the definition of symbol WIN32 or linux"
#endif

/******************************************************************************/
/* Audio source DLLs definitions                                              */
/******************************************************************************/
#ifdef WIN32
typedef int (__stdcall *LASRX_EX_AUDIO_NEW_HANDLE)(int nFormat, void **hAudio);
typedef int (__stdcall *LASRX_EX_AUDIO_DELETE_HANDLE)(void *hAudio);
typedef int (__stdcall *LASRX_EX_AUDIO_SET_DATA)(void *hAudio, char *szData);
typedef int (__stdcall *LASRX_EX_AUDIO_REGISTER)(void *hAudio, void *hInstance);
typedef int (__stdcall *LASRX_EX_AUDIO_UNREGISTER)(void *hAudio);
#else
typedef int (*LASRX_EX_AUDIO_NEW_HANDLE)(int nFormat, void **hAudio);
typedef int (*LASRX_EX_AUDIO_DELETE_HANDLE)(void *hAudio);
typedef int (*LASRX_EX_AUDIO_SET_DATA)(void *hAudio, char *szData);
typedef int (*LASRX_EX_AUDIO_REGISTER)(void *hAudio, void *hInstance);
typedef int (*LASRX_EX_AUDIO_UNREGISTER)(void *hAudio);
#endif
#define LASRX_EX_AUDIO_NEW_FUNCTION "NewHandle"
#define LASRX_EX_AUDIO_DELETE_FUNCTION "DeleteHandle"
#define LASRX_EX_AUDIO_SET_DATA_FUNCTION "SetData"
#define LASRX_EX_AUDIO_REGISTER_FUNCTION "Register"
#define LASRX_EX_AUDIO_UNREGISTER_FUNCTION "Unregister"

typedef struct {
    LASRX_EX_DLL_HND hAudioSourceDll;
    LASRX_EX_AUDIO_NEW_HANDLE pfNew;
    LASRX_EX_AUDIO_DELETE_HANDLE pfDelete;
    LASRX_EX_AUDIO_SET_DATA pfSetData;
    LASRX_EX_AUDIO_REGISTER pfRegister;
    LASRX_EX_AUDIO_UNREGISTER pfUnregister;
} LASRX_EX_DLL_POINTERS;

/******************************************************************************/
/* Definition related to the audio source                                     */
/******************************************************************************/
#ifdef WIN32
#if defined LASRX_EX_USE_FILE_AUDIO_SOURCE
#define LASRX_EX_AUDIO_SOURCE_LIBRARY "LoqASRAudioFile"
#undef LASRX_EX_USE_MM_AUDIO_SOURCE
#elif defined LASRX_EX_USE_MM_AUDIO_SOURCE
#define LASRX_EX_AUDIO_SOURCE_LIBRARY "LoqASRAudioMM"
#else
#error "LASRX_EX_USE_FILE_AUDIO_SOURCE or LASRX_EX_USE_MM_AUDIO_SOURCE must be defined"
#endif
#else
#if defined LASRX_EX_USE_FILE_AUDIO_SOURCE
#define LASRX_EX_AUDIO_SOURCE_LIBRARY "libLoqASRAudioFile.so"
#undef LASRX_EX_USE_MM_AUDIO_SOURCE
#elif defined LASRX_EX_USE_MM_AUDIO_SOURCE
//#define LASRX_EX_AUDIO_SOURCE_LIBRARY "libLoqASRAudioMM.so"
//#define LASRX_EX_AUDIO_SOURCE_LIBRARY "libLoqASRAudioAudio.so"
#define LASRX_EX_AUDIO_SOURCE_LIBRARY "libLoqASRAudioALSA.so"
#else
#error "LASRX_EX_USE_FILE_AUDIO_SOURCE or LASRX_EX_USE_MM_AUDIO_SOURCE must be defined"
#endif
#endif

class ULoquendoASR : public urbi::UObject {
public:
    ULoquendoASR(const std::string& name);
    virtual ~ULoquendoASR();

    /** 
     * Initialization function
     * Responsible for initalization
     */
    void init(std::string grammar);

    urbi::UEvent newRecognizedText;
    
    std::string recognizedText;
    double recognizeTextConfidence;

    // Thread object
    boost::thread recognizeTextThread;
    // Thread function to grab image
    void recognizeTextThreadFunction();
    
    urbi::UVar moduleReady;

private:
    int ReportError(char *szFunction, char *szFormat, ...);
    int ReportLasrxError(char *szFunction, void *hHandle, char *szLasrxFunction, int nRetCode);
    int LoadAudioSource(lasrxHandleType hInstance, char *szAudioSource, LASRX_EX_DLL_POINTERS *pDll, void **phAudio);
    int GetRecognitionResults(lasrxHandleType hInstance, int nMaxHypos);
    int BuildRO(lasrxHandleType hInstance, char *szSource, char *szName);
    int DeleteRO(lasrxHandleType hInstance, char *szName);
    int Recog(void *hInstance, char *szName, void *eszRule, LASRX_EX_DLL_POINTERS *pDll, void *hAudio);

    char szInputBuffer[128];
    /* DLL pointers                                                           */
    LASRX_EX_DLL_POINTERS tDll;
    /* Audio channel                                                          */
    void *hAudio;
    /* Loquendo ASR handles                                                   */
    lasrxHandleType hSession, hInstance;
    lasrxResultType nRetCode;
    lasrxStringType szVersionString;
    lasrxIntType nVersionInt;
    lasrxStringType szSystemInfo;
    lasrxStringType szId;

};

#endif	/* UEIGENFACES_H */

