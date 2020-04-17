#pragma once
#include <rack.hpp>
#include "mog_components.hpp"


// Explicit <array> include required on OS X
#include <array> 

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;


// Declare each Model, defined in each module source file
// extern Model* modelMyModule;

extern Model* modelNetwork;
extern Model* modelNetworkExpander;
extern Model* modelNexus;
extern Model* modelNoise;


