#include "plugin.hpp"
#include <math.h>

#define PI 3.14159265

//polyphic inputs! 
//pulse mode?
//attenuvert input + knob add instead of override? different scaling?
//changeable skins (light / dark)
//expansions??


enum PolyMode {
    ROTATE_MODE,
    RESET_MODE,
    FIXED_MODE,
    NUM_POLY_MODES
};

struct OutputRouter{
    int channels = 16;
    bool channelOccupied[16];
    
    PolyMode polyMode = RESET_MODE;
    int rotateIndex = -1;

    Output* cvOut;
    Output* gateOut;

	float cvMin = 0;
	float cvMax = 10;

    void init(Output* cv, Output* gate){
        cvOut = cv;
        gateOut = gate;
        for(int i = 0; i < 16; i++) channelOccupied[i] = false;
    }

    void process(bool bipolar, float attenuversion){
        cvOut->setChannels(channels);
		gateOut->setChannels(channels);

		if(bipolar){
			cvMin = -5*attenuversion;
			cvMax = 5*attenuversion;
		}
		else{
			cvMin = 0;
			cvMax = 10*attenuversion;
		}
    }

   	void setPolyMode(PolyMode mode){
		polyMode = mode;
		rotateIndex = -1;
	}
	void setChannels(int n){
		channels = n;
		if(polyMode == ROTATE_MODE && rotateIndex < channels -1) rotateIndex = -1;
	}

    int assignChannel(int node){
        int c = getFreeChannel(node);
        channelOccupied[c] = true;
        return c;
    }

    int getFreeChannel(int node) {	
        if (channels == 1)
			return 0;

		switch (polyMode) {

			case ROTATE_MODE: {
				// Find next available channel
				for (int i = 0; i < channels; i++) {
					rotateIndex++;
					if (rotateIndex >= channels)
						rotateIndex = 0;
					if (not channelOccupied[rotateIndex])
						return rotateIndex;
				}
				// No notes are available. Advance rotateIndex once more.
				rotateIndex++;
				if (rotateIndex >= channels)
					rotateIndex = 0;
				return rotateIndex;
			} break;

			case RESET_MODE: {
				for (int c = 0; c < channels; c++) {
					if (not channelOccupied[c])
						return c;
				}
				return channels - 1;
			} break;

			case FIXED_MODE: {
				return node;
			} break;

			default: return 0;
		}
	}

};

const int NODE_NUM_INS = 2;
const int NODE_NUM_OUTS = 4;


struct Node{
    Param* knob;
    Light* light;
    Input* input1;
    Output* output1;
    Param* bypassBtn;

    int id;
    int state = -1;
    bool bypass = false;

	const float LIGHT_FADE = 2;
	const float TRIG_THRESHOLD = 0.00001;

    float inputsPrev[NODE_NUM_INS];
    int toChannel = 0;
    Output* relayTo = nullptr;
    OutputRouter* outputRouter;
    dsp::SchmittTrigger bypassTrigger;
    
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

    void closeRelay(){   
        if(relayTo == outputRouter->gateOut){
            outputRouter->channelOccupied[toChannel] = false;
        }
        
        if(relayTo != nullptr)
            relayTo->setVoltage(0.f, toChannel);  
        relayTo = nullptr;
        
        toChannel = 0;
    }

    void process(float dt){
        if(bypassBtn != nullptr && bypassTrigger.process(bypassBtn->getValue())){
            bypass =! bypass;
            if(bypass) closeRelay();
        }
		light->setBrightness(light->getBrightness()-LIGHT_FADE*dt);
    
        float maxVoltage = -1;
        for (int in = 0; in < NODE_NUM_INS; in++){

            float newVal = getInput(in)->getVoltage();
            if(newVal > maxVoltage) maxVoltage = newVal;
            
            if(inputsPrev[in] <= TRIG_THRESHOLD && newVal > TRIG_THRESHOLD){
                trigger();                                       
            }
            inputsPrev[in] = newVal;

        }

        if (relayTo != nullptr){

            if(maxVoltage <= 0){
                closeRelay();
            }
            else{
                relayTo->setVoltage(maxVoltage, toChannel);

                if(relayTo == outputRouter->gateOut){
					light->setBrightness(std::max(maxVoltage/10.f, light->getBrightness()-LIGHT_FADE*dt));
					float out = knob->getValue();
					out = rescale(out, 0.f, 1.f, outputRouter->cvMin, outputRouter->cvMax);
                    outputRouter->cvOut->setVoltage(out, toChannel);
                }
            }
        }

    }

    void advanceState(){
        state++;
        if(state >= NODE_NUM_OUTS) state = -1;
    }
    
