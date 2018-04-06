#include "PQ.hpp"
#include <climits>
#include "sid.h"
#define MAX_VOLTAGE 5.f
#define CLOCK 1022730
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
	enum ParamIds {
		FREQ,
		WAVEFORM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
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
	const int base = 0x0e;

};


void VSid::onSampleRateChange() {
	sid.adjust_sampling_frequency(engineGetSampleRate());
	//sid.enable_filter(true);

	sid.write(0x15,2); // cutoff low
	sid.write(0x16,2); // cutoff high
	sid.write(0x17,4); // routing to voice 3
	sid.write(0x18,16); // set volume to 15 + lopass

	sid.write(0x15,15); // filter cutoff
	sid.write(0x16,3);

	// third voice
	sid.write(base+3,4); // pw
	sid.write(base+4,5);

	sid.write(base+5,0); // AD
	sid.write(base+6,5); // SR

}


void VSid::step() {

	sid.write(base+0,0); // freq
	sid.write(base+1,127);
	sid.write(base+1, params[FREQ].value);
	sid.write(base+4, int(params[WAVEFORM].value)| (clock++ % 10000 == 0 ? 1 : 0)|2); // control register

	sid.clock(CLOCK/engineGetSampleRate());
	sid.enable_filter(false);
	int16_t sample = sid.output();
	sample = sid.read(0x1b); // use osc 3
	float value = 100.f * MAX_VOLTAGE*float(sample)/(float)SHRT_MAX;
	outputs[AUDIO_OUT].value = value;
}


struct VSidWidget : ModuleWidget {
	VSidWidget(VSid *module);
};

VSidWidget::VSidWidget(VSid *module) : ModuleWidget(module) {
	box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	Panel* panel = new LightPanel();
	panel->box.size = box.size;
	panel->backgroundColor = COLOR_WHITE;
	addChild(panel);

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));
	addOutput(Port::create<PJ301MPort>(Vec(3*RACK_GRID_WIDTH, box.size.y - 2*RACK_GRID_WIDTH), Port::OUTPUT, module, VSid::AUDIO_OUT));
	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH, RACK_GRID_WIDTH), module, VSid::FREQ, 0, 255, 0));
	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH, box.size.y - 2*RACK_GRID_WIDTH), module, VSid::WAVEFORM, 0, 255, 0));

}


Model *modelSid = Model::create<VSid, VSidWidget>("Pulsum Quadratum", "SID", "SID Synthesizer", SYNTH_VOICE_TAG);
