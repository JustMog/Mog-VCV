Mog-VCV
===========================
Mog's modules for VCV Rack

**Network:**
---------------------------
![Network](/doc/network.png)

Network is a "node-based" polyphonic sequencer consisting of 16 identical nodes.

When a node first receives a trigger at either of its two inputs, it sends its value to the main cv out and a gate to the main gate out.  
On subsequent triggers, it relays the trigger to each of the node's connected outputs in turn.  
The process then repeats.
  
You can generate a repeating sequence by connecting nodes together.

![Demo](/doc/network_demo.gif)

**Polyphony:**  
Network features up to 16 polyphony channels and three different polyphony modes.  
"Rotate" and "Reset" modes function identically to the modes of the same name found on the VCV Core modules.  
Since Network deals in continuous voltages rather than discrete midi notes, the "Reuse" mode of those modules does not apply.  
Instead, Network has "Fixed", in which each of the 16 nodes gets its own channel.

**Advanced features:**  
Rests can be inserted into the sequence by connecting a node output to something other than another node.  
If in doubt, the SS-112 input sinks module by Submarine is a good choice.  

Node inputs accept polyphonic signals for a total of 32 possible trigger sources per node.  

Nodes 1 and 9 can be "bypassed" with their adjacent buttons.  
When in bypass mode, a node will still relay to the node outputs as normal, but will skip outputting to the main cv and gate outputs.

CV Attenuversion scales the voltage range of all channels of the main CV out.   
The input will override the knob and uses the 1v/Octave standard.

Gate length affects all channels of the main Gate out. It does not affect node outputs.  
The knob is on an exponential scale from 0 - 10 seconds. 
Input scales the value on the knob linearly such that 0-10v = 0-100%.  
Polyphonic input is accepted to scale each output channel separately. 
If a poly input with too few channels is used, the remaining channels get a default of 100% of the knob value, in direct contravention of the VCV standard. 