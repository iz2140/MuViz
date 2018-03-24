#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Objbase.h>
#include <comdef.h>

#include <Functiondiscoverykeys_devpkey.h>

#include <IUnityInterface.h>

#define __CHK(x, ret)                                                     \
  {                                                                       \
    auto result = x;                                                      \
    if (FAILED(result)) {                                                 \
      printf("%s(%d): error \"%s\" returned by %s\n", __FILE__, __LINE__, \
             _com_error{result}.ErrorMessage(), #x);                      \
      return ret;                                                         \
    }                                                                     \
  }
#define CHK(x) __CHK(x, /**/)
#define CHK_NULL(x) __CHK(x, nullptr)

static constexpr auto kMaxBufferSize = 10000u;
static constexpr auto kSamplesPerSecond = 44100u;
static constexpr WAVEFORMATEX kOutputFormat{
    WAVE_FORMAT_IEEE_FLOAT,
    1u,
    kSamplesPerSecond,
    kSamplesPerSecond * sizeof(float),
    sizeof(float),
    sizeof(float) * 8,
    0u,
};

template <typename T>
class AutoPtr {
 public:
  ~AutoPtr() {
    if (value_) value_->Release();
  }
  T** operator&() { return &value_; }
  T* operator->() { return value_; }
  operator bool() { return !!value_; }

 private:
  T* value_ = nullptr;
};

AutoPtr<IMMDeviceEnumerator> device_enumerator;
std::vector<float> samples;
std::mutex samples_mutex;
std::condition_variable samples_condition_variable;
std::atomic_bool quit;
std::thread thread;

void ThreadProc(uint32_t device_index) {
  if (!device_enumerator) return;

  AutoPtr<IMMDevice> device;

  if (device_index == ~0u) {
    CHK(device_enumerator->GetDefaultAudioEndpoint(
        EDataFlow::eRender, ERole::eMultimedia, &device));
  } else {
    AutoPtr<IMMDeviceCollection> device_collection;
    CHK(device_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                              &device_collection));

    UINT device_count;
    CHK(device_collection->GetCount(&device_count));

    if (device_index >= (int)device_count) return;

    CHK(device_collection->Item(device_index, &device));
  }

  AutoPtr<IAudioClient> audio_client;
  CHK(device->Activate(__uuidof(audio_client), CLSCTX_ALL, nullptr,
                       (void**)&audio_client));

  CHK(audio_client->Initialize(
      AUDCLNT_SHAREMODE_SHARED,
      AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, 1, 0,
      &kOutputFormat, nullptr));

  AutoPtr<IAudioCaptureClient> capture_client;
  CHK(audio_client->GetService(__uuidof(IAudioCaptureClient),
                               (void**)&capture_client));

  CHK(audio_client->Start());

  while (!quit) {
    UINT32 packet_size;
    CHK(capture_client->GetNextPacketSize(&packet_size));

    while (packet_size) {
      float* sample_data;
      UINT32 frames_read;
      DWORD flags;
      CHK(capture_client->GetBuffer((BYTE**)&sample_data, &frames_read, &flags,
                                    nullptr, nullptr));

      {
        std::lock_guard<std::mutex> lock{samples_mutex};
        samples.insert(samples.end(), sample_data, sample_data + frames_read);

        if (samples.size() > kMaxBufferSize) {
          auto num_to_remove = samples.size() - kMaxBufferSize;
          samples.erase(samples.begin(), samples.begin() + num_to_remove);
        }
      }
      samples_condition_variable.notify_all();

      CHK(capture_client->ReleaseBuffer(frames_read));

      CHK(capture_client->GetNextPacketSize(&packet_size));
    }
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Stop() {
  quit = true;
  if (thread.joinable()) thread.join();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
Start(uint32_t device_index) {
  Stop();
  quit = false;
  thread = std::thread{ThreadProc, device_index};
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
Read(uint32_t sample_count, float* out_samples) {
  std::unique_lock<std::mutex> lock{samples_mutex};
  samples_condition_variable.wait(
      lock, [&]() { return quit || sample_count <= samples.size(); });

  memcpy(out_samples, samples.data(), sample_count * sizeof(float));

  samples.erase(samples.begin(), samples.begin() + sample_count);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* /*unityInterfaces*/) {
  CHK(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
  CHK(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                       IID_PPV_ARGS(&device_enumerator)));

  Start(~0u);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  Stop();
}

// void GetEndpoints() {
//  if (!unmanaged_->device_enumerator) return nullptr;
//
//  AutoPtr<IMMDeviceCollection> device_collection;
//  CHK_NULL(unmanaged_->device_enumerator->EnumAudioEndpoints(
//      eRender, DEVICE_STATE_ACTIVE, &device_collection));
//
//  UINT device_count;
//  CHK_NULL(device_collection->GetCount(&device_count));
//
//  auto name_list = gcnew List<String ^>();
//  for (auto i = 0u; i < device_count; ++i) {
//    AutoPtr<IMMDevice> device;
//    CHK_NULL(device_collection->Item(i, &device));
//
//    AutoPtr<IPropertyStore> property_store;
//    CHK_NULL(device->OpenPropertyStore(STGM_READ, &property_store));
//
//    PROPVARIANT property_variant;
//    CHK_NULL(
//        property_store->GetValue(PKEY_Device_FriendlyName,
//        &property_variant));
//
//    name_list->Add(gcnew String(property_variant.bstrVal));
//  }
//
//  return name_list;
//}
