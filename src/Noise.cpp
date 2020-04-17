#include "plugin.hpp"


#include "lib/FastNoise/FastNoise.h"

//#include "lib/FastNoise/FastNoise.cpp"

struct Noise : Module {

	enum ParamIds {
		X_PARAM,
		Y_PARAM,
		Z_PARAM,
		X_TRIM_PARAM,
		Y_TRIM_PARAM,
		Z_TRIM_PARAM,
		FREQ_PARAM,
		SEED_SET_PARAM,
		ATTENUVERT_PARAM,
		BIPOLAR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		X_INPUT,
		Y_INPUT,
		Z_INPUT,
		FREQ_INPUT,
		SEED_INPUT,
		SEED_SET_INPUT,
		ATTENUVERT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		SEED_SET_LIGHT,
		NUM_LIGHTS
	};

	FastNoise noise; 
	FastNoise displayNoise;
	bool safeDisplay = true;
	int displayChannel = 0;


	float freq = 0.f;
	float freqMin = 0.09f;
	float freqMax = 0.35f;
	float seed = 309.f;

    dsp::SchmittTrigger setSeedTrigger[16];
	dsp::BooleanTrigger setSeedBtnTrigger;
	float setSeedLightBrightness = 0.f; 


    Noise() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);		
        
		configParam(X_PARAM, -10.f, 10.f, 0.f, "X");
        configParam(Y_PARAM, -10.f, 10.f, 0.f, "Y");
		configParam(Z_PARAM, -10.f, 10.f, 0.f, "Z");

		configParam(X_TRIM_PARAM, 0.f, 1.f, 1.f, "X input trim", "%", 0.f, 100.f);
		configParam(Y_TRIM_PARAM, 0.f, 1.f, 1.f, "Y input trim", "%", 0.f, 100.f);
		configParam(Z_TRIM_PARAM, 0.f, 1.f, 1.f, "Z input trim", "%", 0.f, 100.f);

		configParam(FREQ_PARAM, 0.f, 10.f, 0.f, "Frequency");
		configParam(SEED_SET_PARAM, 0.f, 1.f, 0.f, "Set seed");

		configParam(ATTENUVERT_PARAM, -1.f, 1.f, 1.f, "CV Attenuversion");
		configParam(BIPOLAR_PARAM, 0.f, 1.f, 1.f, "CV Bipolar");

		noise.SetNoiseType(FastNoise::Simplex); 
		noise.SetSeed(309);
		displayNoise.SetNoiseType(FastNoise::Simplex); 
		displayNoise.SetSeed(309);

    }


	void setSeed(){
		setSeedLightBrightness = 10.f;
		if(inputs[SEED_INPUT].isConnected())
			seed = inputs[SEED_INPUT].getVoltage();
		else
			seed = random::uniform()*10;
		seed *= 1000000;

		noise.SetSeed((int)seed);
	}

	int getOutputPolyphony(){
		int maxCh = std::max(inputs[X_INPUT].getChannels(), inputs[Y_INPUT].getChannels());
		maxCh = std::max(maxCh,inputs[Z_INPUT].getChannels());
		maxCh = std::max(1,maxCh);
		return maxCh;
	}

    void process(const ProcessArgs& args) override {
        			
		//SEED
		setSeedLightBrightness = 0.f;
		//trig input
		for(int ch = 0; ch < 16; ch++){
			float val = inputs[SEED_SET_INPUT].getPolyVoltage(ch);	
			val = rescale(val, 0.1f, 2.f, 0.f, 1.f);//obey voltage stadards for triggers
			if(setSeedTrigger[ch].process(val)){
				setSeed();
			}
		}
		//btn
		if(setSeedBtnTrigger.process(params[SEED_SET_PARAM].getValue())){
			setSeed();
		}
	
		lights[SEED_SET_LIGHT].setSmoothBrightness(setSeedLightBrightness, args.sampleTime);

		//FREQ
		freq = inputs[FREQ_INPUT].getVoltage() + params[FREQ_PARAM].getValue(); 
		freq = rescale(freq, 0.f, 10.f, freqMin, freqMax);
		noise.SetFrequency(freq);
		
		//poly channels
		int maxCh = getOutputPolyphony();
		outputs[CV_OUTPUT].setChannels(maxCh);	

		//attenuversion
		float vert = params[ATTENUVERT_PARAM].getValue();
		if(inputs[ATTENUVERT_INPUT].isConnected()){
			vert *= inputs[ATTENUVERT_INPUT].getVoltage()/10.f;
		}	
   
		for(int ch = 0; ch < maxCh; ch++){
			float X = params[X_PARAM].getValue() + inputs[X_INPUT].getPolyVoltage(ch) * params[X_TRIM_PARAM].getValue();
			float Y = params[Y_PARAM].getValue() + inputs[Y_INPUT].getPolyVoltage(ch) * params[Y_TRIM_PARAM].getValue();
			float Z = params[Z_PARAM].getValue() + inputs[Z_INPUT].getPolyVoltage(ch) * params[Z_TRIM_PARAM].getValue();
			
			float out = noise.GetNoise(X,Y,Z);
			out *= 5.f * vert;

			if(not params[BIPOLAR_PARAM].getValue()) out += 5.f;

			outputs[CV_OUTPUT].setVoltage(out, ch);
		}

		//processExpander(args);

    }

	// void processExpander(const ProcessArgs& args) {

	// 	if (rightExpander.module and rightExpander.module->model == modelNoiseRoll) {
	// 		//lights[EXPANDER_LIGHT].setBrightness(1.f);
			
	// 		float* message = (float*) rightExpander.module->leftExpander.producerMessage;			

	// 		message[0] = seed;
	// 		message[1] = freq;
	// 		message[2] = params[BIPOLAR_PARAM].getValue();
	// 		float Z = getInput(&inputs[Z_INPUT], &params[Z_PARAM], 0);
	// 		Z = rescale(Z, 0.f, 10.f, 0.f, zScale);
	// 		message[3] = Z;
			
	// 		for(int ch = 0; ch < 16; ch++){
	// 			message[4+ch] = getInput(&inputs[X_INPUT], &params[X_PARAM], ch);
	// 			message[4+16+ch] = getInput(&inputs[Y_INPUT], &params[Y_PARAM], ch);
	// 		}
			
	// 		// Flip messages at the end of the timestep
	// 		rightExpander.module->leftExpander.messageFlipRequested = true;
			
	// 	} else {
	// 		//lights[EXPANDER_LIGHT].setBrightness(0.f);
	// 	}
	// }

	
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "seed", json_real(seed));
		json_object_set_new(rootJ, "displayChannel", json_integer(displayChannel));
		json_object_set_new(rootJ, "safeDisplay", json_integer(safeDisplay));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* seedJ = json_object_get(rootJ, "seed");
		if (seedJ){
			seed = json_real_value(seedJ);
			noise.SetSeed((int)seed);
		}
		json_t* displayChJ = json_object_get(rootJ, "displayChannel");
		if (displayChJ){
			displayChannel = json_integer_value(displayChJ);
		}
		json_t* displayModeJ = json_object_get(rootJ, "safeDisplay");
		if (displayModeJ){
			safeDisplay = json_integer_value(displayModeJ);
		}
	}

};


