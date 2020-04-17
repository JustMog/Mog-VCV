#pragma once

using namespace rack;
extern Plugin* pluginInstance;


#define MOG_RED 	    nvgRGB(153, 36, 68)
#define MOG_ORANGE 		nvgRGB(226, 126, 88)
#define MOG_YELLOW 		nvgRGB(232, 202, 30)
#define MOG_GREEN 		nvgRGB(172, 214, 45)
#define MOG_BLUE 		nvgRGB(76, 146, 207)
#define MOG_PURPLE 		nvgRGB(187, 179, 216)
#define MOG_WHITE 		nvgRGB(230, 230, 230)
#define MOG_GREY_LIGHT 	nvgRGB(138, 138, 138)
#define MOG_GREY_MED 	nvgRGB(77, 77, 77)
#define MOG_GREY_DARK 	nvgRGBA(58, 58, 58, 255)
#define MOG_BLACK 		nvgRGB(26, 26, 26)


struct PushButtonTiny : SvgSwitch {
	PushButtonTiny() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_tiny_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_tiny_on.svg")));
	}
};

struct PushButtonLarge : SvgSwitch {
	PushButtonLarge() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_on.svg")));
	}
};

struct RockerSwitchHorizontal: SvgSwitch {
	RockerSwitchHorizontal() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/rocker_h_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/rocker_h_on.svg")));
	}
};

struct RockerSwitchVertical: SvgSwitch {
	RockerSwitchVertical() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/rocker_v_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/rocker_v_on.svg")));
	}
};

struct RoundJackOut : SVGPort {
	RoundJackOut() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/socket_round_dark.svg")));
	}
};

struct RoundJackOutRinged : SVGPort {
	RoundJackOutRinged() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/socket_round_dark_ring.svg")));
	}
};

struct KnobLarge: RoundKnob {
	KnobLarge() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob.svg")));
	}
};

struct KnobSmall : RoundKnob {
	KnobSmall() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob_small.svg")));
	}
};


struct ButtonLight : app::ModuleLightWidget {

	ButtonLight() {
		this->box.size = app::mm2px(math::Vec(4.686,4.686));
		this->bgColor = MOG_GREY_MED;
		this->addBaseColor(MOG_WHITE);
	}



};

struct ButtonLightTiny : app::ModuleLightWidget {

	ButtonLightTiny() {
		this->box.size = app::mm2px(math::Vec(3.485-0.627*2,3.485-0.62782));
		this->bgColor = MOG_GREY_MED;
		this->addBaseColor(MOG_WHITE);
	}

};

struct KnobLight : app::ModuleLightWidget {
	KnobLight() {
		this->box.size = app::mm2px(math::Vec(5.731,5.731));
		this->bgColor = MOG_GREY_DARK;
		this->addBaseColor(MOG_WHITE);
	}
};

struct KnobLightSmall : app::ModuleLightWidget {
	KnobLightSmall() {
		this->box.size = app::mm2px(math::Vec(3.909,3.909));
		this->bgColor = MOG_GREY_DARK;
		this->addBaseColor(MOG_WHITE);
	}
};

