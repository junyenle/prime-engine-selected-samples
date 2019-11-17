#include <unordered_map>
#include <string>

#pragma comment(lib,"ole32.lib")

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#ifdef _XBOX //Big-Endian If on XBOX
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian if on Windows
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

#include "SoundManager.h"

struct win32_audio
{
    XAUDIO2_BUFFER Buffer;
    std::vector<IXAudio2SourceVoice *> SourceVoices;
};

static std::unordered_map<std::string, win32_audio> SoundBuffers;
struct win32_xaudio
{
    IXAudio2 *XAudio;
    IXAudio2MasteringVoice *MasterVoice;
};

static win32_xaudio StaticXAudio;
void InitXAudio()
{
    bool XAudioInitFailure = FAILED(XAudio2Create(&StaticXAudio.XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
    if(XAudioInitFailure)
    {
        // TODO(Rohan): Logging
    }

    XAudioInitFailure = FAILED(StaticXAudio.XAudio->CreateMasteringVoice(&StaticXAudio.MasterVoice));
    if (XAudioInitFailure)
    {
        // TODO(Rohan): Logging
    }
}

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if(INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if( 0 == ReadFile( hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        if( 0 == ReadFile( hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        switch (dwChunkType)
        {
            case fourccRIFF:
                dwRIFFDataSize = dwChunkDataSize;
                dwChunkDataSize = 4;
                if( 0 == ReadFile( hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL ) )
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                break;

            default:
                if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
                    return HRESULT_FROM_WIN32( GetLastError() );            
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;
}

HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}

// TCHAR * strFileName = _TEXT("media\\MusicMono.wav");
void InitSound(char *FileName, bool Loop, int NumVoice)
{
    WAVEFORMATEXTENSIBLE wfx = {0};
    XAUDIO2_BUFFER buffer = {0};
    
    HANDLE hFile = CreateFileA(FileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL );
	if (INVALID_HANDLE_VALUE == hFile)
	{
		// TODO(Rohan): Logging
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
	{
		// TODO(Rohan): Logging
	}

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    // check the file type, should be fourccWAVE or 'XWMA'
    FindChunk(hFile,fourccRIFF,dwChunkSize, dwChunkPosition );
    DWORD filetype;
    ReadChunkData(hFile,&filetype,sizeof(DWORD),dwChunkPosition);
	if (filetype != fourccWAVE)
	{
		// TODO(Rohan): Logging
	}

    FindChunk(hFile,fourccFMT, dwChunkSize, dwChunkPosition );
    ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition );

    //fill out the audio data buffer with the contents of the fourccDATA chunk
    FindChunk(hFile,fourccDATA,dwChunkSize, dwChunkPosition );
    BYTE * pDataBuffer = (BYTE *)malloc(sizeof(BYTE) * dwChunkSize);
    ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

    buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
    buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    if(Loop)
    {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    win32_audio Audio;
    for(int i = 0; i < NumVoice; ++i)
    {
        IXAudio2SourceVoice* pSourceVoice;
        StaticXAudio.XAudio->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx);
        Audio.SourceVoices.push_back(pSourceVoice);
    }
    
    Audio.Buffer = buffer;
    SoundBuffers.insert(std::make_pair(std::string(FileName), Audio));
}
    
void PlaySoundX(std::string name)
{
    win32_audio Audio = SoundBuffers[name];
    IXAudio2SourceVoice *pSourceVoice = Audio.SourceVoices[0];
    for(IXAudio2SourceVoice *SourceVoice : Audio.SourceVoices)
    {
        XAUDIO2_VOICE_STATE VoiceState;
        SourceVoice->GetState(&VoiceState);
        if(VoiceState.BuffersQueued > 0)
        {
            continue;
        }
        else
        {
            pSourceVoice = SourceVoice;
        }
    }

    pSourceVoice->SubmitSourceBuffer(&Audio.Buffer);
    pSourceVoice->Start( 0 );
}
