# CQUAM-AM-Stereo-Universal-Tuner-for-GNUradio
#############################################################################################
a C++ INSTALLABLE STANDALONE HAS BEEN CREATED AND IS IN BETA TEST, YOU CAN TRY IT OUT AND FEEL
FREE TO SEND YOUR FEEDBACK,MAKE INSTALLATION FOR lINUX, WINDOWS AND MAC ARE AVAILABLE,
NAVIGATE TO FIND THE INSTALLATION INSTRUCTIONS TO BUILD YOUR INSTALL.

THE APPLICATION SHOULKD BE TURNKEY ONCE INSTALLED, NO NEED TO CONFIGURE, IT SHOULD AUTO
DETECT YOUR SDR DONGLE IF ITS SET UP.


###############################################################################################

The aim is to create a pleasurable AM listening experience even on mono transmissions

tunes from 100KHz to 30,000KHz, so will cover the longwave and Shortwave bands as well
Notch filter selection: 5KHz for some shortwave broadcast bands, 9KHz for most other regions, 10KHz for USA, 
the notch filter takes away one of the worst complaints of AM radio is the high pitched squeal

the receiver can get very noisy when not tuned to a station, that is due to the Q chasing the noise floor when it hasnt got a carrier to lock to, the solution is to turn off stereo for now, but will be considering some way to reduce this noise on later revision, its not really a fault, just a wide open CQUAM decoder.

it has variable capture bandwidth slider thats named Audio Bandwidth, the slider numbers represent the frequency response which is half the channel capture bandwidth

The GUI interface will be replaces with a more visually appealing version when the final features are developed, the aim for this version is to provide a user friendly experience of an AM/CQUAM only receiver that covers all bands at or below 30MHz

the aim is to release windows, mac and linux versions and a lineup of standalone receivers and media centres with all the features that are often neglected, there will be no shortcuts by the end, the power of software has made possibilities endless without the hardware bulk of yesterdays implementations.

#######################
THE AIM OF THIS PROJECT
#######################
I want to help make CQUAM mainstream, Manufacturers can learn a lesson on this, Hardware is expensive, even more so in the past, but computer code gets really cheap and easy to implement especially if its mass deployed after development

This AM Stereo project is part of implementing what is often neglected in 99% of receivers with AM reception, and worse, poor circuit design too often exist in the AM section that gives very poor distortion figures if measured, in nearly every AM receiver there is absolutely no noise reduction implemented, instead, perception of noise is reduced by a very cheap and dirty method of squeezing the channel capture width so narrow, you barely get anything more than 2KHz top end frequency response in some receivers, some of the better receivers are mostly at 6KHz top end, less often frequency response up to 8KHz if you are lucky, this is a very good explanation why AM receivers have a well known reputation of very poor audio fidelity. 

the reality, AM radio is very much capable of high fidelity matching the quality of FM if the broadcast is set wide enough for 15KHz top end response, in fact, with CQUAM, you can achieve quite easily a true HiFi frequency response of 20KHz top end, this is because unlike the FM stereo Multiplex Stereo system, CQUAM does not have a 19KHz pilot tone you have to avoid clashing with, so you have to hardwall limit at 15KHz, in fact you can go beyond 20KHz with CQUAM AM Stereo, however, The reality sets in where the limit on AM frequency response in this case would be more to do with emission regulations, and how transmitters and receivers are built accordingly, but definitely not to do with frequency response limits of AM, because there simply isnt any, one good example to look at is how PAL and NTSC television images are tramsmitted: 

The picture is transmitted with Amplitude Modulation (AM), or more accurately Vestigal Sideband Modulation where its similar to Single Side Band, but has a small amount of the opposing sideband, which does three things in order to convey the lower frequencies of the video signal better and simplify filtering in television tuners, at the same time reducing the channel occupation space of the RF signal, SSB and VSB are very much a version of AM, sideband information around the carrier where the frequency of the signal corresponds exactly its position in frequency away from the carrier was employed for a very good reason, because AM is very efficient when it comes to channel occupation to frequency response ratio, now here comes the proof, Video itself contains information that requires a frequency response up to 6 Megahertz for NTSC, and up to 8 Megahertz for PAL, thats Megahertz, now audio only requires up to 20 Kilohertz, thats 0.02 Megahertz, this is where I am going here with this.

