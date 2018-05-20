
//
// "snd_osx.m" - MacOS X Sound driver.
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo   [http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software     [http://www.idsoftware.com].
//
// Version History:
// v1.2.0: Rewritten. Uses now AudioGraph/AudioUnit.
// v1.0.9: Added function for reserving the buffersize [required to be called before any QuickTime media is loaded].
// v1.0.0: Initial release.
//


#import "quakedef.h"

#import <FDFramework/FDFramework.h>



static FDAudioBuffer*   sSndAudioBuffer         = nil;
static UInt8            sSndBuffer[64*1024]     = { 0 };
static UInt32           sSndBufferPosition      = 0;
static const UInt32     skSndBufferByteCount    = 4 * 1024;



static NSUInteger SNDDMA_Callback (void* pDst, NSUInteger numBytes, void* pContext);



NSUInteger SNDDMA_Callback (void* pDst, NSUInteger numBytes, void* pContext)
{
    while (numBytes)
    {
        if (sSndBufferPosition >= FD_SIZE_OF_ARRAY (sSndBuffer))
        {
            sSndBufferPosition = 0;
        }

        NSUInteger toCopy = FD_SIZE_OF_ARRAY (sSndBuffer) - sSndBufferPosition;

        if (toCopy > numBytes)
        {
            toCopy = numBytes;
        }

        FD_MEMCPY (pDst, &(sSndBuffer[sSndBufferPosition]), toCopy);

        pDst                += toCopy;
        numBytes            -= toCopy;
        sSndBufferPosition  += toCopy;
    }

    return 0;
}



int SNDDMA_Init (void)
{
    cbool        success         = false;
    const UInt32    sampleRate      = sound_rate_hz;  // SOUND_RATE sndspeed.value; //11025; // 44100;
    const UInt32    bitsPerChannel  = 16;
    const UInt32    numChannels     = 2;

    FD_MEMSET (&(sSndBuffer[0]), 0, FD_SIZE_OF_ARRAY (sSndBuffer));
    sSndBufferPosition  = 0;

    sSndAudioBuffer = [[FDAudioBuffer alloc] initWithMixer: [FDAudioMixer sharedAudioMixer]
                                                 frequency: sampleRate
                                            bitsPerChannel: bitsPerChannel
                                                  channels: numChannels
                                                  callback: &SNDDMA_Callback
                                                   context: nil];

    if (sSndAudioBuffer)
    {
        shm = Hunk_AllocName (sizeof (*shm), "shm");

        shm->splitbuffer        = 0;
        shm->samplebits         = bitsPerChannel;
        shm->speed              = sampleRate;
        shm->channels           = numChannels;
        shm->samples            = FD_SIZE_OF_ARRAY (sSndBuffer) / (bitsPerChannel >> 3);
        shm->samplepos          = 0;
        shm->soundalive         = true;
        shm->gamealive          = true;
        shm->submission_chunk   = skSndBufferByteCount;
        shm->buffer             = &(sSndBuffer[0]);

        success = true;
    }
    else
    {
        Con_PrintLinef ("Audio init: Failed to initialize!");
    }

    return success;
}



void    SNDDMA_Shutdown (void)
{
    [sSndAudioBuffer release];
    sSndAudioBuffer = nil;
}



void    SNDDMA_Submit (void)
{
}



int SNDDMA_GetDMAPos (void)
{
    int pos = 0;

    if (sSndAudioBuffer != nil)
    {
        pos = sSndBufferPosition / (shm->samplebits >> 3);
    }

    return pos;
}

