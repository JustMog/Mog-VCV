#include "plugin.hpp"


struct NetworkExpander : Module {
	enum ParamIds {
		HOLD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUT, 16),
		ENUMS(BYPASS_INPUT, 2),
		HOLD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float leftMessages[2][16+2+1];
	float rightMessages[2][16+2+1];

	dsp::SchmittTrigger bypassTriggers[2];
	dsp::SchmittTrigger holdTrigger;

	NetworkExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(HOLD_PARAM, 0.f, 1.f, 0.f, "Sample & Hold");

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	
	float getCV(int node, int chOffset){
		if(inputs[CV_INPUT+node].isConnected())
			if( inputs[CV_INPUT+node].getChannels() > chOffset)
				return inputs[CV_INPUT+node].getPolyVoltage(chOffset) / 10.f;
			else
				return -1.f;
		else if(node > 0)
			return getCV(node-1, chOffset+1);
		else
			return -1.f;

	}
	
	
	void processExpanderMessage(float* message){
		for(int node = 0; node < 16; node ++)
			message[node] = getCV(node, 0);

		message[16] = bypassTriggers[0].isHigh() ? 10.f : 0.f;
		message[17] = bypassTriggers[1].isHigh() ? 10.f : 0.f;

		message[18] = params[HOLD_PARAM].getValue();

	}

	void process(const ProcessArgs& args) override {

		bypassTriggers[0].process(inputs[BYPASS_INPUT].getVoltage());
		bypassTriggers[1].process(inputs[BYPASS_INPUT+1].getVoltage());
		holdTrigger.process(inputs[HOLD_INPUT].getVoltage());
		if(inputs[HOLD_INPUT].isConnected())
			params[HOLD_PARAM].setValue(holdTrigger.isHigh() ? 1.f : 0.f);

		if (rightExpander.module and rightExpander.module->model == modelNetwork) {
	
			float* message = (float*) rightExpander.producerMessage;			
			processExpanderMessage(message);

			
			
		} 
		if (leftExpander.module and leftExpander.module->model == modelNetwork) {
	
			float* message = (float*) leftExpander.producerMessage;			
			processExpanderMessage(message);

			// Flip messages at the end of the timestep
			leftExpander.module->rightExpander.messageFlipRequested = true;
			
		} 
	
	}


};


struct NetworkExpanderWidget : ModuleWidget {
	NetworkExpanderWidget(NetworkExpander* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/NetworkExpander.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RockerSwitchVertical>(mm2px(Vec(16.450178, 86.096748)), module, NetworkExpander::HOLD_PARAM));

		int cv = 0;
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.0418444, 19.455309)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.986107, 19.455309)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.927036, 19.455309)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.874638, 19.455309)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.498113, 28.290989)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.371878, 28.290989)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36.457443, 28.290989)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.378242, 28.290989)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.0418444, 37.146969)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.986107, 37.146969)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.927036, 37.146969)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.874638, 37.146969)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.498113, 46.125298)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.371878, 46.125298)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36.457443, 46.125298)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.378242, 46.125298)), module, NetworkExpander::CV_INPUT+cv++));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.498113, 61.46711)), module, NetworkExpander::BYPASS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.041851, 70.32309)), module, NetworkExpander::BYPASS_INPUT+1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.041851, 86.096748)), module, NetworkExpander::HOLD_INPUT));
	}
};


Model* modelNetworkExpander = createModel<NetworkExpander, NetworkExpanderWidget>("NetworkExpander");