Now AM Broadcast is Double Side Band with a Carrier, again, audio information frequency components appear exactly "on frequency" away from the carrier, mirrored on both upper and lower side of the carrier, this means for a frequency response of 20KHz top end, the channel occupation sits at 40KHz where this can only be done in a lab, radio stations are strictly not allowed to transmit this wide, nor there is any receiver made that could receive this wide, reduce the response to 15KHz matching FM standards, you end up with 30KHz frequency response, now this was the bandwidth radio stations were allowed in Australia before the change from 10KHz chanel spacing to 9KHz, back then Australia did not have FM until 1975, and didnt take off until the mid 1980's.

A more realistic frequency response of AM broadcast is 9 to 10 KHz frequency response, maybe 12KHz in some cases particularly during the peak of AM Stereo, this is why I allow the filters open up to 15KHz, so that means 9KHz equates to 18KHz occupation, 10K is 20K occupaton, 12K is 24K occupation. Shortwave and European broadcast is 5K typically, though the occasional Shortwave station has been found to be the full 20K channel occupation.

FM on the other hand when it comes to channel occupation is extremely inefficient in comparison, a typical FM broadcast signal has 150KHz channel occupation, with an added 25KHz on the upper and lower side to serve as a guard band within the allocated channel to occupy, this adds to a total of 200KHz, now that makes the 30KHz of a "quality matched" CQUAM signal look extremely small. Now this inefficiency at VHF frequencies is no big deal at all, FM broadcast is given a much larger slice of spectrum to operate on than the AM broadcast band, The mediumwave or MF AM band was given a slice of 1.179 MHz (522 - 1701 KHz) to give you 131 of 9KHz wide channels, while the VHF FM band was given a 20MHz slice for 88 - 108MHz to give one hundred 200KHz wide channels.

The biggest killer of AM radio happens to be how the channels have no guard bands at all like FM does, instead, the channel spacing is opposite, each channel overlaps each other with their allowed occupation space, this limitation is a result in two things, the limited slice of spectrum allocated, and history. This very closely coupled channel spacing is the cause of the biggest complaint of AM radio that often comes in at night when stations hundreds to thousands of K's away could be received, often you would have a station parked on one or either side on the next channel, 9KHz away, and there is where the most unbearable ear piercing high pitched tone comes screaming in, Bang on 9KHz, the older folk may not hear it, but the youth hears it and can often be driven crazy, this is where this CQUAM AM STEREO UNIVERSAL TUNER fixes this issue, apply Notch filter, high pitch noise GONE!

##################
HISTORY
##################

Another thing to understand is AM was the first method in how audio was transmitted, it began when Reginald Fessenden on Christmas Eve, December 24, 1906 surprised an audience of radio operators in the ships moored or out at sea with something they didnt expect, everyone was familiar with the raspy morse hash coming from their receivers which were sent by spark gap transmitters at the time, Reginald sent a message out for ships crew to listen in for something they have never heard before, he then moved away from the spark gap transmitter and fired up the pure sine wave carrier based AM transmitter and began to broadcast "O Holy Night" on the violin, reading a passage from the Bible (Luke Chapter 2), and playing Handel's "Largo" on an Ediphone phonograph.

This sparked a revolution, though there was no concept of guard bands back then, there was more to discover through mistakes that brought up better ideas, The full AM broadcast band was progressively established through the 1920's, and became established around 1927 to what we know to this day, What was set back then cannot be changed much 

####################
TECHNICAL AGENDA
####################

