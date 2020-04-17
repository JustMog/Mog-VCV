#include "plugin.hpp"
#include <math.h>

#define PI 3.14159265

//TODO:
//attenuvert input + knob add instead of override? different scaling?
//changeable skins (light / dark)
//expansions??

enum PolyMode {
    ROTATE_MODE,
    RESET_MODE,
    FIXED_MODE,
    NUM_POLY_MODES
};

const int NODE_NUM_INS = 2;
const int NODE_NUM_OUTS = 4;

struct OutputRouter;

struct Node{
    Param* knob;
    Light* light;
    Input* input1;
    Output* output1;
    Param* bypassBtn;

    int id;
    int state = -2;
    bool bypass = false;
   	dsp::SchmittTrigger inputTriggers[NODE_NUM_INS][16];
	dsp::Timer suppressTrigsTimer;
    
    OutputRouter* outputRouter;
	float lightBrightness = 0.f;
	bool doReset = false;
    
    void init(int _id, Param* _knob, Light* _light, Input* _input, Output* _output, OutputRouter* _out, Param* _bypassBtn = nullptr){
        id = _id;
        knob = _knob;
        light = _light;
        input1 = _input;
        output1 = _output;
        outputRouter = _out;
		bypassBtn = _bypassBtn;

    }

    Input * getInput(int n){
        return (input1+n);
    }

    Output * getOutput(int n){
        return (output1+n);
    }

	bool isBypass(){
		return bypassBtn != nullptr && bypassBtn->getValue();
	}

	bool allTrigsLow(){
		for (int in = 0; in < NODE_NUM_INS; in++)
			for (int ch = 0; ch < 16; ch++)
				if(inputTriggers[in][ch].isHigh()) 
					return false;
		return true;
	}

    void process(float dt, bool sampleHold, float expanderCV){

		light->setSmoothBrightness(lightBrightness, dt);

		bool doTrigger = false;   
		for (int in = 0; in < NODE_NUM_INS; in++){
			for (int ch = 0; ch < 16; ch++){							
				float val = getInput(in)->getVoltage(ch);
				val = rescale(val, 0.1f, 2.f, 0.f, 1.f);//obey voltage stadards for triggers
				if(inputTriggers[in][ch].process(val)) doTrigger = true;
			}
        }
		if(suppressTrigsTimer.process(dt) > 1e-3f && doTrigger) trigger(sampleHold, expanderCV);  
        		
		if (state >= 0)
			getOutput(state)->setVoltage(allTrigsLow() ? 0.f : 10.f);
		else if(state == -1 && allTrigsLow())
			stop();	

    }
  
    void trigger(bool sampleHold, float expanderCV){
		//ignore trigs that are too close together
		suppressTrigsTimer.reset();

		//stop current state's gate output
		if(state >= 0)
			getOutput(state)->setVoltage(0.f);
		else if(state == -1) 
			stop();

		if(doReset){
			state = -2;
			doReset = false;
		}

        for(int i = 0; i < NODE_NUM_OUTS + 1; i++){
            advanceState();
			if(state == -1){
                if(not isBypass()){
					if(sampleHold && expanderCV >= 0.f)
						knob->setValue(expanderCV);
					play();
                    return;
                }
            }
            else if(getOutput(state)->isConnected()){
        		return; 
            }
        }
		//no action available
		reset();
    }

    void advanceState(){
        state++;
        if(state >= NODE_NUM_OUTS) state = -1;
    }

	void play();

	void stop();

    void reset(){
        doReset = true;
    }

};

struct OutputRouter{
    int numChannels = 16;
    Node* channels[16];
    
    PolyMode polyMode = RESET_MODE;
    int rotateIndex = -1;

    Output* cvOut;
    Output* gateOut;
	Output* retrigOut;

	float cvMin = 0;
	float cvMax = 10;

	dsp::PulseGenerator retrigPulses[16];


    void init(Output* cv, Output* gate, Output* retrig){
        cvOut = cv;
        gateOut = gate;
		retrigOut = retrig;
        for(int i = 0; i < 16; i++) channels[i] = nullptr;
    }

    void process(float dt, bool bipolar, float attenuversion){
        cvOut->setChannels(numChannels);
		gateOut->setChannels(numChannels);
		retrigOut->setChannels(numChannels);

		if(bipolar){
			cvMin = -5*attenuversion;
			cvMax = 5*attenuversion;
		}
		else{
			cvMin = 0;
			cvMax = 10*attenuversion;
		}
		
		for(int ch = 0; ch < numChannels; ch++){
			if(channels[ch] != nullptr){
				Node* node = channels[ch];

				float out = node->knob->getValue();
				out = rescale(out, 0.f, 1.f, cvMin, cvMax);
				cvOut->setVoltage(out, ch);
				node->lightBrightness = 1.f;		
			}
			retrigOut->setVoltage(retrigPulses[ch].process(dt)*10.f,ch);
		}

    }

