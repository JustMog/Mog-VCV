#pragma once

using namespace rack;
extern Plugin* pluginInstance;


struct PushButtonMomentary : SvgSwitch {
	PushButtonMomentary() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_on.svg")));
		momentary = true;
	}
};

struct PushButtonMomentaryLarge : SvgSwitch {
	PushButtonMomentaryLarge() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_on.svg")));
		momentary = true;
	}
};

struct PushButtonLarge : SvgSwitch {
	PushButtonLarge() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_on.svg")));
		momentary = false;
	}
};

struct PushButtonLargeTransparent : SvgSwitch {
	PushButtonLargeTransparent() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/pushbutton_large_transparent_on.svg")));
		momentary = false;
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

struct KnobTransparent : RoundKnob {
	KnobTransparent() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob_transparent.svg")));
	}
};

struct KnobTransparentDotted : RoundKnob {
	KnobTransparentDotted() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob_transparent_dotted.svg")));
	}
};

struct KnobLight : app::ModuleLightWidget {

	KnobLight() {
		this->box.size = app::mm2px(math::Vec(5.731,5.731));//11.077,11.077));
		this->bgColor = nvgRGB(0x3b, 0x3b, 0x3b); //nvgRGB(0x0e, 0x69, 0x77);
		this->addBaseColor(nvgRGB(0xff, 0xff, 0xff)); //nvgRGB(0xff, 0xcc, 0x03));
	}

	void drawLight(const widget::Widget::DrawArgs& args) override {
		float radius = std::min(this->box.size.x, this->box.size.y) / 2.0;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, radius, radius, radius);

		// Background
		if (this->bgColor.a > 0.0) {
			nvgFillColor(args.vg, this->bgColor);
			nvgFill(args.vg);
		}

		// Foreground
		if (this->color.a > 0.0) {
			nvgFillColor(args.vg, this->color);
			nvgFill(args.vg);
		}

	}
};
