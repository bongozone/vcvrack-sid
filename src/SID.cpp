#include "PQ.hpp"
#include <climits>
#include "sid.h"
#define MAX_VOLTAGE 5.f
#define CLOCK 1022730
#define NUM_VOICES 3

/*
TODO:
	- should gates be normalled?
	- should freqs be normalled?
	- pitch not constant across sample rates!
*/

struct MyLabel : Widget {
	std::string text;
	int fontSize;
	NVGcolor color = nvgRGB(255,20,20);
	MyLabel(int _fontSize = 18) {
		fontSize = _fontSize;
	}
	void draw(NVGcontext *vg) override {
		nvgTextAlign(vg, NVG_ALIGN_CENTER|NVG_ALIGN_BASELINE);
		nvgFillColor(vg, color);
		nvgFontSize(vg, fontSize);
		nvgText(vg, box.pos.x, box.pos.y, text.c_str(), NULL);
	}
};



/*
0 REM *** C64-WIKI SOUND-DEMO ***
10 S = 54272: W = 17: ON INT(RND(TI)*4)+1 GOTO 12,13,14,15
12 W = 33: GOTO 15
13 W = 65: GOTO 15
14 W = 129
15 POKE S+24,15: POKE S+5,97: POKE S+6,200: POKE S+4,W
16 FOR X = 0 TO 255 STEP (RND(TI)*15)+1
17 POKE S,X :POKE S+1,255-X
18 FOR Y = 0 TO 33: NEXT Y,X
19 FOR X = 0 TO 200: NEXT: POKE S+24,0
20 FOR X = 0 TO 100: NEXT: GOTO 10
21 REM *** ABORT ONLY WITH RUN/STOP ! ***
*/
struct VSid : Module {

	enum VoiceParamIds {
		FREQ,
		FREQ_ATT,
		OCT,
		PW,
		PW_ATT,
		ENV_A,
		ENV_D,
		ENV_S,
		ENV_R,
		WAVE_NOISE,
		WAVE_PULSE,
		WAVE_SAW,
		WAVE_TRI,
		WAVE_RING,
		WAVE_SYNC,
		FILTER_ENABLE,
		NUM_VOICE_PARAMS
	};

	enum ParamIds {
		VOICE1_PARAM_BASE,
		VOICE2_PARAM_BASE = VOICE1_PARAM_BASE + NUM_VOICE_PARAMS,
		VOICE3_PARAM_BASE = VOICE2_PARAM_BASE + NUM_VOICE_PARAMS,
		FILT_EXT,
		FILT_HP,
		FILT_BP,
		FILT_LP,
		FILT_CUTOFF,
		FILT_CUTOFF_ATT,
		FILT_RES,
		FILT_RES_ATT,
		MUTE3,
		WAVEFORM, // fixme temporary
		NUM_PARAMS
	};

	enum VoiceInputIds {
		GATE,
		FREQ_CV,
		FREQ_CV2,
		PW_CV,
		SYNC,
		NUM_VOICE_INPUTS
	};

	enum InputIds {
		VOICE1_INPUT_BASE,
		VOICE2_INPUT_BASE = VOICE1_INPUT_BASE + NUM_VOICE_INPUTS,
		VOICE3_INPUT_BASE = VOICE2_INPUT_BASE + NUM_VOICE_INPUTS,
		FILT_CUTOFF_CV,
		FILT_RES_CV,
		MUTE3_CV,
		EXT_AUDIO,
		NUM_INPUTS
	};

	enum OutputIds {
		ENV3_OUT,
		OSC3_OUT,
		AUDIO_OUT,
		NUM_OUTPUTS
	};

	enum LightIds {
		NUM_LIGHTS
	};

	VSid() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		sid.set_sampling_parameters(CLOCK, SAMPLE_RESAMPLE_INTERPOLATE, engineGetSampleRate(), 20000, 1.0);
		onSampleRateChange();
	}
	void step() override;
	void onSampleRateChange() override;

	SID sid;
	long clock = 0;

};


void VSid::onSampleRateChange() {
	sid.adjust_sampling_frequency(engineGetSampleRate());
	sid.enable_filter(true);
	sid.write(0x17,3|(15<<4)); // routing to 3 and resonant
	sid.write(0x18,15|(1<<4)); // set volume to 15


	for(int base = 0; base <= 0x0e; base+=7 ) {
		sid.write(base+3,4); // pw
		sid.write(base+4,5);

		sid.write(base+5,0); // AD
		sid.write(base+6,56); // SR
	}

}


void VSid::step() {

	for(int base = 0; base <= 0x0e; base+=7 ) {
		sid.write(base+0,0); // freq
		sid.write(base+1, params[VOICE1_PARAM_BASE + FREQ].value);
		sid.write(base+4, int(params[WAVEFORM].value)| (clock++ % 100000 == 0 ? 1 : 0)); // control register
	}


	sid.write(0x15,0); // cutoff low
	sid.write(0x16,int(params[FILT_CUTOFF].value)); // cutoff high


	sid.clock(CLOCK/engineGetSampleRate());

	{
		int16_t sample = sid.read(0x1b); // use osc 3
		float value = 100.f * MAX_VOLTAGE*float(sample)/(float)SHRT_MAX;
		outputs[OSC3_OUT].value = value;
	}
	{
		int16_t sample = sid.output(16);
		float value = MAX_VOLTAGE*float(sample)/(float)SHRT_MAX;;
		outputs[AUDIO_OUT].value = value;
	}

}


