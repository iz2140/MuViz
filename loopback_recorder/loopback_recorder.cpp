#include <intrin.h>

#include <atomic>
#include <chrono>
#include <cstdio>

#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Objbase.h>
#include <comdef.h>

#include <Functiondiscoverykeys_devpkey.h>

#define UNITY_INTERFACE_API __stdcall
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)

#if _DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define __CHK(x, ret)                                                  \
  {                                                                    \
    auto result = x;                                                   \
    if (FAILED(result)) {                                              \
      LOG("%s(%d): error \"%s\" returned by %s\n", __FILE__, __LINE__, \
          _com_error{result}.ErrorMessage(), #x);                      \
      return ret;                                                      \
    }                                                                  \
  }
#define CHK(x) __CHK(x, /**/)
#define CHK_NULL(x) __CHK(x, nullptr)
#define CHK_FALSE(x) __CHK(x, false)
#define CHK_ERR(x) __CHK(x, 1)

static constexpr auto kMaxBufferDelay = std::chrono::milliseconds(420);
static constexpr auto kSampleBufferSize = 1000000u;
static constexpr auto kSamplesPerSecond = 44100u;
static constexpr WAVEFORMATEX kOutputFormat{
    WAVE_FORMAT_IEEE_FLOAT,
    2u,
    kSamplesPerSecond,
    kSamplesPerSecond * 2 * sizeof(float),
    sizeof(float) * 2,
    sizeof(float) * 8,
    0u,
};

template <typename T>
class AutoPtr {
 public:
  AutoPtr(T* value) : value_(value) {}
  AutoPtr() = default;
  AutoPtr(const AutoPtr&) = delete;
  AutoPtr(AutoPtr&& other) { *this = std::move(other); }
  void operator=(const AutoPtr&) = delete;
  void operator=(AutoPtr&& rhs) {
    if (value_) value_->Release();
    value_ = rhs.value_;
    rhs.value_ = nullptr;
  }
  ~AutoPtr() {
    if (value_) value_->Release();
  }
  T** operator&() { return &value_; }
  T* operator->() { return value_; }
  operator bool() { return !!value_; }
  T* Detach() {
    auto old_value = value_;
    value_ = nullptr;
    return old_value;
  }

 private:
  T* value_ = nullptr;
};

IMMDeviceEnumerator* device_enumerator;
float samples[kSampleBufferSize];
uint32_t sample_count;
CRITICAL_SECTION samples_critical_section;
CONDITION_VARIABLE samples_condition_variable;
std::atomic_bool quit;
HANDLE thread;

DWORD WINAPI ThreadProc(LPVOID parameter) {
  AutoPtr<IMMDevice> device{(IMMDevice*)parameter};

  AutoPtr<IAudioClient> audio_client;
  CHK_ERR(device->Activate(__uuidof(audio_client), CLSCTX_ALL, nullptr,
                           (void**)&audio_client));

  CHK_ERR(audio_client->Initialize(
      AUDCLNT_SHAREMODE_SHARED,
      AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, 1, 0,
      &kOutputFormat, nullptr));

  AutoPtr<IAudioCaptureClient> capture_client;
  CHK_ERR(audio_client->GetService(__uuidof(IAudioCaptureClient),
                                   (void**)&capture_client));

  CHK_ERR(audio_client->Start());

  while (!quit) {
    UINT32 packet_size;
    CHK_ERR(capture_client->GetNextPacketSize(&packet_size));

    while (packet_size) {
      float* frame_data;
      UINT32 num_frames_read;
      DWORD flags;
      CHK_ERR(capture_client->GetBuffer((BYTE**)&frame_data, &num_frames_read,
                                        &flags, nullptr, nullptr));

      {
        EnterCriticalSection(&samples_critical_section);

        auto num_samples_read = num_frames_read * 2;
        if (sample_count + num_samples_read >= kSampleBufferSize) {
          sample_count = 0;
        }

        __movsd((unsigned long*)(samples + sample_count),
                (unsigned long*)frame_data, num_samples_read);
        sample_count += num_samples_read;

        LeaveCriticalSection(&samples_critical_section);
      }

      WakeConditionVariable(&samples_condition_variable);

      CHK_ERR(capture_client->ReleaseBuffer(num_frames_read));

      CHK_ERR(capture_client->GetNextPacketSize(&packet_size));
    }
  }

  return 0;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Stop() {
  quit = true;
  if (thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    thread = nullptr;
  }
  sample_count = 0;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
Start(uint32_t device_index) {
  Stop();
  quit = false;

  if (!device_enumerator) return false;

  AutoPtr<IMMDevice> device;
  if (device_index == ~0u) {
    CHK_FALSE(device_enumerator->GetDefaultAudioEndpoint(
        EDataFlow::eRender, ERole::eMultimedia, &device));
  } else {
    AutoPtr<IMMDeviceCollection> device_collection;
    CHK_FALSE(device_enumerator->EnumAudioEndpoints(
        eRender, DEVICE_STATE_ACTIVE, &device_collection));

    UINT device_count;
    CHK_FALSE(device_collection->GetCount(&device_count));

    if (device_index >= (int)device_count) return false;

    CHK_FALSE(device_collection->Item(device_index, &device));
  }

  thread = CreateThread(nullptr, 0, &ThreadProc, device.Detach(), 0, nullptr);
  return true;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
Read(uint32_t out_sample_count, float* out_samples) {
  if (!thread) return false;

  EnterCriticalSection(&samples_critical_section);
  auto result = [&]() {
    while (!quit && sample_count < out_sample_count) {
      auto buffer_delay_ms =
          (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(
              kMaxBufferDelay)
              .count();

      auto sleep_result =
          SleepConditionVariableCS(&samples_condition_variable,
                                   &samples_critical_section, buffer_delay_ms);

      if (!sleep_result && GetLastError() == ERROR_TIMEOUT) {
        return false;
      }
    }

    __movsd((unsigned long*)out_samples, (unsigned long*)samples,
            out_sample_count);

    __movsd((unsigned long*)samples,
            (unsigned long*)(samples + out_sample_count),
            sample_count - out_sample_count);
    sample_count -= out_sample_count;

    return true;
  }();
  LeaveCriticalSection(&samples_critical_section);

  return result;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(void* /*unityInterfaces*/) {
  CHK(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
  CHK(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                       IID_PPV_ARGS(&device_enumerator)));

  InitializeCriticalSection(&samples_critical_section);
  InitializeConditionVariable(&samples_condition_variable);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  Stop();

  device_enumerator->Release();
  device_enumerator = nullptr;

  DeleteCriticalSection(&samples_critical_section);
  CoUninitialize();
}

extern "C" BSTR UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetEndpointName(UINT index) {
  if (!device_enumerator) return nullptr;

  AutoPtr<IMMDeviceCollection> device_collection;
  CHK_NULL(device_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                                 &device_collection));

  UINT endpoint_count;
  CHK_NULL(device_collection->GetCount(&endpoint_count));
  if (index >= endpoint_count) return nullptr;

  AutoPtr<IMMDevice> device;
  CHK_NULL(device_collection->Item(index, &device));

  AutoPtr<IPropertyStore> property_store;
  CHK_NULL(device->OpenPropertyStore(STGM_READ, &property_store));

  PROPVARIANT property_variant;
  CHK_NULL(
      property_store->GetValue(PKEY_Device_FriendlyName, &property_variant));

  return SysAllocString(property_variant.bstrVal);
}

BOOL WINAPI main(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/,
                 LPVOID /*lpvReserved*/) {
  return TRUE;
}
