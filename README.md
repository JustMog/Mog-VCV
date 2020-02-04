Mog-VCV
===========================
Mog's modules for VCV Rack

**Network:**
---------------------------
![Network](/doc/network.png)

Network is a "node-based" polyphonic sequencer consisting of 16 identical nodes.

When a node receives a trigger at either of its two inputs, it cycles between relaying that trigger first to the main gate output and then to each of the node's four outputs in turn.  
When a node relays a trigger to the main gate out, the value of the node's knob is sent to the main CV out.  
Node outputs that are disconnected are skipped.

You can generate a repeating sequence by connecting nodes together.

![Demo](/doc/network_demo.gif)

**Polyphony:**  
Network features up to 16 polyphony channels and three different polyphony modes.  
"Rotate" and "Reset" modes function identically to the modes of the same name found on the VCV Core modules.  
Since Network deals in continuous voltages rather than discrete midi notes, the "Reuse" mode of those modules does not apply.  
Instead, Network has "Fixed", in which each of the 16 nodes gets its own channel.

**Advanced features:**  
Nodes 1 and 9 can be "bypassed" with the buttons next to them.  
When in bypass mode, a node will still relay to the node outputs as normal, but will skip outputting its own value to the main cv output and relaying to the main gate output.

Though Network will work just fine when sent triggers, it actually takes and relays any input signal, triggering when the input goes from <=0 to >0.  
When relaying input signals, the maximum value of the two inputs is relayed.

Finding creative uses for this is left up to the reader.
