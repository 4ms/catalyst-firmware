
# MacroSeq
### Controls
- Macro Mode
	- **Encoder**: adjusts output signal of scene depending on slider's position in pathway
	- **Encoder + Scene button(s)**: adjust output signal of selected scene(s)
	- **Encoder + Alt**: fine adjustment (same rules as above)
		- Adjustment: approx 1 semitone per step
		- Fine adjustment: approx 4 cents per step
	- **A button + scene button**:
		- *On first scene button press*:
			- if in between two scenes, insert scene in between
			- otherwise replace scene in pathway
		- *On subsequent scene button presses*:
			- insert scene directly after the previously inserted or replaced scene
	- **A button + B button**: Clear all scenes in pathway except for the first and last
	- **B button + Scene button**: Pressing a scene button that is lit up will remove that scene from the pathway.
		- This isn't very intuitive. need to think of a way to make it better.
	- **Alt + play**: begins recording fader and cv inputs
	- **Play**: either stops recording and play or plays / pauses the recorded phrase
	- **B button + play button**: clears the recorded phrase
	- **Alt + B button**: adjust global macro params. 
		- morph step: working
		- random: working but needs some help
		- need: friction, bounce, quantizer
	- **Alt + Bank**: adjust per scene params.
		- Random amount, inc and dec
- Seq Mode:
	- **Scene Button**: Select Seq Channel
    - **B Button**: Selected Seq Length
    - **A Button**: Selected Seq Start Point
    - **A + B Buttons**: Reset Selected Seq Length && Start Point
    - **Bank button + Encoder**: Selected Seq CV type (quan, gate, unquan), clock div, seq direction, random amount, random seed
    - **Alt + B + Encoder** Global seq settings: Bpm, Clock div....
    