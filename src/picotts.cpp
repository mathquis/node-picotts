#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>

#include "svoxpico/picoapi.h"
#include "svoxpico/picoapid.h"
#include "svoxpico/picoos.h"

#include <napi.h>

using namespace std;

enum typelog {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

struct structlog {
    typelog level = WARN;
};

structlog LOGCFG = {};

class LOG {
public:
    LOG() {}
    LOG(typelog type) {
        msglevel = type;
        operator << ("[ "+getLabel(type)+" ] ");
    }
    ~LOG() {
        if(opened) {
            cout << endl;
        }
        opened = false;
    }
    template<class T>
    LOG &operator<<(const T &msg) {
        if(msglevel >= LOGCFG.level) {
            cout << msg;
            opened = true;
        }
        return *this;
    }
private:
    bool opened = false;
    typelog msglevel = DEBUG;
    inline string getLabel(typelog type) {
        string label;
        switch(type) {
            case DEBUG: label = "DEBUG"; break;
            case INFO:  label = "INFO "; break;
            case WARN:  label = "WARN "; break;
            case ERROR: label = "ERROR"; break;
        }
        return label;
    }
};

class SvoxPicoTTS : public Napi::ObjectWrap<SvoxPicoTTS> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		SvoxPicoTTS(const Napi::CallbackInfo& info);
		~SvoxPicoTTS();
		Napi::Value speak(const Napi::CallbackInfo& info);

	private:
		static Napi::FunctionReference constructor;

		void cleanup();

		int32_t 			volume;
		int32_t 			speed;
		int32_t 			pitch;

		bool 				processed;

	    pico_System         picoSystem;
	    pico_Resource       picoTaResource;
	    pico_Resource       picoSgResource;
	    pico_Engine         picoEngine;

	    // PicoVoices_t        voices;

	    pico_Char *         local_text;
	    pico_Int16          text_remaining;
	    char *              picoLingwarePath;

	    char                picoVoiceName[10];

	    void *              picoMemArea;
	    pico_Char *         picoTaFileName;
	    pico_Char *         picoSgFileName;
	    pico_Char *         picoTaResourceName;
	    pico_Char *         picoSgResourceName;
};


/* ============================================
 Native module implementation
============================================ */

Napi::FunctionReference SvoxPicoTTS::constructor;

