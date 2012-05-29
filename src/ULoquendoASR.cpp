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
 * File:   ULoquendoASR.cpp
 * Author: lmalek
 * 
 * Created on 21 marzec 2012, 09:55
 */

#include "ULoquendoASR.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <fstream>


using namespace std;
using namespace urbi;
using namespace boost;

ULoquendoASR::ULoquendoASR(const std::string& name) : UObject(name) {
    cerr << "[ULoquendoASR]::ULoquendoASR()" << endl;
    UBindFunction(ULoquendoASR, init);
}

ULoquendoASR::~ULoquendoASR() {
    cerr << "[ULoquendoASR]::~ULoquendoASR()" << endl;

    lasrxDeleteInstance(hInstance);
    lasrxDeleteSession(hSession);
}

void ULoquendoASR::init(std::string grammar) {
    cerr << "[ULoquendoASR]::init()" << endl;
    
    if (grammar.empty())
        grammar = LASRX_EX_GRAMMAR_SOURCE;

    // Bind all functions
    UBindVar(ULoquendoASR, moduleReady);
    UBindEvent(ULoquendoASR, newRecognizedText);

    moduleReady = false;

    nRetCode = LASRX_RETCODE_OK;
    szVersionString = NULL;


    /* Creates a session */
    if ((nRetCode = lasrxNewSession(LASRX_EX_SESSION_FILE, LASRX_EX_INIINFOBUFFER, &hSession)) != LASRX_RETCODE_OK) {
        ReportLasrxError("main", NULL, "lasrxNewSession", nRetCode);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to initialize new session");
    }
    cerr << "[ULoquendoASR]::init() - Created session "<< hex << hSession << endl;
    /* Set audio mode                                                         */
    if ((nRetCode = lasrxSetAudioMode(hSession, LASRX_EX_AUDIO_MODE)) != LASRX_RETCODE_OK) {
        ReportLasrxError("main", hSession, "lasrxSetAudioMode", nRetCode);
        lasrxDeleteSession(hSession);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to initialize set audio mode");
    }
    if ((nRetCode = lasrxNewInstance(hSession, NULL, LASRX_FALSE, &hInstance)) != LASRX_RETCODE_OK) {
        ReportLasrxError("main", hSession, "lasrxNewInstance", nRetCode);
        lasrxDeleteSession(hSession);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to initialize new instance");
    }
    if ((nRetCode = lasrxGetInstanceId(hInstance, &szId)) != LASRX_RETCODE_OK) {
        ReportLasrxError("main", hInstance, "lasrxGetInstanceId", nRetCode);
        lasrxDeleteInstance(hInstance);
        lasrxDeleteSession(hSession);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to getinstance id");
    }
    cerr << "[ULoquendoASR]::init() - Created instance " << hex << hInstance << dec << szId << endl;
    lasrxFree(szId);
    if ((nRetCode = BuildRO(hInstance, LASRX_EX_GRAMMAR_SOURCE, LASRX_EX_RO_NAME)) != 0) {
        ReportError("main", "ERROR building recognition object\n");
        lasrxDeleteInstance(hInstance);
        lasrxDeleteSession(hSession);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to build RO grammar");
    }
    if ((nRetCode = LoadAudioSource(hInstance, LASRX_EX_AUDIO_SOURCE_LIBRARY, &tDll, &hAudio)) != 0) {
        ReportError("main", "ERROR setting audio source\n");
        lasrxDeleteInstance(hInstance);
        lasrxDeleteSession(hSession);
        throw std::runtime_error("[ULoquendoASR]::init() : Failed to load audio source");
    }

    recognizeTextThread = boost::thread(&ULoquendoASR::recognizeTextThreadFunction, this);
}

void ULoquendoASR::recognizeTextThreadFunction() {
    moduleReady = true;
    cerr << "[ULoquendoASR]::recognizeTextThreadFunction()" << endl;
    try {
        while (true) {
            this_thread::interruption_point();
//            fprintf(stdout, "\n--- waiting for audio ---\n");
            if ((nRetCode = Recog(hInstance, LASRX_EX_RO_NAME, LASRX_EX_RO_RULE, &tDll, hAudio)) != 0) {
                ReportError("main", "ERROR in recognition\n");
                lasrxDeleteInstance(hInstance);
                lasrxDeleteSession(hSession);
                throw std::runtime_error("[ULoquendoASR]::recognizeTextThreadFunction() : Error in recognition");
            }

        }
    } catch (boost::thread_interrupted&) {
        moduleReady = false;
        cerr << "[ULoquendoASR]::recognizeTextThreadFunction() - stopped" << endl;
        return;
    }
}

/******************************************************************************/
/* Error reporting                                                            */