   	void setPolyMode(PolyMode mode){
		polyMode = mode;
		rotateIndex = -1;
	}
	void setChannels(int n){
		numChannels = n;
		for(int i = n; i < 16; i++) closeChannel(i);
		if(polyMode == ROTATE_MODE && rotateIndex > numChannels -1) rotateIndex = -1;
	}

    void playNode(Node* node){
        int c = getChannel(node);
		closeChannel(c);
        channels[c] = node;
		gateOut->setVoltage(10.f, c);
		retrigPulses[c].trigger();
    }

	void stopNode(Node* node){
		for(int ch = 0; ch < 16; ch++)
			if(channels[ch] == node) closeChannel(ch);
	}

	int getChannel(Node* node) {	
        if (numChannels == 1)
			return 0;

		switch (polyMode) {

			case ROTATE_MODE: {
				// Find next available channel
				for (int i = 0; i < numChannels; i++) {
					rotateIndex++;
					if (rotateIndex >= numChannels)
						rotateIndex = 0;
					if (channels[rotateIndex] == nullptr)
						return rotateIndex;
				}
				// No notes are available. Advance rotateIndex once more.
				rotateIndex++;
				if (rotateIndex >= numChannels)
					rotateIndex = 0;
				return rotateIndex;
			} break;

			case RESET_MODE: {
				for (int c = 0; c < numChannels; c++) {
					if (channels[c] == nullptr)
						return c;
				}
				return numChannels - 1;
			} break;

			case FIXED_MODE: {
				return node->id;
			} break;

			default: return 0;
		}
	}

	void closeChannel(int ch){
		gateOut->setVoltage(0.f, ch);
		if(channels[ch] != nullptr) 
			channels[ch]->lightBrightness = 0.f;
		channels[ch] = nullptr;
	}


};

inline void Node::play(){
	outputRouter->playNode(this);
}

inline void Node::stop(){
	outputRouter->stopNode(this);
}

