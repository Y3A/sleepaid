#include <Windows.h>
#include <stdio.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#pragma comment(lib, "ole32")

#define warn(x, y) printf("Error %s : 0x%08X\n", x, y)

int GraduallyDecreaseAudio(IAudioEndpointVolume *pEndpointVolume, DWORD dwPer, DWORD dwBy, DWORD dwStopAt);

int main(int argc, char **argv)
{
    HRESULT                 hr;
    IMMDeviceEnumerator     *pEnumerator = NULL;
    IMMDevice               *pDevice = NULL;
    IAudioEndpointVolume    *pEndpointVolume = NULL;
    DWORD                   dwPer = 0, dwBy = 0, dwStopAt = 0;

    if (argc < 4) {
        printf("Usage: %s <seconds before decreasing audio> <percentage to decrease> <stop at>\n", argv[0]);
        printf("Example: <%s 200 2 0> decreases audio by 2%% every 200 seconds, until 0.\n", argv[0]);

        return 1;
    }

    dwPer = atoi(argv[1]);
    dwBy = atoi(argv[2]);
    dwStopAt = atoi(argv[3]);

    // Initialize COM
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        warn("CoInitialize fail", hr);
        return 1;
    }

    // Create a device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator);
    if (FAILED(hr)) {
        warn("CoCreateInstance fail", hr);
        CoUninitialize();
        return 1;
    }

    // Get the default audio endpoint
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
    if (FAILED(hr)) {
        warn("GetDefaultAudioEndpoint fail", hr);
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    // Get the endpoint volume interface
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&pEndpointVolume);
    if (FAILED(hr)) {
        warn("Activate fail", hr);
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    puts("Acquired pointers, now decreasing audio gradually");

    if (!GraduallyDecreaseAudio(pEndpointVolume, dwPer, dwBy, dwStopAt))
        goto out;

out:
    pEndpointVolume->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
    return 0;
}

int GraduallyDecreaseAudio(IAudioEndpointVolume *pEndpointVolume, DWORD dwPer, DWORD dwBy, DWORD dwStopAt)
{
    HRESULT                 hr;
    float                   fCurrentAudioLevel = 0.0;
    float                   fStopAt = (float)((float)dwStopAt / (float)100.0f);
    float                   fBy = (float)((float)dwBy / (float)100.0f);

    // Initial
    hr = pEndpointVolume->GetMasterVolumeLevelScalar(&fCurrentAudioLevel);
    if (FAILED(hr)) {
        warn("GetMasterVolumeLevelScalar fail", hr);
        return 0;
    }

    printf("Initial Volume: %d\n", (DWORD)(fCurrentAudioLevel*100.0f));

    // Gradually decrease
    while (fCurrentAudioLevel > fStopAt && fCurrentAudioLevel > 0.0f && fStopAt + fBy <= fCurrentAudioLevel) {
        Sleep(dwPer * 1000);
        hr = pEndpointVolume->SetMasterVolumeLevelScalar(fCurrentAudioLevel - fBy, NULL);
        if (FAILED(hr)) {
            warn("SetMasterVolumeLevelScalar fail", hr);
            return 0;
        }
        fCurrentAudioLevel -= fBy;
        printf("Current Volume: %d\n", (DWORD)(fCurrentAudioLevel*100.0f));
    }

    return 1;
}