#include "plugin.hpp"

struct Quantizer : Module {
	enum ParamIds {
		ROOT_PARAM,
		MODE_PARAM,
		COLLAPSE_PARAM,
		TEST_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		ROOT_INPUT,
		MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float rootKnobPrevPos = 0.f;
	float root = 0.5f;

	Quantizer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		//configParam(ROOT_PARAM, 0, 11, 6, "Root", "", 0.f, 1.f, 1.f);
		configParam(ROOT_PARAM, -INFINITY, INFINITY, 0.f, "Root");
		configParam(MODE_PARAM, 0, 6, 1, "Mode", "", 0.f, 1.f, 1.f);
	}

	int getRoot(){		
		if(inputs[ROOT_INPUT].isConnected()){
			float in = inputs[ROOT_INPUT].getVoltage() * 12;
			while(in < 0){ in += 120000; }  //adding 12s until positive so modulo behaves correctly
			return (int)in % 12;
		}
		else{
			return (int)root % 12;
		}

	}


	void process(const ProcessArgs& args) override {

		if(inputs[ROOT_INPUT].isConnected() == false){
			root += (params[ROOT_PARAM].getValue() - rootKnobPrevPos) / 2.4f * 12;//why these numbers? good question
			//keep in sensible range, stay positive for modulo
			while(root >= 12){ root -= 12; } 
			while(root < 0){ root += 12; } 
			rootKnobPrevPos = params[ROOT_PARAM].getValue();
		}
		outputs[CV_OUTPUT].setVoltage(getRoot());
		outputs[TRIG_OUTPUT].setVoltage(root);
		
	}
	
	void onRandomize() override {
		root = (random::u32() % 12) + 0.5f;
	}

	void onReset() override {
		root = 0.5f;
	}


};

struct CenteredLabel : Widget {
	std::string text;
	int fontSize;
	CenteredLabel(int _fontSize = 12) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &args) override {
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGB(25, 0, 10));
		nvgFontSize(args.vg, fontSize);
		nvgText(args.vg, box.pos.x, box.pos.y, text.c_str(), NULL);
	}
};

struct QuantizerWidget : ModuleWidget {
	QuantizerWidget(Quantizer* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Quantizer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		CenteredLabel* const rootLabel = new CenteredLabel;
		rootLabel->box.pos = Vec(15, 22);
		rootLabel->text = "Root here";
		addChild(rootLabel);
		

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.17, 23)), module, Quantizer::ROOT_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.17, 33)), module, Quantizer::ROOT_INPUT));

		CenteredLabel* const modeLabel = new CenteredLabel;
		modeLabel->box.pos = Vec(15, 70);
		modeLabel->text = "Mode here";
		addChild(modeLabel);

		auto k = createParamCentered<RoundBlackKnob>(mm2px(Vec(10.17, 55)), module, Quantizer::MODE_PARAM);
		dynamic_cast<Knob*>(k)->snap = true;
		addParam(k);
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.17, 65)), module, Quantizer::MODE_INPUT));
		
		
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.17, 85)), module, Quantizer::CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(6.251-.5f, 93)), module, Quantizer::TRIG_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.089+.5f, 93)), module, Quantizer::CV_OUTPUT));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.17, 100)), module, Quantizer::TEST_PARAM));

	}

};

// void QuantizerWidget::appendContextMenu(Menu *menu) {
// 	Quantizer *quantizer = dynamic_cast<quantizer*>(module);
// 	menu->addChild(new MenuSeparator());

// 	// NSChannelItem *channelItem = new NSChannelItem;
// 	// channelItem->text = "Polyphony channels";
// 	// channelItem->rightText = string::f("%d", noteSeq->channels) + " " +RIGHT_ARROW;
// 	// channelItem->module = noteSeq;
// 	// menu->addChild(channelItem);
// }


Model* modelQuantizer = createModel<Quantizer, QuantizerWidget>("Quantizer");