struct NoiseDisplay : Widget {
	Noise* module;
	static const int resolution = 120;
	NVGcolor colors[6] = {
		MOG_RED,
		MOG_ORANGE,
		MOG_YELLOW,
		MOG_GREEN,
		MOG_BLUE,
		MOG_PURPLE
	};
	int frameCtr;
	static const int seedUpdateInterval = 25;
	
	NoiseDisplay(){
		frameCtr = 0;
	}

	//todo: allow set xy & gate with click
	// void onButton(const event::Button &e) override {
	// 	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
	// 		e.consume(this);
	// 		float x = rescale(e.pos.x, 0.f, box.size.x, -10.f, 10.f)
	// 		module->params[X_PARAM].setValue()= Vec(e.pos.x, e.pos.y);
	// 		//todo: gate
	// 	}
	// }
	
	// void onDragStart(const event::DragStart &e) override {
	// 	//module->displayClick = Vec(e.pos.x, e.pos.y);
	// }

	// void onDragMove(const event::DragMove &e) override {
	// 	module->displayClick = module->displayClick.plus(e.mouseDelta.div(2));
	// 	module->displayClick.x = clamp(module->displayClick.x, 0.f, box.size.x);
	// 	module->displayClick.y = clamp(module->displayClick.y, 0.f, box.size.y);
	// }

