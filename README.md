# CQUAM-AM-Stereo-Universal-Tuner-for-GNUradio
#############################################################################################

DONT USE THE GNURADIO FLOWGRAPH ANYMORE, IT IS OUT OF DATE!!! RUN THE STANDALONE PY SCRIPT
USE THE AMRADIO.PY INSTEAD, AND DOWNLOAD THE LATEST EPYBLOCK_5 AND RUN IT IN THE SAME FOLDER
ITS THE LATEST MOST REFINED VERSION.

###############################################################################################

The aim is to create a pleasurable AM listening experience even on mono transmissions, its close to polished in performance but needs work on the notch filter selection, the GUI interface is basic, and has only RTL SDR support
tunes from 100KHz to 30,000KHz, so will cover the longwave and Shortwave bands as well
Notch filter selection: 5KHz for some shortwave broadcast bands, 9KHz for most other regions, 10KHz for USA, not responding yet, needs some work, the workaround is to fill in the notch settings in the default field of the selector interface then relaunch the application, the Notch filter is extremely effective, takes away one of the worst complaints of AM radio is the high pitched squeal
the receiver can get very noisy when not tuned to a station, that is due to the Q chasing the noise floor when it hasnt got a carrier to lock to, the solution is to turn off stereo for now, but will be considering some way to reduce this noise on later revision, its not really a fault, just a wide open CQUAM decoder.
variable capture bandwidth slider thats named Audio Bandwidth, the slider numbers represent the frequency response which is half the channel capture bandwidth
The GUI interface will be replaces with a more visually appealing version when the final features are decided, the aim for this version is to provide a user friendly experience of an AM/CQUAM only receiver that covers all bands
beta testing needs to be done to broaden support for other SDR's, right now it is built to run on the cheap RTL SDR, if you have the V4, it should be able to run out of the box the moment you load it onto GNUradio
the aim on the final is to release a standalone SDR based receiver
feel free to develop your version from this receiver, I want to help make CQUAM mainstream, you can use a genuine Motorola chip in your design, but SDR is far more maleable, offering flexibility for more features, such as Stereo sound stage stabilisation, which is considered later, for now this radio has been developed to stay within the constraints of a Rasberry Pi5, the Notch filtering feature has increased processor demand, a Pi5 will need to be running at 2.4GHz to run this radio smoothly.
Have a play and have fun with it, cheers, VK4MTV aka Christopher O'Reilly.


23/03/2026 UPDATE: an issue was found with the pilot tone detecor, Fixed!

03/04/2026 UPDATE: notch selection has beeh fixed, the problem happens to be within GNU radio QT selector blocks, it has a bug that prevents the selection block from being able to talk to the AM Stereo block to change filter settings, since the modules within GNUradio are immutable, this bug cannot be fixed, editing has now turned to working on the exported Python files from now on. 

PROBLEMS STILL EXISTING BEFORE MOVING ON: issues are still existing with the pilot light, more sophisticated code will be added to detect a specific 25Hz pilot tone, it will be fixed on next update. The 25Hz detector was previously inefficient on CPU power, so it was simplified. eventual agenda is to export this radio into embedded systems once refinement and processor efficiency has been achieved at the moment this software is quite heavy on processor

03/04/2026 SECOND UPDATE: Finally, the Pilot light is fixed, a very efficient code typically used in DTMF detection in embedded systems which is now used for the 25Hz detection, its called the "Direct Form II Biquad" the rest of the code has been made more efficient on CPU, there are still further refinements to go to lighten the CPU load and further refinement, this is to do with the notch filter. working on the GUI interface so far has been low priority, it looks odd and only functional for testing and development purposes.

so all is working now, have fun with it, dont use the flowgraph anympre for GNUradio, as the blocks I am trying to use is buggy and doesnt work for notch filter selection, plus its out of date.
