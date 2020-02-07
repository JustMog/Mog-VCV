#include "plugin.hpp"
#include <math.h>

#define PI 3.14159265

//TODO:
//----------------
//low priority:

//attenuvert input + knob add instead of override? 
//changeable skins (light / dark)
//expansions??
//----------------

enum PolyMode {
    ROTATE_MODE,
    RESET_MODE,
    FIXED_MODE,
    NUM_POLY_MODES
};

#define NODE_NUM_INS 2
#define NODE_NUM_OUTS 4

struct OutputRouter;

struct Node{

    Param* knob;
    Light* light;
    Input* input1;
    Output* output1;
    Param* bypassBtn;
	dsp::PulseGenerator outPulses[NODE_NUM_OUTS];
	dsp::SchmittTrigger inTriggers[NODE_NUM_INS][16];

    int id;
	int state = -1;
	float lightBrightness = 0.f;

    OutputRouter* outputRouter;   
    
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

    void process(float dt){
        //light
		light->setSmoothBrightness(lightBrightness, dt);
		
		//send pulse outputs
		for(int out = 0; out < NODE_NUM_OUTS; out++){
			getOutput(out)->setVoltage(outPulses[out].process(dt)*10.f);
		}

		//trigger on inputs
		bool triggered = false;
		for(int in = 0; in < NODE_NUM_INS; in++)
			for(int ch = 0; ch < getInput(in)->getChannels(); ch++)
				if(inTriggers[in][ch].process(getInput(in)->getVoltage(ch)))
					triggered = true; 		

		if(triggered) trigger();
		
    }


    void advanceState(){
        state++;
        if(state >= NODE_NUM_OUTS) state = -1;
    }

    void trigger(){

        for(int i = 0; i < NODE_NUM_OUTS + 1; i++){
            if(state == -1){
                if(bypassBtn == nullptr or not bypassBtn->getValue()){		
					play();
                    advanceState();
                    return;
                }
            }
            else if(getOutput(state)->isConnected()){

				outPulses[state].trigger(1e-3f);//trigger length recommended by vcv voltage standards
                advanceState();
                return; 
            }
            advanceState();
        }
    }

    void reset(){
        state = -1;
    }

	void play(); 

};

struct OutputRouter{
    int numChannels = 1;
    Node* channels[16];
    
    PolyMode polyMode = FIXED_MODE;
    int rotateIndex = -1;

	Param* bipolar;
	Param* attenuversionParam;
	Input* attenuversionInput;

	Param* gateLenParam;
	Input* gateLenInput;	
	dsp::PulseGenerator gatePulses[16];
	dsp::PulseGenerator retrigPulses[16];

    Output* cvOut;
    Output* gateOut;
	Output* retrigOut;

    void init(	
		Param* _bipolar,
		Param* _attenuversionParam,
		Input* _attenuversionInput,

		Param* _gateLenParam,
		Input* _gateLenInput,	

		Output* _cvOut,
		Output* _gateOut,
		Output* _retrigOut
	){
		bipolar = _bipolar;
		attenuversionParam = _attenuversionParam;
		attenuversionInput = _attenuversionInput;

		gateLenParam =  _gateLenParam;
		gateLenInput =  _gateLenInput;

        cvOut = _cvOut;
        gateOut = _gateOut;
		retrigOut = _retrigOut;

		for(int i = 0; i < 16; i++) channels[i] = nullptr;

    }

	void setPolyMode(PolyMode mode){
		polyMode = mode;
		rotateIndex = -1;
	}

	void setChannels(int n){
		numChannels = n;
		cvOut->setChannels(numChannels);
		gateOut->setChannels(numChannels);
		retrigOut->setChannels(numChannels);
		
		if(polyMode == ROTATE_MODE && rotateIndex >= numChannels) rotateIndex = -1;
		
		for(int ch = numChannels; ch < 16; ch++){
			clearChannel(ch);
		}
	}