struct VSidWidget : ModuleWidget {
	VSidWidget(VSid *module);
	void createVoice(int yOffset, int baseParam, int baseInput);
};


void VSidWidget::createVoice(int yOffset, int baseParam, int baseInput)  {
	addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(RACK_GRID_WIDTH, yOffset), module, baseParam + VSid::FREQ, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<CKSSThree>(Vec(75, yOffset), module, VSid::OCT, 0.0f, 2.0f, 1.0f));
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(100, yOffset), module, baseParam + VSid::PW, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(135, yOffset), module, baseParam + VSid::ENV_A, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(165, yOffset), module, baseParam + VSid::ENV_D, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(195, yOffset), module, baseParam + VSid::ENV_S, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(225, yOffset), module, baseParam + VSid::ENV_R, 0, 255, 0)); // FIXME range

	const int yOffset2 = yOffset + 35;

	addParam(ParamWidget::create<Trimpot>(Vec(75, yOffset2), module, baseParam + VSid::FREQ_ATT, 0, 255, 0)); // FIXME range
	addParam(ParamWidget::create<Trimpot>(Vec(105, yOffset2), module, baseParam + VSid::PW_ATT, 0, 255, 0)); // FIXME range

	addParam(ParamWidget::create<CKSS>(Vec(135, yOffset2), module, VSid::WAVE_NOISE, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<CKSS>(Vec(150, yOffset2), module, VSid::WAVE_PULSE, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<CKSS>(Vec(165, yOffset2), module, VSid::WAVE_SAW, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<CKSS>(Vec(180, yOffset2), module, VSid::WAVE_TRI, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<CKSS>(Vec(195, yOffset2), module, VSid::WAVE_RING, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<CKSS>(Vec(210, yOffset2), module, VSid::WAVE_SYNC, 0.0f, 1.0f, 0.0f));

	addInput(Port::create<PJ301MPort>(Vec(box.size.x-3*RACK_GRID_WIDTH, yOffset2), Port::INPUT, module, baseInput + VSid::SYNC));

	const int yOffset3 = yOffset2 + 25;

	addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH, yOffset3), Port::INPUT, module, baseInput + VSid::GATE));
	addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH*3, yOffset3), Port::INPUT, module, baseInput + VSid::FREQ_CV));
	addInput(Port::create<PJ301MPort>(Vec(70, yOffset3), Port::INPUT, module, baseInput + VSid::FREQ_CV2));
	addInput(Port::create<PJ301MPort>(Vec(100, yOffset3), Port::INPUT, module, baseInput + VSid::PW_CV));

	{
		MyLabel* const cLabel = new MyLabel(18);
		cLabel->box.pos = Vec(195/2, (yOffset3+ 10)/2); // coordinate system is broken FIXME
		cLabel->color = nvgRGB(0,0,0);
		cLabel->text = "& ▮ ◢ ▲ O 1  rst";
		addChild(cLabel);
	}

}

VSidWidget::VSidWidget(VSid *module) : ModuleWidget(module) {
	box.size = Vec(18 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	Panel* panel = new LightPanel();
	panel->box.size = box.size;
	panel->backgroundColor = COLOR_WHITE;
	addChild(panel);

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

	for(int i = 0; i < NUM_VOICES+1; i++) {
		const int yOffset = RACK_GRID_WIDTH + i*100;
		int paramBase, inputBase;
		switch(i) {
			case 0:
				paramBase = VSid::VOICE1_PARAM_BASE;
				inputBase = VSid::VOICE1_INPUT_BASE;
				break;
			case 1:
				paramBase = VSid::VOICE2_PARAM_BASE;
				inputBase = VSid::VOICE2_INPUT_BASE;
				break;
			case 2:
				paramBase = VSid::VOICE3_PARAM_BASE;
				inputBase = VSid::VOICE3_INPUT_BASE;
				break;
			case 3:
				// FIXME layout the filter row
				continue;
			default:
				assert(false);
		}
		createVoice(yOffset, paramBase, inputBase);
	}

	/*
	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH, RACK_GRID_WIDTH), module, VSid::VOICE1_PARAM_BASE + VSid::FREQ, 0, 255, 0));
	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH, 3*RACK_GRID_WIDTH), module, VSid::WAVEFORM, 0, 255, 0));

	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH, 5*RACK_GRID_WIDTH), module, VSid::FILT_CUTOFF, 0, 255, 0));
	*/

	addOutput(Port::create<PJ301MPort>(Vec(box.size.x-3*RACK_GRID_WIDTH, box.size.y - 5*RACK_GRID_WIDTH), Port::OUTPUT, module, VSid::OSC3_OUT));
	addOutput(Port::create<PJ301MPort>(Vec(box.size.x-3*RACK_GRID_WIDTH, box.size.y - 3*RACK_GRID_WIDTH), Port::OUTPUT, module, VSid::AUDIO_OUT));

}


Model *modelSid = Model::create<VSid, VSidWidget>("Pulsum Quadratum", "SID", "SID Synthesizer", SYNTH_VOICE_TAG);
