Mog-VCV
===========================
Mog's modules for VCV Rack

**Network:**
---------------------------
![Network](https://github.com/JustMog/Mog-VCV-Docs/blob/master/doc/network.png)

Network is a "node-based" polyphonic sequencer consisting of 16 identical nodes.  

When a node first receives a gate at either of its two inputs, it sends the node's value (set by the knob) to the main cv out, and relays the gate to the main gate out.  
Subsequent gates are relayed to each of the nodes connected outputs in turn, then the cycle repeats.


You can generate a repeating sequence by connecting nodes together.

![Demo](https://github.com/JustMog/Mog-VCV-Docs/blob/master/doc/network_demo.gif)

**Polyphony:**  

Network features up to 16 polyphony channels and three different polyphony modes.  
"Rotate" and "Reset" modes function identically to the modes of the same name found on the VCV Core modules.  
Since Network deals in continuous voltages rather than discrete midi notes, the "Reuse" mode of those modules does not apply.  
Instead, Network has "Fixed", in which each of the 16 nodes gets its own channel.

**Advanced features:**  

Rests can be inserted into the sequence by connecting a node output to something other than another node.
If in doubt, the SS-112 input sinks module by Submarine is a good choice.

Node inputs accept polyphonic signals for a total of 32 possible gate sources per node.

Nodes 1 and 9 can be "bypassed" with their adjacent buttons.
When in bypass mode, a node will still relay to the node outputs as normal, but will skip outputting to the main cv and gate outputs.

CV Attenuversion scales the voltage range of all channels of the main CV out.
The input will override the knob and uses the 1v/Octave standard.

**Nexus:**
---------------------------
![Nexus](https://github.com/JustMog/Mog-VCV-Docs/blob/master/doc/nexus.png)

Nexus is a combination clock divider & sequential switch designed for use with network, but it may be useful elsewhere too.

Nexus is made up of 6 identical "stages".  
Each stage sends the first N gates it receives to its first output and all subsequent gates to its second output. (N is adjustable using that stage's knob).  

![Demo](https://github.com/JustMog/Mog-VCV-Docs/blob/master/doc/nexus_demo.gif)

**Advanced features:**  

The second output of each stage is "normalled" to the input of the next, except the final stage, which is normalled to reset.

Inputs and outputs are polyphonic.