This project will be tackling every complaint about AM radio, the worst part first is co channel interference, instead of narrowing the capture to filter out co channel interference which in turn throws out a lot of audio information, we instead do it properly: apply very precise notch filtering with cancelling techniques that do not compromise audio fidelity, next is to develop T filtering, high end AM Stereo tuners offered a rare feature where high amplitude pulses received such as static or lightning crashes triggered a blanking circuit, preventing these pulses reaching your speakers or headphones, software implementation in this case will have more sophisticated approaches to tackle this interference, other types of interference such as whats generated from Switch Mode power supplies would be a very difficult one to cancel out without compromising fidelity substantially, this is where the limits are reached, the user will need to rectify this and replace offending power supplies with better quality quiet ones, thankfully these days are over since an eforcement strategy was placed on imports arount year 2015, its now only a matter of time people replace these power supplies along with the associated appliances as usual.


###########################
WHY SOFTWARE DEFINED RADIO?
###########################

Because with software defined radio, you are not stuck with the limits of Analog solutions where once built, it cannot be changed and modified easily, adding features in the desigh of an analog design adds to the expense of the production unit, and the most expensive full featured radio would be packed full of components and still only offer two bandwidth options for example, in comparison, a DSP radio that fits in the palm of your hand can offer all band coverage, AM, FM SSB, and six selectable bandwidths, break it open and you will find hardly anything except about four chips, maybe just a couple of inductors, not much more, and thats all, something made in the pre DSP days such as the late 1990's and earlier would not be offering as many decoding and filtering features, you crack that open to find it packed full of resonator circuits.

To achieve AM stereo, you can definitely use a genuine Motorola chip in your design, as you can still get them, though they are no longer produced, you can only go as far as the analog will take you, if you go instead for DSP solutions offered in Software Defined radio, this is where the future becomes now, offering flexibility for more features through adding lines of code, not bulky hardware.


Have a play and have fun with it, cheers, VK4MTV aka Christopher O'Reilly.


23/03/2026 UPDATE: an issue was found with the pilot tone detecor, Fixed!

03/04/2026 UPDATE: notch selection has beeh fixed, the problem happens to be within GNU radio QT selector blocks, it has a bug that prevents the selection block from being able to talk to the AM Stereo block to change filter settings, since the modules within GNUradio are immutable, this bug cannot be fixed, editing has now turned to working on the exported Python files from now on. 

PROBLEMS STILL EXISTING BEFORE MOVING ON: issues are still existing with the pilot light, more sophisticated code will be added to detect a specific 25Hz pilot tone, it will be fixed on next update. The 25Hz detector was previously inefficient on CPU power, so it was simplified. eventual agenda is to export this radio into embedded systems once refinement and processor efficiency has been achieved at the moment this software is quite heavy on processor

03/04/2026 SECOND UPDATE: Finally, the Pilot light is fixed, a very efficient code typically used in DTMF detection in embedded systems which is now used for the 25Hz detection, its called the "Direct Form II Biquad" the rest of the code has been made more efficient on CPU, there are still further refinements to go to lighten the CPU load and further refinement, this is to do with the notch filter. working on the GUI interface so far has been low priority, it looks odd and only functional for testing and development purposes.

so all is working now, have fun with it, dont use the flowgraph anympre for GNUradio, as the blocks I am trying to use is buggy and doesnt work for notch filter selection, plus its out of date.

04/04/2026 UPDATE: More refinement on the epy_Block_5 AM Stereo core, more processor efficient, however now I have reached the limits of the Python code, will be moving the development to C code next, it all works well now, in this next C code version, it will become a standalone turnkey application with support for all SDR dongles, the aim is to ensure user friendliness where the user only needs to make sure the SDR is plugged in and set up and launch the application, my aim is it will just work, none of that configuration configs that will confuse the average joe. this is where development of an attractive user interface and more noise reduction features will be developed, the Notch filter already is the highlight feature of this radio as it stands, a feature that is very rare in AM radios.
