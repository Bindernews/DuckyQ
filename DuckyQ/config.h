#define PLUG_NAME "DuckyQ"
#define PLUG_MFR "BinderNews"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'CAhv'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2019 Acme Inc"
#define PLUG_CLASS_NAME DuckyQ

#define BUNDLE_NAME "DuckyQ"
#define BUNDLE_MFR "BinderNews"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "DuckyQ"

#define PLUG_CHANNEL_IO "1.1-1 2.2-2"

#define PLUG_LATENCY 4096
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 600
#define PLUG_HEIGHT 600
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY DuckyQ_Entry
#define AUV2_ENTRY_STR "DuckyQ_Entry"
#define AUV2_FACTORY DuckyQ_Factory
#define AUV2_VIEW_CLASS DuckyQ_View
#define AUV2_VIEW_CLASS_STR "DuckyQ_View"

#define AAX_TYPE_IDS 'IEF1', 'IEF2'
#define AAX_TYPE_IDS_AUDIOSUITE 'IEA1', 'IEA2'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "DuckyQ\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"

// User-defined configs. Unrelated to iPlug

#define FFT_BLOCK_SIZE (4096)
#define MAX_INPUT_CHANS (4)
#define MAX_OUTPUT_CHANS (2)
#define GUI_FREQ_BUF_SIZE (256)