    void process(float dt){
		
		//have to do it here because of reasons
		cvOut->setChannels(numChannels);
		gateOut->setChannels(numChannels);
		retrigOut->setChannels(numChannels);	

		float attenuversion = attenuversionInput->isConnected() ? 
			attenuversionInput->getVoltage()/10 :
			attenuversionParam->getValue();
		
		float cvMin, cvMax;
		if(bipolar->getValue()){
			cvMin = -5*attenuversion;
			cvMax =  5*attenuversion;
		}
		else{
			cvMin = 0;
			cvMax = 10*attenuversion;
		}
		

		for(int ch = 0; ch < numChannels; ch++){

			retrigOut->setVoltage(retrigPulses[ch].process(dt) * 10.f, ch);
	
			if(gatePulses[ch].process(dt)){
				float out = channels[ch]->knob->getValue();
				out = rescale(out, 0.f, 1.f, cvMin, cvMax);
				cvOut->setVoltage(out, ch);
			}
			else clearChannel(ch);	

		}

    }

	void clearChannel(int c){
		if(channels[c] != nullptr)
			channels[c]->lightBrightness = 0.f;
		gateOut->setVoltage(0.f, c);	
		gatePulses[c].reset();
		channels[c] = nullptr;
	}

	void playNode(Node* node){
		for(int ch = 0; ch < numChannels; ch++){
			if(channels[ch] == node) clearChannel(ch);
		}
		int ch = getChannel(node);
		clearChannel(ch);

		channels[ch] = node;
		retrigPulses[ch].trigger(1e-3f);
		gateOut->setVoltage(10.f, ch);
		gatePulses[ch].trigger(channelGetGateLen(ch));
		node->lightBrightness = 1.f;

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

	float channelGetGateLen(int ch){
		float res = pow(11.f, gateLenParam->getValue()) -1;
		
		if(gateLenInput->getChannels() == 1)
			res *= gateLenInput->getVoltage()/10;	
		else if(ch < gateLenInput->getChannels())
			res *= gateLenInput->getVoltage(ch)/10;
		
		res = std::max(res, 0.f);
		return res;
	}


};

inline void Node::play(){
	outputRouter->playNode(this);
}

struct Network : Module {
	enum ParamIds {
		ENUMS(VAL_PARAM, 4 * 4),
		ENUMS(BYPASS_PARAM, 2),
		ATTENUVERSION_PARAM,
		BIPOLAR_PARAM,
		GATE_LEN_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIG_INPUT, NODE_NUM_INS * 4 * 4),
		ATTENUVERSION_INPUT,
		GATE_LEN_INPUT,
        ENUMS(RESET_INPUT,6),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUT, NODE_NUM_OUTS * 4 * 4),
		CV_OUTPUT,
		GATE_OUTPUT,
		RETRIG_OUTPUT,
		//DEBUG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(TRIG_LIGHT, 4 * 4),
		NUM_LIGHTS
	};

    Node nodes[4*4];
    OutputRouter outputRouter;

    dsp::SchmittTrigger resetTriggers[6];

    Network() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);		
        
		configParam(ATTENUVERSION_PARAM, -1.f, 1.f, 0.8f, "CV Attenuversion", "%", 0, 100);
        configParam(BIPOLAR_PARAM, 0.f, 1.f, 1.f, "CV Bipolar");
		configParam(GATE_LEN_PARAM, 0.f, 1.f, 0.169092083673438f, "Gate Length", "s", 11.f, 1.f, -1.f);//10.f, 10.f/9.f, -(10.f/9.f));	

        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");	
		configParam(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass");	
		configParam(BYPASS_PARAM+1, 0.f, 1.f, 0.f, "Bypass");	
       
	    outputRouter.init(
			&params[BIPOLAR_PARAM],
			&params[ATTENUVERSION_PARAM],
			&inputs[ATTENUVERSION_INPUT],
			&params[GATE_LEN_PARAM],
			&inputs[GATE_LEN_INPUT],
			&outputs[CV_OUTPUT], 
			&outputs[GATE_OUTPUT], 
			&outputs[RETRIG_OUTPUT]
		);
        
		int bypass = 0;
		for(int i = 0; i < 4*4; i++){
            configParam(VAL_PARAM + i, 0.f, 1.f, 0.5f, "");		
            nodes[i].init(
				i, 
                &params[VAL_PARAM+i], 
                &lights[TRIG_LIGHT+i], 
                &inputs[TRIG_INPUT+(i*NODE_NUM_INS)], 
                &outputs[TRIG_OUTPUT+(i*NODE_NUM_OUTS)], 
                &outputRouter,
				(i == 0 or i == 8) ? &params[BYPASS_PARAM+bypass++] : nullptr
            );

        }

    }

    void process(const ProcessArgs& args) override {
        //reset
		for(int i = 0; i < 6; i++)		
			if(params[RESET_PARAM].getValue() or resetTriggers[i].process(inputs[RESET_INPUT+i].getVoltage())){
				for(int node = 0; node < 16; node++)
					nodes[node].reset();
			}

		for(int node = 0; node < 4*4; node++){			
            nodes[node].process(args.sampleTime);
        }

        outputRouter.process(args.sampleTime);
        
    }

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "channels", json_integer(outputRouter.numChannels));
		json_object_set_new(rootJ, "polyMode", json_integer(outputRouter.polyMode));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* channelsJ = json_object_get(rootJ, "channels");
		if (channelsJ)
			outputRouter.setChannels(json_integer_value(channelsJ));

		json_t* polyModeJ = json_object_get(rootJ, "polyMode");
		if (polyModeJ)
			outputRouter.setPolyMode((PolyMode) json_integer_value(polyModeJ));
		
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

			cy += border -3;

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
				
				knobLights[nodesDone] = createLightCentered<KnobLight>(mm2px(Vec(cx,cy)), module, Network::TRIG_LIGHT + nodesDone);
				addChild(knobLights[nodesDone]);			
 			
				addParam(createParamCentered<KnobTransparent>(mm2px(Vec(cx,cy)), module, Network::VAL_PARAM+nodesDone));
				
				if( j == 0 && i % 2 == 0){
					x = cos(PI*2/6)*dist*2;
					y = sin(PI*2/6)*dist*2;
					addParam(createParamCentered<PushButtonLarge>(mm2px(Vec(cx-x,cy+y)), module, Network::BYPASS_PARAM+bypassesDone++));
				}
				nodesDone++;
			}
		}

		dist = 8;
		cx = 18.960155; cy = 111.20094;
		for(int k = 0; k < 6; k++){
			angle = (k+0.5)* (PI*2/6);
			x = cos(angle)*dist;
			y = sin(angle)*dist;

			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cx+x, cy+y)), module, Network::RESET_INPUT+k));
		}
		addParam(createParamCentered<PushButtonMomentaryLarge>(mm2px(Vec(cx, cy)), module, Network::RESET_PARAM));
		
		addParam(createParamCentered<KnobTransparentDotted>(mm2px(Vec(43.443615, 109.08425)), module, Network::ATTENUVERSION_PARAM));
		addParam(createParamCentered<RockerSwitchHorizontal>(mm2px(Vec(52.209492, 115.00568)), module, Network::BIPOLAR_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.77454, 107.494)), module, Network::ATTENUVERSION_INPUT));
		
		addParam(createParamCentered<KnobTransparentDotted>(mm2px(Vec(67.05793, 112.74732)), module, Network::GATE_LEN_PARAM));	
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(78.388855, 111.15707)), module, Network::GATE_LEN_INPUT));

		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(115.15991, 108.12705)), module, Network::CV_OUTPUT));
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(105.50927, 113.84677)), module, Network::RETRIG_OUTPUT));
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(115.15991, 120.06448)), module, Network::GATE_OUTPUT));

		//addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(4,4)), module, Network::DEBUG_OUTPUT));

	}

	void step() override {
		Network *module = dynamic_cast<Network*>(this->module);

		if (module) {
			for(int node = 0; node < 4*4; node++){
				if(module->nodes[node].bypassBtn != nullptr && module->nodes[node].bypassBtn->getValue()){
					knobLights[node]->bgColor = nvgRGB(0x10,0x00,0x00); 	
				}
				else{ 
					knobLights[node]->bgColor = nvgRGB(0x3B, 0x3B, 0x3B);
				}
			}
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		Network* module = dynamic_cast<Network*>(this->module);

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