struct Network : Module {
	enum ParamIds {
		ENUMS(VAL_PARAM, 4 * 4),
		ENUMS(BYPASS_PARAM, 2),
		ATTENUVERSION_PARAM,
		BIPOLAR_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIG_INPUT, NODE_NUM_INS * 4 * 4),
		ATTENUVERSION_INPUT,
        ENUMS(RESET_INPUT,6),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUT, NODE_NUM_OUTS * 4 * 4),
		CV_OUTPUT,
		GATE_OUTPUT,
		RETRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(TRIG_LIGHT, 4 * 4),
		ENUMS(BYPASS_LIGHT, 2),
		RESET_LIGHT,
		NUM_LIGHTS
	};

    Node nodes[4*4];
    OutputRouter outputRouter;
	float resetLightBrightness = 0.f;

    dsp::SchmittTrigger resetTriggers[6];
	dsp::BooleanTrigger resetBtnTrigger;

	bool expanderHold;
	float expanderCV[16];

    Network() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);		
        
		configParam(ATTENUVERSION_PARAM, -1.f, 1.f, 0.4f, "CV Attenuversion", "%", 0, 100);
        configParam(BIPOLAR_PARAM, 0.f, 1.f, 1.f, "CV Bipolar");		
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");	
		configParam(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass");	
		configParam(BYPASS_PARAM+1, 0.f, 1.f, 0.f, "Bypass");	
       
	    outputRouter.init(&outputs[CV_OUTPUT], &outputs[GATE_OUTPUT], &outputs[RETRIG_OUTPUT]);
        
		int bypass = 0;
		for(int i = 0; i < 4*4; i++){
            configParam(VAL_PARAM + i, 0.f, 1.f, 0.5f, string::f("Node %d", i+1));		
            nodes[i].init(i, 
                &params[VAL_PARAM+i], 
                &lights[TRIG_LIGHT+i], 
                &inputs[TRIG_INPUT+(i*NODE_NUM_INS)], 
                &outputs[TRIG_OUTPUT+(i*NODE_NUM_OUTS)], 
                &outputRouter,
				(i == 0 or i == 8) ? &params[BYPASS_PARAM+bypass++] : nullptr
            );

        }

    }

	void resetNodes(){
		resetLightBrightness = 1.f;
		for(int node = 0; node < 16; node++)
			nodes[node].reset();
	}

	void processExpanderMessage(float* message){
		expanderHold = message[18];
		
		for(int node = 0; node < 16; node++){
			if(expanderHold)
				expanderCV[node] = message[node];
			else if(message[node] >= 0.f)
				params[VAL_PARAM+node].setValue(message[node]);
		}
		
		params[BYPASS_PARAM].setValue(message[16]);
		params[BYPASS_PARAM+1].setValue(message[17]);

	}
	
	
	void processExpander() {
		if(leftExpander.module and leftExpander.module->model == modelNetworkExpander) {
			float *message = (float*) leftExpander.module->rightExpander.consumerMessage;
			processExpanderMessage(message);

			leftExpander.module->rightExpander.messageFlipRequested = true;
			
			
		}
		else if(rightExpander.module and rightExpander.module->model == modelNetworkExpander) {
			float *message = (float*) rightExpander.module->leftExpander.consumerMessage;
			processExpanderMessage(message);

			rightExpander.module->leftExpander.messageFlipRequested = true;

		}
		else expanderHold = false;
	}


    void process(const ProcessArgs& args) override {
		processExpander();
        
		resetLightBrightness = 0.f;
		
		if(resetBtnTrigger.process(params[RESET_PARAM].getValue()))
			resetNodes();

		for(int i = 0; i < 6; i++){	
			float val = inputs[RESET_INPUT+i].getVoltage();	
			val = rescale(val, 0.1f, 2.f, 0.f, 1.f);//obey voltage stadards for triggers
			if(resetTriggers[i].process(val))
				resetNodes();		
		}
		lights[RESET_LIGHT].setSmoothBrightness(resetLightBrightness, args.sampleTime);

		
		if(inputs[ATTENUVERSION_INPUT].isConnected())
			params[ATTENUVERSION_PARAM].setValue(inputs[ATTENUVERSION_INPUT].getVoltage()/10.f);
		
		outputRouter.process(
			args.sampleTime,
			params[BIPOLAR_PARAM].getValue() > 0.f,
			params[ATTENUVERSION_PARAM].getValue()
		);

        
        for(int node = 0; node < 4*4; node++){			
            nodes[node].process(args.sampleTime, expanderHold, expanderCV[node]);
        }
		lights[BYPASS_LIGHT].setSmoothBrightness(nodes[0].isBypass() ? 1.f : 0.f, args.sampleTime);
		lights[BYPASS_LIGHT+1].setSmoothBrightness(nodes[8].isBypass() ? 1.f : 0.f, args.sampleTime);
    }

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "channels", json_integer(outputRouter.numChannels));
		json_object_set_new(rootJ, "polyMode", json_integer(outputRouter.polyMode));

		json_t *nodeStatesJ = json_array();
		for (int node = 0; node < 16; node++) {
			json_t *nodeStateJ = json_integer((int) nodes[node].state);
			json_array_append_new(nodeStatesJ, nodeStateJ);
		}
		json_object_set_new(rootJ, "nodeStates", nodeStatesJ);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* channelsJ = json_object_get(rootJ, "channels");
		if (channelsJ)
			outputRouter.setChannels(json_integer_value(channelsJ));

		json_t* polyModeJ = json_object_get(rootJ, "polyMode");
		if (polyModeJ)
			outputRouter.setPolyMode((PolyMode) json_integer_value(polyModeJ));
		
		json_t *nodeStatesJ = json_object_get(rootJ, "nodeStates");
		if (nodeStatesJ) {
			for (int node = 0; node < 16; node++) {
				json_t *nodeStateJ = json_array_get(nodeStatesJ, node);
				if (nodeStateJ)
					nodes[node].state = json_integer_value(nodeStateJ);
			}
		}
	}

	

};

struct ChannelValueItem : MenuItem {
	Network* module;
	int channels;
	void onAction(const event::Action& e) override {
		module->outputRouter.setChannels(channels);
	}
};


struct ChannelItem : MenuItem {
	Network* module;
	Menu* createChildMenu() override {
		Menu* menu = new Menu;
		for (int channels = 1; channels <= 16; channels++) {
			ChannelValueItem* item = new ChannelValueItem;
			if (channels == 1)
				item->text = "Monophonic";
			else
				item->text = string::f("%d", channels);
			item->rightText = CHECKMARK(module->outputRouter.numChannels == channels);
			item->module = module;
			item->channels = channels;
			menu->addChild(item);
		}
		return menu;
	}
};


struct PolyModeValueItem : MenuItem {
	Network* module;
	PolyMode polyMode;
	void onAction(const event::Action& e) override {
		module->outputRouter.setPolyMode(polyMode);
	}
};


struct PolyModeItem : MenuItem {
	Network* module;
	Menu* createChildMenu() override {
		Menu* menu = new Menu;
		std::vector<std::string> polyModeNames = {
			"Rotate",
			"Reset",
			"Fixed"
		};
		for (int i = 0; i < NUM_POLY_MODES; i++) {
			PolyMode polyMode = (PolyMode) i;
			PolyModeValueItem* item = new PolyModeValueItem;
			item->text = polyModeNames[i];
			item->rightText = CHECKMARK(module->outputRouter.polyMode == polyMode);
			item->module = module;
			item->polyMode = polyMode;
			menu->addChild(item);
		}
		return menu;
	}
};