    void trigger(){
        closeRelay();

        for(int i = 0; i < NODE_NUM_OUTS + 1; i++){
            if(state == -1){
                if(not bypass){
                    relayTo = outputRouter->gateOut;
                    toChannel = outputRouter->assignChannel(id);                   
                    
                    advanceState();
                    return;
                }
            }
            else if(getOutput(state)->isConnected()){
                relayTo = getOutput(state);
                advanceState();
                return; 
            }
            advanceState();
        }
    }

    void reset(){
        state = -1;
    }
};



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
        
		configParam(ATTENUVERSION_PARAM, -1.f, 1.f, 0.4f, "CV Attenuversion", "%", 0, 100);
        configParam(BIPOLAR_PARAM, 0.f, 1.f, 1.f, "CV Bipolar");		
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");	
		configParam(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass");	
		configParam(BYPASS_PARAM+1, 0.f, 1.f, 0.f, "Bypass");	
       
	    outputRouter.init(&outputs[CV_OUTPUT], &outputs[GATE_OUTPUT]);
        
		int bypass = 0;
		for(int i = 0; i < 4*4; i++){
            configParam(VAL_PARAM + i, 0.f, 1.f, 0.5f, "");		
            nodes[i].init(i, 
                &params[VAL_PARAM+i], 
                &lights[TRIG_LIGHT+i], 
                &inputs[TRIG_INPUT+(i*2)], 
                &outputs[TRIG_OUTPUT+(i*4)], 
                &outputRouter,
				(i == 0 or i == 8) ? &params[BYPASS_PARAM+bypass++] : nullptr
            );

        }

    }

    void process(const ProcessArgs& args) override {
        for(int i = 0; i < 6; i++)		
			if(params[RESET_PARAM].getValue() or resetTriggers[i].process(inputs[RESET_INPUT+i].getVoltage())){
				for(int node = 0; node < 16; node++)
					nodes[node].reset();
			}

        outputRouter.process(
			params[BIPOLAR_PARAM].getValue() > 0.f,
			inputs[ATTENUVERSION_INPUT].isConnected() ?
			inputs[ATTENUVERSION_INPUT].getVoltage() :
			params[ATTENUVERSION_PARAM].getValue()
		);
        
        for(int node = 0; node < 4*4; node++){			
            nodes[node].process(args.sampleTime);
        }
    }

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "channels", json_integer(outputRouter.channels));
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
			if (channels == 1)
				item->text = "Monophonic";
			else
				item->text = string::f("%d", channels);
			item->rightText = CHECKMARK(module->outputRouter.channels == channels);
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
				
				knobLights[nodesDone] = createLightCentered<KnobLight>(mm2px(Vec(cx,cy)), module, Network::TRIG_LIGHT + nodesDone);
				addChild(knobLights[nodesDone]);			
 			
				addParam(createParamCentered<KnobTransparent>(mm2px(Vec(cx,cy)), module, Network::VAL_PARAM+nodesDone));
				
				if( j == 0 && i % 2 == 0){
					x = cos(PI*2/6)*dist*2;
					y = sin(PI*2/6)*dist*2;
					addParam(createParamCentered<PushButtonMomentaryLarge>(mm2px(Vec(cx-x,cy+y)), module, Network::BYPASS_PARAM+bypassesDone++));
				}
				nodesDone++;
			}
		}

		dist = 8;
		cx = border; cy = 128.5 - (border*0.8);
		for(int k = 0; k < 6; k++){
			angle = (k+0.5)* (PI*2/6);
			x = cos(angle)*dist;
			y = sin(angle)*dist;

			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cx+x, cy+y)), module, Network::RESET_INPUT+k));
		}
		addParam(createParamCentered<PushButtonMomentaryLarge>(mm2px(Vec(cx, cy)), module, Network::RESET_PARAM));
		
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(116.8, cy-3.5)), module, Network::CV_OUTPUT));
		addOutput(createOutputCentered<RoundJackOutRinged>(mm2px(Vec(109.5, cy+3.5)), module, Network::GATE_OUTPUT));

		cy -= 3.5;
		addParam(createParamCentered<RockerSwitchVertical>(mm2px(Vec(43.463, cy)), module, Network::BIPOLAR_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47, cy+8.1)), module, Network::ATTENUVERSION_INPUT));
		addParam(createParamCentered<KnobTransparentDotted>(mm2px(Vec(53, cy)), module, Network::ATTENUVERSION_PARAM));
		
	}

	void step() override {
		Network *module = dynamic_cast<Network*>(this->module);

		if (module) {
			for(int node = 0; node < 4*4; node++){
				if(module->nodes[node].bypass){
					knobLights[node]->bgColor = nvgRGB(0x50,0x00,0x00); 	
				}
				else{ 
					knobLights[node]->bgColor = nvgRGB(0x20, 0x20, 0x20);
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
		channelItem->rightText = string::f("%d", module->outputRouter.channels) + "  " + RIGHT_ARROW;
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