Napi::Object SvoxPicoTTS::Init(Napi::Env env, Napi::Object exports) {

	Napi::Function func = DefineClass(env, "SvoxPicoTTS", {
    	InstanceMethod("speak", &SvoxPicoTTS::speak)
	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();

	exports.Set("SvoxPicoTTS", func);

	return exports;
}

SvoxPicoTTS::SvoxPicoTTS(const Napi::CallbackInfo& info) : Napi::ObjectWrap<SvoxPicoTTS>(info) {
	extern structlog LOGCFG;
	LOGCFG.level = WARN;

	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if ( info.Length() < 1 ) {
		// throw
		Napi::TypeError::New(env, "You need to provide an options object").ThrowAsJavaScriptException();
		return;
	}

    const Napi::Object config = info[0].As<Napi::Object>();

    // Handle debug
    if ( config.Has("debug") ) {
    	if ( config.Get("debug").ToBoolean().Value() ) {
    		LOGCFG.level = DEBUG,
    		LOG(DEBUG) << "Using debug...";
    	}
    }

    // Modifiers
    volume					= 100;
    speed 					= 100;
    pitch 					= 100;

    processed				= false;

    // Engine
	picoSystem              = 0;
    picoTaResource          = 0;
    picoSgResource          = 0;
    picoEngine              = 0;

    strcpy( picoVoiceName, "PicoVoice" );

    picoMemArea             = 0;
    picoTaFileName          = 0;
    picoSgFileName          = 0;
    picoTaResourceName      = 0;
    picoSgResourceName      = 0;

    const int PICO_MEM_SIZE = 2500000;
    text_remaining          = 0;
    pico_Retstring  outMessage;
    int             ret;

    picoMemArea = malloc( PICO_MEM_SIZE );

    LOG(INFO) << "Initializing...\n";

    // Set volume
    if (config.Has("volume")) {
		volume = std::max(0, std::min(500, config.Get("volume").ToNumber().Int32Value()));
		LOG(DEBUG) << "Global volume: " << volume;
	}

    // Set pitch
    if (config.Has("pitch")) {
		pitch = std::max(0, std::min(500, config.Get("pitch").ToNumber().Int32Value()));
		LOG(DEBUG) << "Global pitch: " << pitch;
	}

    // Set speed
    if (config.Has("speed")) {
		speed = std::max(0, std::min(500, config.Get("speed").ToNumber().Int32Value()));
		LOG(DEBUG) << "Global speed: " << speed;
	}

	if ( (ret = pico_initialize( picoMemArea, PICO_MEM_SIZE, &picoSystem )) ) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot initialize pico: " << outMessage;
        pico_terminate(&picoSystem);
        picoSystem = 0;
        return;
    }

    LOG(DEBUG) << "Loading TA file...\n";

   /* Load the text analysis Lingware resource file.   */
    if (!config.Has("ta_file")) throw Napi::Error::New(env, "Missing ta file path");
	const std::string taFile = config.Get("ta_file").ToString();
	picoTaFileName = (pico_Char *) taFile.c_str();

    // attempt to load it
    if ( (ret = pico_loadResource(picoSystem, picoTaFileName, &picoTaResource)) ) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot load text analysis resource file: " << outMessage;
        // goto unloadTaResource;
    }

    LOG(DEBUG) << "Loading SG file...";

    /* Load the signal generation Lingware resource file.   */
    if (!config.Has("sg_file")) throw Napi::Error::New(env, "Missing sg file path");
	const std::string sgFile = config.Get("sg_file").ToString();
	picoSgFileName = (pico_Char *) sgFile.c_str();

    if ( (ret = pico_loadResource(picoSystem, picoSgFileName, &picoSgResource)) ) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot load signal generation Lingware resource file: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(DEBUG) << "Getting TA resource name...";

    /* Get the text analysis resource name.     */
    picoTaResourceName = (pico_Char *) malloc( PICO_MAX_RESOURCE_NAME_SIZE );
    if((ret = pico_getResourceName( picoSystem, picoTaResource, (char *) picoTaResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot get the text analysis resource name: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(DEBUG) << "Getting SG resource name...";

    /* Get the signal generation resource name. */
    picoSgResourceName = (pico_Char *) malloc( PICO_MAX_RESOURCE_NAME_SIZE );
    if((ret = pico_getResourceName( picoSystem, picoSgResource, (char *) picoSgResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot get the signal generation resource name: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(INFO) << "Creating voice definition...";

    /* Create a voice definition.   */
    if((ret = pico_createVoiceDefinition( picoSystem, (const pico_Char *) picoVoiceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot create voice definition: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(INFO) << "Adding TA resource to voice definition...";

    /* Add the text analysis resource to the voice. */
    if((ret = pico_addResourceToVoiceDefinition( picoSystem, (const pico_Char *) picoVoiceName, picoTaResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot add the text analysis resource to the voice: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(INFO) << "Adding SG resource to voice definition...";

    /* Add the signal generation resource to the voice. */
    if((ret = pico_addResourceToVoiceDefinition( picoSystem, (const pico_Char *) picoVoiceName, picoSgResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot add the signal generation resource to the voice: " << outMessage;
        // goto unloadSgResource;
    }

    LOG(INFO) << "Creating engine...";

    /* Create a new Pico engine. */
    if((ret = pico_newEngine( picoSystem, (const pico_Char *) picoVoiceName, &picoEngine ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        LOG(ERROR) << "Cannot create a new pico engine: " << outMessage;
        // goto disposeEngine;
    }

    LOG(INFO) << "Ready";
}

SvoxPicoTTS::~SvoxPicoTTS() {
    cleanup();
}

Napi::Value SvoxPicoTTS::speak(const Napi::CallbackInfo& info) {

	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if ( processed ) {
		// throw
		Napi::TypeError::New(env, "Instance has already processed text. Please create a new instance.").ThrowAsJavaScriptException();
		return env.Null();
	}

	processed = true;

	// Get emit function
	Napi::Function emit = info.This().As<Napi::Object>().Get("emit").As<Napi::Function>();

	const std::string text = info[0].As<Napi::String>().ToString();

	LOG(INFO) << "Speaking text: " << text;

	int local_pitch = pitch;
	int local_speed = speed;
	int local_volume = volume;

	if ( info.Length() > 1 ) {
		const Napi::Object config = info[1].As<Napi::Object>();
		if ( config.Has("pitch") ) {
			local_pitch = std::max(0, std::min(500, config.Get("pitch").ToNumber().Int32Value()));
			LOG(DEBUG) << "Using local pitch: " << local_pitch;
		}
		if ( config.Has("speed") ) {
			local_speed = std::max(0, std::min(500, config.Get("speed").ToNumber().Int32Value()));
			LOG(DEBUG) << "Using local speed: " << local_speed;
		}
		if ( config.Has("volume") ) {
			local_volume = std::max(0, std::min(500, config.Get("volume").ToNumber().Int32Value()));
			LOG(DEBUG) << "Using local volume: " << local_volume;
		}
	}

    // *****

	const int       MAX_OUTBUF_SIZE     = 128;
    const int       PCM_BUFFER_SIZE     = 512;
    pico_Char *     inp                 = 0;
    pico_Int16      bytes_sent, bytes_recv, out_data_type;
    short           outbuf[MAX_OUTBUF_SIZE/2];
    pico_Retstring  outMessage;
    int8_t          pcm_buffer[ PCM_BUFFER_SIZE ];
    int             ret, getstatus;

    // *****

    std::string local_text = text;

    // Pitch
	local_text = std::string("<pitch level=\"") + std::to_string(local_pitch) + "\">" + local_text + "</pitch>";
	// Speed
	local_text = std::string("<speed level=\"") + std::to_string(local_speed) + "\">" + local_text + "</speed>";
	// Volume
	local_text = std::string("<volume level=\"") + std::to_string(local_volume) + "\">" + local_text + "</volume>";

    // Add sentence marker to synthesize all the text
    local_text = std::string("<s>") + local_text + "</s>";

    const char * text_to_process = local_text.c_str();



	long long int text_length = strlen(text_to_process);
    inp = (pico_Char *) text_to_process;

    unsigned int bufused = 0;
    memset( pcm_buffer, 0, PCM_BUFFER_SIZE );

    LOG(DEBUG) << "Processing...";

	/* synthesis loop   */
    while(1)
    {
        //−32,768 to 32,767.
        if (text_remaining <= 0)
        {
            if ( text_length <= 0 ) {
                break; /* done */
            }
            // continue feed main text
            else {
                int increment = text_length >= 32767 ? 32767 : text_length;
                text_length -= increment;
                text_remaining = increment;
            }
        }

        /* Feed the text into the engine.   */
        if ( (ret = pico_putTextUtf8(picoEngine, inp, text_remaining, &bytes_sent)) ) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            emit.Call(
            	info.This(),
            	{
            		Napi::String::New(env, "error"),
        			Napi::String::New(env, outMessage)
            	}
            );
            return Napi::Boolean::New(env, false);
        }

        text_remaining -= bytes_sent;
        inp += bytes_sent;

    	LOG(DEBUG) << "Bytes send: " << bytes_sent;

        do {

            /* Retrieve the samples */
            getstatus = pico_getData( picoEngine, (void *) outbuf, MAX_OUTBUF_SIZE, &bytes_recv, &out_data_type );
            if ( (getstatus !=PICO_STEP_BUSY) && (getstatus !=PICO_STEP_IDLE) ) {
                pico_getSystemStatusMessage(picoSystem, getstatus, outMessage);
	            emit.Call(
	            	info.This(),
	            	{
	            		Napi::String::New(env, "error"),
            			Napi::String::New(env, outMessage)
	            	}
	            );
            	return Napi::Boolean::New(env, false);
            }

            /* copy partial encoding and get more bytes */
            if ( bytes_recv > 0 )
            {
	          	LOG(DEBUG) << "status: " << getstatus << "    bytes_recv: " << bytes_recv << "     bufused: " << bufused;

                if ( (bufused + bytes_recv) <= PCM_BUFFER_SIZE ) {
                    memcpy( pcm_buffer + bufused, (int8_t *) outbuf, bytes_recv );
                    bufused += bytes_recv;
                }
                else
                {
          			LOG(DEBUG) << "Sending chunk...";

                	// Send the chunk
	                emit.Call(
                    	info.This(),
                    	{
                    		Napi::String::New(env, "data"),
                    		Napi::Buffer<int8_t>::Copy(env, (int8_t *)pcm_buffer, bufused),
            				Napi::Boolean::New(env, false)
                    	}
                    );

                    bufused = 0;
                    memcpy( pcm_buffer, (int8_t *) outbuf, bytes_recv );
                    bufused += bytes_recv;
                }
            }

        } while (PICO_STEP_BUSY == getstatus);

        // Send the last chunk
        if ( bufused > 0 ) {

          	LOG(DEBUG) << "Sending chunk...";

            emit.Call(
            	info.This(),
            	{
            		Napi::String::New(env, "data"),
            		Napi::Buffer<int8_t>::Copy(env, (int8_t *)pcm_buffer, bufused),
            		Napi::Boolean::New(env, true)
            	}
            );
	    }
    }

    LOG(INFO) << "Processing complete";

    // Processing has ended
	emit.Call(
		info.This(),
		{Napi::String::New(env, "end")}
	);

	// cleanup();

	return Napi::Boolean::New(env, true);
}

void SvoxPicoTTS::cleanup() {
    if (picoEngine) {
        pico_disposeEngine( picoSystem, &picoEngine );
        pico_releaseVoiceDefinition( picoSystem, (pico_Char *) picoVoiceName );
        picoEngine = 0;
    }

    if (picoSgResource) {
        pico_unloadResource( picoSystem, &picoSgResource );
        picoSgResource = 0;
    }

    if (picoTaResource) {
        pico_unloadResource( picoSystem, &picoTaResource );
        picoTaResource = 0;
    }

    if (picoSystem) {
        pico_terminate(&picoSystem);
        picoSystem = 0;
    }

    if ( picoMemArea )
        free( picoMemArea );
    if ( picoTaResourceName )
        free( picoTaResourceName );
    if ( picoSgResourceName )
        free( picoSgResourceName );
    // if ( picoTaFileName )
    //     free( picoTaFileName );
    // if ( picoSgFileName )
    //     free( picoSgFileName );

	LOG(INFO) << "Destroyed";
}

/* ============================================
 Native module initialization
============================================ */

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	// Register the engine
	SvoxPicoTTS::Init(env, exports);

	return exports;
};

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)