	void draw(const DrawArgs &args) override {

		if(module){
			if(frameCtr > 0) frameCtr--;
			if(!module->safeDisplay) frameCtr = 0;

			bool updateSeed = module->safeDisplay && frameCtr <= 0;
			updateSeed |= not module->safeDisplay;

			float seed = (int)(module->seed); 
			if(updateSeed && seed != module->displayNoise.GetSeed() ){
				module->displayNoise.SetSeed(seed);
				frameCtr = seedUpdateInterval;
			}
			float freq = module->params[Noise::FREQ_PARAM].getValue();
			if(not module->safeDisplay) freq += module->inputs[Noise::FREQ_INPUT].getVoltage();
			freq = rescale(freq, 0.f, 10.f, module->freqMin, module->freqMax);
			module->displayNoise.SetFrequency(freq);	
			
			//noise
			float val;
			float x, y, nX, nY;
			float cellW = box.size.x/resolution;
			float cellH = box.size.y/resolution;

			float cx = module->params[Noise::X_PARAM].getValue();
			float cy = module->params[Noise::Y_PARAM].getValue();

			nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);

			float z;
			if(module->safeDisplay)
				z = module->params[Noise::Z_PARAM].getValue();
			else{
				z = module->params[Noise::Z_PARAM].getValue() + 
					module->inputs[Noise::Z_INPUT].getPolyVoltage(module->displayChannel) * 
					module->params[Noise::Z_TRIM_PARAM].getValue();
			}
				
			for(int col = 0; col < resolution; col++)
			for(int row = 0; row < resolution; row++){
			
				x = col*cellW;
				y = row*cellH;
				
				nX = rescale(x, 0, box.size.x, -10.f + cx, 10.f + cx); //(x/box.size.x)*10;
				nY = rescale(y, 0, box.size.y, -10.f + cy, 10.f + cy); //(y/box.size.y)*10;
								
				val = module->displayNoise.GetNoise(nX, nY, z);
					
				val = rescale(val, -1.f, 1.f, 0.f, 255.f);
				nvgFillColor(args.vg, nvgRGB(val, val, val)); 
				nvgStrokeColor(args.vg, nvgRGB(val, val, val)); 

				nvgBeginPath(args.vg);
				nvgRect(args.vg, x, y, cellW+0.4, cellH + 0.4);
				nvgFill(args.vg);

			}

			//balls
			// float r = 2.5f;
			
			// for(int ch = 0; ch < module->inputs[NoiseRoll::GATE_INPUT].getChannels(); ch++){
			// 	if(module->gateTriggers[ch].state){
			// 		Ball ball = module->balls[ch];
			// 		x = rescale(ball.pos.x, 0.f, 10.f, 0.f, box.size.x);
			// 		//x = clamp(x, r, box.size.x-r);
			// 		y = rescale(ball.pos.y, 0.f, 10.f, 0.f, box.size.y);
			// 		//y = clamp(y, r, box.size.y-r);

					
			// 		nvgFillColor(args.vg, ballColors[ch%6]); 
			// 		nvgStrokeColor(args.vg, nvgRGB(255, 255, 0)); 
			// 		nvgBeginPath(args.vg);
					
			// 		nvgCircle(args.vg, x, y, r);
			// 		nvgFill(args.vg);
			// 	}
			// }

			//crosshair center


			
			nvgStrokeWidth(args.vg, 1.f); 	
			x = box.size.x/2.f;
			y = box.size.y/2.f;
			
			for(int r = -2; r <= 2; r ++){
				if(r == 0) continue;
				nvgBeginPath(args.vg);
				if (abs(r) == 1) nvgStrokeColor(args.vg, MOG_WHITE);
				else if(abs(r) == 2)  nvgStrokeColor(args.vg, MOG_BLACK);

				nvgMoveTo(args.vg, x+r, y);
				nvgLineTo(args.vg, x+r*2, y);
				nvgMoveTo(args.vg, x, y+r);
				nvgLineTo(args.vg, x, y+r*2);
				
				nvgStroke(args.vg);
			}
			
			
			//crosshair actual
			float r = 2.f;

			for(int ch = 0; ch < module->getOutputPolyphony(); ch++){
				
				x = module->inputs[Noise::X_INPUT].getPolyVoltage(ch) * 
					module->params[Noise::X_TRIM_PARAM].getValue()
				;
				x = rescale(x, -10.f, 10.f, 0.f, box.size.x);
				y = module->inputs[Noise::Y_INPUT].getPolyVoltage(ch) * 
					module->params[Noise::Y_TRIM_PARAM].getValue()
				;
				y = rescale(y, -10.f, 10.f, 0.f, box.size.y);

				if(ch == module->displayChannel)
					nvgStrokeColor(args.vg, MOG_WHITE); 
				else
					nvgStrokeColor(args.vg, colors[ch%6]); 
				nvgStrokeWidth(args.vg, 1.f); 
					
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, x, y, r);
				nvgStroke(args.vg);

				nvgStrokeColor(args.vg, MOG_BLACK); 
				nvgStrokeWidth(args.vg, 1.f); 
					
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, x, y, r+1);
				nvgStroke(args.vg);
				
			}


		}
	}
};

