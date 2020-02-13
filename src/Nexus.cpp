#include "plugin.hpp"
#include <math.h>

#define PI 3.14159265

#define NUM_STAGES 6

struct Stage{
	int counter = 0;
	Param* knob;
    Light* light;
    Input* input;
    Output* output;
	Output* next;
	dsp::SchmittTrigger inputTriggers[16];
	dsp::Timer suppressTrigsTimer;
	
	bool inConnected = false;
	bool nextConnected = false;

	bool done = false;
	float lightBrightness = 0.f;
	
	void init(Param* _knob, Light* _light, Input* _input, Output* _output, Output* _next){
		knob = _knob;
		light = _light;
		input = _input;
		output = _output;
		next = _next;
		reset();
	}

	void reset(){
		counter = 0;
		done = false;
		lightBrightness = 0.f;
		for(int i = 0; i < 16; i++){
			output->setVoltage(0.f,i);
			next->setVoltage(0.f,i);
			inputTriggers[i].process(0.f);
		}

	}

};



struct Nexus : Module {
	enum ParamIds {
		ENUMS(REPS_PARAM, NUM_STAGES),
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIG_INPUT, NUM_STAGES),
        ENUMS(RESET_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUT, NUM_STAGES),
		ENUMS(NEXT_OUTPUT, NUM_STAGES),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHT, NUM_STAGES),
		NUM_LIGHTS
	};

    dsp::SchmittTrigger resetTrigger[2][16];
	dsp::BooleanTrigger resetBtnTrigger;
	Stage stages[NUM_STAGES];	

	dsp::Timer resetTimer;

    Nexus() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RESET_PARAM, 0, 1, 0, "Reset");		 
		for(int stage = 0; stage < NUM_STAGES; stage++){
			configParam(REPS_PARAM + stage, 1, 99, 4, string::f("Stage %d reps", stage+1), "x");
			stages[stage].init(
				&params[REPS_PARAM+stage],
				&lights[STEP_LIGHT+stage], 
				&inputs[TRIG_INPUT+stage],
				&outputs[TRIG_OUTPUT+stage], 
				&outputs[NEXT_OUTPUT+stage]
			);
		}	
    }

	float getInput(int stageNum, int ch){
		Stage* s = &stages[stageNum];

		if(s->input->isConnected())
			return s->input->getVoltage(ch);
		else if(stageNum > 0 && stages[stageNum-1].done && not stages[stageNum-1].next->isConnected())
			return getInput(stageNum-1, ch);
		else return 0;
	}

	int getNumChannels(int stageNum){
		Stage* s = &stages[stageNum];

		if(s->input->isConnected())
			return s->input->getChannels();
		else if(stageNum > 0 && not stages[stageNum-1].next->isConnected())
			return getNumChannels(stageNum-1);
		else return 0;
	}

	bool allTrigsLow(int stageNum){
		for (int ch = 0; ch < 16; ch++)
			if(stages[stageNum].inputTriggers[ch].isHigh()) 
				return false;
		return true;
	}

	void reset(){
		for(int stage = 0; stage < NUM_STAGES; stage++){
			stages[stage].reset();	
			stages[stage].suppressTrigsTimer.time = 1;
		}
		resetTimer.reset();
	}

    void process(const ProcessArgs& args) override {

		for(int stage = 0; stage < NUM_STAGES; stage++){
			
			Stage* s = &stages[stage];	
		
			//process input			
			bool doTrigger = false;

			int numChannels = getNumChannels(stage);
			s->output->setChannels(numChannels);
			s->next->setChannels(numChannels);
			
			s->suppressTrigsTimer.process(args.sampleTime);

			if(resetTimer.process(args.sampleTime) > 1e-3f && s->suppressTrigsTimer.time > 1e-3f){


				for (int ch = 0; ch < 16; ch++){		
					float val = getInput(stage, ch);		
					val = rescale(val, 0.1f, 2.f, 0.f, 1.f);//obey voltage stadards for triggers
					if(s->inputTriggers[ch].process(val)) doTrigger = true;
				}
			
				if(doTrigger) s->suppressTrigsTimer.reset();

				if(not s->done && doTrigger){			
					s->counter ++;
					if(s->counter > s->knob->getValue()){
						//done now
						s->reset();
						s->done = true; 				
					}
				}
				
				if(not s->done){
					//still going. to output.		
					for (int ch = 0; ch < 16; ch++)
						s->output->setVoltage(getInput(stage, ch),ch);
					
					float v = allTrigsLow(stage) ? 0.f : 10.f;	
					s->lightBrightness = v/10.f;
				}
				else{
					//done. 
					if(s->next->isConnected()){
						//"next" output connected, forward there.
						for (int ch = 0; ch < 16; ch++)
							s->next->setVoltage(getInput(stage, ch),ch);
					}
					else{
						//"next" not connected. fall back to "normal" routing
						if(stage == NUM_STAGES -1)
							reset();
					}
				}
			}
			s->light->setSmoothBrightness(s->lightBrightness, args.sampleTime);

		}

		//important that this happens last
		if(resetBtnTrigger.process(params[RESET_PARAM].getValue())){
			reset();
		}

		for(int in = 0; in < 2; in++){
			for (int ch = 0; ch < 16; ch++){
				float val = inputs[RESET_INPUT+in].getVoltage(ch);
				val = rescale(val, 0.1f, 2.f, 0.f, 1.f);//obey voltage stadards for triggers
				if(resetTrigger[in][ch].process(val))
					reset();		
			}
		}

    }	

};