struct NetworkWidget : ModuleWidget {
    LightWidget *knobLights[4*4];

	NetworkWidget(Network* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Network.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		float dist = 8.8;
		float x, y, angle, cx, cy;
		float border = 20;
		int nodesDone = 0;
        int insDone = 0;
        int outsDone = 0;
		int bypassesDone = 0;
		float angleOffset = 0;// PI/6;

		for(int i = 0; i < 4; i++){
			cy = (128.52-(border*2))/3*i;
			cy *= 0.75;

			cy += border;

			for(int j = 0; j < 4; j++){
				cx = (121.92-(border*2))/3*(j + (i % 2 ? 0.5 : 0));
				cx += border;
				
				
				for(int k = 0; k < 6; k++){
					angle = (k+3.5) * (PI*2/6);
					angle += angleOffset;  

					x = cos(angle)*dist;
					y = sin(angle)*dist;

					if( k < NODE_NUM_INS )
                        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cx+x, cy+y)), module, Network::TRIG_INPUT+insDone++));
                    else
                        addOutput(createOutputCentered<RoundJackOut>(mm2px(Vec(cx+x, cy+y)), module, Network::TRIG_OUTPUT+outsDone++));

                }						
							
				addParam(createParamCentered<KnobLarge>(mm2px(Vec(cx,cy)), module, Network::VAL_PARAM+nodesDone));
				knobLights[nodesDone] = createLightCentered<KnobLight>(mm2px(Vec(cx,cy)), module, Network::TRIG_LIGHT + nodesDone);
				addChild(knobLights[nodesDone]);		

				if( j == 0 && i % 2 == 0){
					x = cos(PI*2/6)*dist*2;
					y = sin(PI*2/6)*dist*2;
					addParam(createParamCentered<PushButtonLarge>(mm2px(Vec(cx-x,cy+y)), module, Network::BYPASS_PARAM+bypassesDone));
					addChild(createLightCentered<ButtonLight>(mm2px(Vec(cx-x,cy+y)), module, Network::BYPASS_LIGHT+bypassesDone));
					bypassesDone++;
				}
				nodesDone++;
			}
		}

		cx = border; cy = 128.5 - (border*0.8);

		auto b = createParamCentered<PushButtonLarge>(mm2px(Vec(cx, cy)), module, Network::RESET_PARAM);
		dynamic_cast<Switch*>(b)->momentary = true;
		addParam(b);

		addChild(createLightCentered<ButtonLight>(mm2px(Vec(cx,cy)), module, Network::RESET_LIGHT));
		

		dist = 8.2;	
		for(int k = 0; k < 6; k++){
			angle = (k+0.5)* (PI*2/6);
			x = cos(angle)*dist;
			y = sin(angle)*dist;

			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cx+x, cy+y)), module, Network::RESET_INPUT+k));
		}
		
		cy += 0.5f;
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(115, cy-5)), module, Network::CV_OUTPUT));
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(115, cy+5)), module, Network::GATE_OUTPUT));
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(106, cy)), module, Network::RETRIG_OUTPUT));
	
		cy -= 4.f;
		addParam(createParamCentered<RockerSwitchVertical>(mm2px(Vec(43.463, cy)), module, Network::BIPOLAR_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47, cy+8.1)), module, Network::ATTENUVERSION_INPUT));
		addParam(createParamCentered<KnobLarge>(mm2px(Vec(53, cy)), module, Network::ATTENUVERSION_PARAM));
		
	}

	void step() override {
		Network *module = dynamic_cast<Network*>(this->module);

		if (module) {
			for(int node = 0; node < 4*4; node++){
				if(module->nodes[node].isBypass()){
					knobLights[node]->bgColor = MOG_BLACK;//nvgRGB(0x10,0x00,0x00); 	
				}
				else{ 
					knobLights[node]->bgColor = MOG_GREY_DARK; //nvgRGB(0x3B, 0x3B, 0x3B);
				}
			}
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		Network* module = dynamic_cast<Network*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(new MenuSeparator());

		ChannelItem* channelItem = new ChannelItem;
		channelItem->text = "Polyphony channels";
		channelItem->rightText = string::f("%d", module->outputRouter.numChannels) + "  " + RIGHT_ARROW;
		channelItem->module = module;
		menu->addChild(channelItem);

		PolyModeItem* polyModeItem = new PolyModeItem;
		polyModeItem->text = "Polyphony mode";
		polyModeItem->rightText = RIGHT_ARROW;
		polyModeItem->module = module;
		menu->addChild(polyModeItem);


	}


};

Model* modelNetwork = createModel<Network, NetworkWidget>("Network");