/******************************************************************************/
int ULoquendoASR::ReportError(char *szFunction, char *szFormat, ...) {
    va_list pArgs;
    va_start(pArgs, szFormat);
    fprintf(stderr, "\nFunction %s - ", szFunction);
    vfprintf(stderr, szFormat, pArgs);
    va_end(pArgs);
    return 0;
}

int ULoquendoASR::ReportLasrxError(char *szFunction, void *hHandle, char *szLasrxFunction, int nRetCode) {
    char *szErrorMessage;
    char *szApiBuffer;
    lasrxGetErrorMessage(hHandle, &szErrorMessage);
    lasrxLogGetApiBuffer(hHandle, &szApiBuffer);
    ReportError(szFunction, "ERROR: Loquendo ASR function \"%s\" failed with code %d:\n === Error message ===\n%s\n === Log Buffer ===\n%s\n", szLasrxFunction, nRetCode, szErrorMessage == NULL ? "(null)" : szErrorMessage, szApiBuffer == NULL ? "(null)" : szApiBuffer);
    lasrxFree(szErrorMessage);
    lasrxFree(szApiBuffer);
    return 0;
}

/******************************************************************************/
/* Load and connect to audio source                                           */

/******************************************************************************/
int ULoquendoASR::LoadAudioSource(lasrxHandleType hInstance, char *szAudioSource, LASRX_EX_DLL_POINTERS *pDll, void **phAudio) {
    lasrxResultType nRetCode = LASRX_RETCODE_OK;
    lasrxHandleType hSession;
    lasrxIntType nAudioMode;
    if ((pDll->hAudioSourceDll = LASRX_EX_DLL_LOAD(szAudioSource)) == NULL) {
        ReportError("LoadAudioSource", "Error loading library \"%s\"\n", szAudioSource);
        return -1;
    }
    if ((pDll->pfNew = (LASRX_EX_AUDIO_NEW_HANDLE) LASRX_EX_DLL_SYM(pDll->hAudioSourceDll, LASRX_EX_AUDIO_NEW_FUNCTION)) == NULL) {
        ReportError("LoadAudioSource", "Error getting proc address \"%s\"\n", LASRX_EX_AUDIO_NEW_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((pDll->pfDelete = (LASRX_EX_AUDIO_DELETE_HANDLE) LASRX_EX_DLL_SYM(pDll->hAudioSourceDll, LASRX_EX_AUDIO_DELETE_FUNCTION)) == NULL) {
        ReportError("LoadAudioSource", "Error getting proc address \"%s\"\n", LASRX_EX_AUDIO_DELETE_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((pDll->pfSetData = (LASRX_EX_AUDIO_SET_DATA) LASRX_EX_DLL_SYM(pDll->hAudioSourceDll, LASRX_EX_AUDIO_SET_DATA_FUNCTION)) == NULL) {
        ReportError("LoadAudioSource", "Error getting proc address \"%s\"\n", LASRX_EX_AUDIO_SET_DATA_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((pDll->pfRegister = (LASRX_EX_AUDIO_REGISTER) LASRX_EX_DLL_SYM(pDll->hAudioSourceDll, LASRX_EX_AUDIO_REGISTER_FUNCTION)) == NULL) {
        ReportError("LoadAudioSource", "Error getting proc address \"%s\"\n", LASRX_EX_AUDIO_REGISTER_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((pDll->pfUnregister = (LASRX_EX_AUDIO_UNREGISTER) LASRX_EX_DLL_SYM(pDll->hAudioSourceDll, LASRX_EX_AUDIO_UNREGISTER_FUNCTION)) == NULL) {
        ReportError("LoadAudioSource", "Error getting proc address \"%s\"\n", LASRX_EX_AUDIO_UNREGISTER_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((nRetCode = lasrxGetSessionHandle(hInstance, &hSession)) != LASRX_RETCODE_OK) {
        ReportLasrxError("LoadAudioSource", hInstance, "lasrxGetSessionHandle", nRetCode);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((nRetCode = lasrxGetAudioMode(hSession, &nAudioMode)) != LASRX_RETCODE_OK) {
        ReportLasrxError("LoadAudioSource", hInstance, "lasrxGetAudioMode", nRetCode);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((*(pDll->pfNew))(nAudioMode, phAudio) != 0) {
        ReportError("LoadAudioSource", "ERROR: function \"%s\" failed\n", LASRX_EX_AUDIO_NEW_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    if ((*(pDll->pfRegister))(*phAudio, hInstance) != 0) {
        ReportError("LoadAudioSource", "ERROR: function \"%s\" failed\n", LASRX_EX_AUDIO_REGISTER_FUNCTION);
        LASRX_EX_DLL_FREE(pDll->hAudioSourceDll);
        return -1;
    }
    return 0;
}

int ULoquendoASR::GetRecognitionResults(lasrxHandleType hInstance, int nMaxHypos) {
    lasrxResultType nRetCode = LASRX_RETCODE_OK;
    lasrxIntType nNumHypos;
    if ((nRetCode = lasrxRRGetNumberOfHypothesis(hInstance, &nNumHypos)) != LASRX_RETCODE_OK) {
        ReportLasrxError("GetRecognitionResults", hInstance, "lasrxRRGetNumberOfHypothesis", nRetCode);
        return -1;
    }
    if (nNumHypos > 0) {
        lasrxFloatType fConfidence;
        lasrxEncodedStringType eszString;
        lasrxIntType nWords;
        if ((nRetCode = lasrxRRGetHypothesisConfidence(hInstance, 0, &fConfidence)) != LASRX_RETCODE_OK) {
            ReportLasrxError("GetRecognitionResults", hInstance, "lasrxRRGetHypothesisConfidence", nRetCode);
            return -1;
        }
        if ((nRetCode = lasrxRRGetHypothesisString(hInstance, 0, &eszString)) != LASRX_RETCODE_OK) {
            ReportLasrxError("GetRecognitionResults", hInstance, "lasrxRRGetHypothesisString", nRetCode);
            return -1;
        }
        recognizedText = (char *) eszString;
        recognizeTextConfidence = fConfidence;
        urbi::UList payload;
        payload.push_back(recognizedText);
        payload.push_back(fConfidence);
        newRecognizedText.emit(payload);//, recognizeTextConfidence);
        lasrxFree(eszString);
    }
    return 0;
}

/******************************************************************************/
/* Builds grammar                                                             */

/******************************************************************************/
int ULoquendoASR::BuildRO(lasrxHandleType hInstance, char *szSource, char *szName) {
    lasrxResultType nRetCode = LASRX_RETCODE_OK;
    lasrxStringType szRPName = LASRX_RPNAME_DEFAULT;
    if ((nRetCode = lasrxOTFSetGrammarROGenerationDirectives(hInstance, szSource, NULL, LASRX_RPNAME_DEFAULT, NULL, 0, NULL, LASRX_FALSE, szName, NULL)) != LASRX_RETCODE_OK) {
        ReportLasrxError("BuildRO", hInstance, "lasrxOTFSetGrammarROGenerationDirectives", nRetCode);
        return -1;
    }
    if ((nRetCode = lasrxOTFGenerateRO(hInstance, LASRX_RUNMODE_BLOCKING, NULL)) != LASRX_RETCODE_OK) {
        ReportLasrxError("BuildRO", hInstance, "lasrxOTFGenerateRO", nRetCode);
        return -1;
    }
    cerr << "[ULoquendoASR]::BuildRO - Generate returned "<<nRetCode <<endl;
    return nRetCode;
}

/******************************************************************************/
/* Deletes grammar                                                            */

/******************************************************************************/
int ULoquendoASR::DeleteRO(lasrxHandleType hInstance, char *szName) {
    lasrxResultType nRetCode = LASRX_RETCODE_OK;
    char szCompleteName[256];
    sprintf(szCompleteName, "%s/%s", LASRX_RPNAME_DEFAULT, szName);
    if ((nRetCode = lasrxOTFDeleteRO(hInstance, szCompleteName)) != LASRX_RETCODE_OK) {
        ReportLasrxError("DeleteRO", hInstance, "lasrxOTFDeleteRO", nRetCode);
    }
    return nRetCode;
}

/******************************************************************************/
/* Perform recognition                                                        */

/******************************************************************************/
int ULoquendoASR::Recog(void *hInstance, char *szName, void *eszRule, LASRX_EX_DLL_POINTERS *pDll, void *hAudio) {
    lasrxResultType nRetCode = LASRX_RETCODE_OK;
    lasrxEncodedStringType eszRORule = NULL;
    char szCompleteName[256];
    sprintf(szCompleteName, "%s/%s", LASRX_RPNAME_DEFAULT, szName);
    if ((nRetCode = lasrxClearROs(hInstance)) != LASRX_RETCODE_OK) {
        ReportLasrxError("Recog", hInstance, "lasrxClearROs", nRetCode);
        return -1;
    }
    if ((nRetCode = lasrxAddRO(hInstance, szCompleteName, eszRule)) != LASRX_RETCODE_OK) {
        ReportLasrxError("Recog", hInstance, "lasrxAddRO", nRetCode);
        return -1;
    }
    if ((nRetCode = lasrxRecog(hInstance, LASRX_RUNMODE_BLOCKING, NULL)) != LASRX_RETCODE_OK) {
        ReportLasrxError("Recog", hInstance, "lasrxRecog", nRetCode);
        return -1;
    }
//    cerr << "[ULoquendoASR]::Recog - Recognition returned " << nRetCode <<endl;
    GetRecognitionResults(hInstance, 10);
    return nRetCode;
}

UStart(ULoquendoASR);