struct Readout : TransparentWidget{
	Nexus *module;
	Knob *knob;
	std::shared_ptr<Font> font;

	Readout()
	{
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Exo2-BoldItalic.ttf"));
	}

	void draw(const DrawArgs &args) override
	{
		nvgSave(args.vg);

		std::string text_to_display = "";

		int xOff = 0;
		if(module && knob){
			int val = (int)knob->paramQuantity->getDisplayValue();
			text_to_display = std::to_string(val);
			if(val < 10) xOff = 4;
		}

		nvgFontSize(args.vg, 16);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0);
		nvgFillColor(args.vg, nvgRGBA(0xff,0xff,0xff,0xff));
		
		nvgTextBox(args.vg, xOff, 0, 32, text_to_display.c_str(), NULL);
		nvgRestore(args.vg);
	}

};


struct NexusWidget : ModuleWidget {
	LightWidget *knobLights[NUM_STAGES];

	NexusWidget(Nexus* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Nexus.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		float x, y;
		float xBorder = 7;
		float yBorder = 13.5;

		LightWidget* light;
	
		for(int stage = 0; stage < NUM_STAGES; stage++){
			y = yBorder + ((stage/3) * 51);

			x = ((35.56 - (xBorder*2)) / 2) * (stage % 3);
			x += xBorder;
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, Nexus::TRIG_INPUT+stage));
												
			//x += xSpacing*2;	
			y += 14;		
			light = createLightCentered<KnobLight>(mm2px(Vec(x+2,y+2)), module, Nexus::STEP_LIGHT+stage);
			light->box.pos.x -= 4;
			light->box.pos.y -= 4; 
			knobLights[stage] = light;
			addChild(light);

			auto k = createParamCentered<KnobTransparentSmall>(mm2px(Vec(x,y)), module, Nexus::REPS_PARAM+stage);
			dynamic_cast<Knob*>(k)->snap = true;
			addParam(k);

			Readout *readout = new Readout();
			readout->box.pos = mm2px(Vec(x-2.5, y-5.35));
			readout->box.size = mm2px(Vec(20, 20)); // bounding box of the widget
			readout->module = module;
			readout->knob = k;
			addChild(readout);

			//x += xSpacing;
			y += 10;
			addOutput(createOutputCentered<RoundJackOut>(mm2px(Vec(x, y)), module, Nexus::TRIG_OUTPUT+stage));
			
			//x += xSpacing;
			y += 10;
			addOutput(createOutputCentered<RoundJackOut>(mm2px(Vec(x, y)), module, Nexus::NEXT_OUTPUT+stage));
									
		}

		x = 35.56/2.f;
		y = 128.5 - 15;
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x + 10, y)), module, Nexus::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x - 10, y)), module, Nexus::RESET_INPUT+1));					
		addParam(createParamCentered<PushButtonMomentaryLarge>(mm2px(Vec(x, y)), module, Nexus::RESET_PARAM));

	
	}

};

Model* modelNexus = createModel<Nexus, NexusWidget>("Nexus");