struct DisplayChannelValueItem : MenuItem {
	Noise* module;
	int channel;
	void onAction(const event::Action& e) override {
		if(module) module->displayChannel = channel;
	}
};


struct DisplayChannelMenuItem : MenuItem {
	Noise* module;
	Menu* createChildMenu() override {
		Menu* menu = new Menu;
		for (int channel = 0; channel < std::max(1, module->inputs[Noise::Z_INPUT].getChannels()); channel++) {
			DisplayChannelValueItem* item = new DisplayChannelValueItem;

			item->text = string::f("%d", channel+1);
			item->rightText = CHECKMARK(module->displayChannel == channel);
			item->module = module;
			item->channel = channel;
			menu->addChild(item);
		}
		return menu;
	}
};


struct safeDisplayModeMenuItem : MenuItem {
	Noise* module;
	void onAction(const event::Action& e) override {
		module->safeDisplay = !(module->safeDisplay);
	}
};



struct NoiseWidget : ModuleWidget {


	NoiseWidget(Noise* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Noise.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
			
		
		// mm2px(Vec(33.867, 33.867))
		NoiseDisplay* display = createWidget<NoiseDisplay>(mm2px(Vec(3.387, 4.445)));
		display->module = module;
		display->box.size = mm2px(Vec(33.867,33.867));
		addChild(display);
		
		
		addParam(createParamCentered<KnobLarge>(mm2px(Vec(7.8845778, 52.36515)), module, Noise::X_PARAM));
		addParam(createParamCentered<KnobLarge>(mm2px(Vec(20.32, 52.36515)), module, Noise::Y_PARAM));
		addParam(createParamCentered<KnobLarge>(mm2px(Vec(32.755421, 52.36515)), module, Noise::Z_PARAM));

		addParam(createParamCentered<KnobSmall>(mm2px(Vec(7.8845778, 62.784054)), module, Noise::X_TRIM_PARAM));
		addParam(createParamCentered<KnobSmall>(mm2px(Vec(20.32, 62.784054)), module, Noise::Y_TRIM_PARAM));
		addParam(createParamCentered<KnobSmall>(mm2px(Vec(32.755421, 62.784054)), module, Noise::Z_TRIM_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.8845778, 71.842499)), module, Noise::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 71.842499)), module, Noise::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.755421, 71.842499)), module, Noise::Z_INPUT));

		addParam(createParamCentered<KnobLarge>(mm2px(Vec(32.755424, 83.330902)), module, Noise::FREQ_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 83.330902)), module, Noise::FREQ_INPUT));
		
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.8845778, 102.66243)), module, Noise::SEED_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 102.66243)), module, Noise::SEED_SET_INPUT));
			
		auto b = createParamCentered<PushButtonLarge>(mm2px(Vec(32.755421, 102.66243)), module, Noise::SEED_SET_PARAM);
		dynamic_cast<Switch*>(b)->momentary = true;
		addParam(b);
		addChild(createLightCentered<ButtonLight>(mm2px(Vec(32.755421, 102.66243)), module, Noise::SEED_SET_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.8845778, 119.99696)), module, Noise::ATTENUVERT_INPUT));
		addParam(createParamCentered<KnobSmall>(mm2px(Vec(17.326338, 119.99696)), module, Noise::ATTENUVERT_PARAM));
		addParam(createParamCentered<RockerSwitchVertical>(mm2px(Vec(25.028788, 119.99696)), module, Noise::BIPOLAR_PARAM));

		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(32.755421, 119.99696)), module, Noise::CV_OUTPUT));
		
	}

	void appendContextMenu(Menu* menu) override {
		Noise* module = dynamic_cast<Noise*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(new MenuSeparator());

		DisplayChannelMenuItem* channelItem = new DisplayChannelMenuItem;
		channelItem->text = "Noise display Z from channel";
		channelItem->rightText = string::f("%d", module->displayChannel+1) + "  " + RIGHT_ARROW;
		channelItem->module = module;
		menu->addChild(channelItem);

		menu->addChild(new MenuEntry);

		MenuLabel* m = new MenuLabel;
		m->text = "!!PHOTOSENSITIVITY WARNING!!";
		menu->addChild(m);
		
		safeDisplayModeMenuItem* displayItem = new safeDisplayModeMenuItem;
		displayItem->module = module;
		displayItem->text = "Display reflect cv input";
		displayItem->rightText = CHECKMARK(module->safeDisplay == false);
		menu->addChild(displayItem);

	}

};

Model* modelNoise = createModel<Noise, NoiseWidget>("Noise");
