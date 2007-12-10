/*
 *  SonarController.cpp
 *  sonarController
 *
 *  Created by Leo Singer on 11/25/07.
 *  Copyright 2007 Robotics@Maryland. All rights reserved.
 *
 */


#include "SonarController.h"


#include "TDOA.h"


#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


namespace ram {
namespace sonar {

		
void init(int ns)
{
	nchannels = ns;
	samprate = SAMPRATE;
	targetfreq = 30000;
	nearestperiod = round(samprate / targetfreq);
	numPeriods = 6;
	windowlength = nearestperiod * numPeriods;
	threshold = ((1 << (BITS_ADCCOEFF + 8 * sizeof(adcdata_t) - 2)) 
				 * windowlength) >> 5;
	
	/**	Minimum number of samples to wait before listening for the next set of
	 *	pings.
	 */
	
	minSamplesBetweenPings = 
		(NOMINAL_PING_DELAY - ((float) MAXIMUM_SPEED * NOMINAL_PING_DELAY 
		+ 3 * MAX_SENSOR_SEPARATION)/ SPEED_OF_SOUND) * samprate;
	
	/** Maximum number of samples to listen past minSamplesBetweenPings before 
	 *	going back to sleep
	 */
	
	maxSamplesToWaitForFirstPing = 
		((float) 2 * MAXIMUM_SPEED * NOMINAL_PING_DELAY 
		 + 2 * MAX_SENSOR_SEPARATION) / SPEED_OF_SOUND * samprate;
	
	/** Maximum number of samples past the rising edge of the last receipt of 
	 *	the current ping to wait for the next rising edge on the next hydrophone
	 */
	
	maxSamplesTDOA = 1.25 * MAX_SENSOR_SEPARATION / SPEED_OF_SOUND * samprate;
	
	/** Set up sampled cosine and sampled sine for sliding DFT */
	
	setupCoefficients();
	
	/** Set up buffers */
	
	setupWindow();
	
	sampleIndex = indexOfLastRisingEdge = indexOfLastWake = 0;
	sonarchannelstate = new sonarchannelstate_t[nchannels];
	
	
	sleepingChannelCount = ns;
	listeningChannelCount = captureChannelCount = 0;
	for (int i = 0 ; i < nchannels ; i ++)
		sonarchannelstate[i] = SONAR_CHANNEL_SLEEPING;
	sonarstate = SONAR_SLEEPING;
}


void destroy()
{
	delete [] coefreal;
	delete [] coefimag;
	delete [] sumreal;
	delete [] sumimag;
	delete [] mag;
	delete [] sample;
	for (int i = 0 ; i < nchannels ; i ++)
	{
		delete [] windowreal[i];
		delete [] windowimag[i];
	}
	delete [] windowreal;
	delete [] windowimag;
	delete [] sonarchannelstate;
	delete [] currentChunks;
	for (int i = 0 ; i < oldChunks.size() ; i ++)
		delete oldChunks[i];
}


void receiveSample(adcdata_t *newdata)
{
	sampleIndex ++;
	if (getState() == SONAR_SLEEPING)
		if (sleepTimeIsUp())
			wake();
	
	if (getState() == SONAR_LISTENING)
	{
		memcpy(sample, newdata, sizeof(*sample) * nchannels);
		updateSlidingDFT();
		for (int channel = 0 ; channel < nchannels ; channel ++)
		{
			switch (getChannelState(channel))
			{
				case SONAR_CHANNEL_LISTENING:
					if (exceedsThreshold(channel))
					{
						startCapture(channel);
						captureSample(channel);
					}
					break;
				case SONAR_CHANNEL_CAPTURING:
					if (exceedsThreshold(channel))
						captureSample(channel);
					else
						stopCapture(channel);
					break;
			}
		}
		if (listeningChannelCount + captureChannelCount == 0 || 
			(captureChannelCount == 0 && listenTimeIsUp()))
			sleep();
	}
}


adcmath_t getMag(int channel)
{
	return mag[channel];
}


sonarstate_t getState()
{
	return sonarstate;
}


sonarchannelstate_t getChannelState(int channel)
{
	return sonarchannelstate[channel];
}


void go()
{
	wake();
}


namespace /* internal */ {

	
void setupCoefficients()
{
	coefreal = new adcmath_t[nearestperiod];
	coefimag = new adcmath_t[nearestperiod];
	int mag = 1 << (BITS_ADCCOEFF - 1);
	for (int i = 0 ; i < nearestperiod ; i++)
	{
		coefreal[i] = cosf(- 2 * M_PI * i / nearestperiod) * mag;
		coefimag[i] = sinf(- 2 * M_PI * i / nearestperiod) * mag;
	}
}


void setupWindow() {
	windowreal = new adcmath_t*[nchannels];
	windowimag = new adcmath_t*[nchannels];
	sumreal = new adcmath_t[nchannels];
	sumimag = new adcmath_t[nchannels];
	mag = new adcmath_t[nchannels];
	sample = new adcdata_t[nchannels];
	for (int i = 0 ; i < nchannels ; i ++)
	{
		windowreal[i] = new adcmath_t[windowlength];
		windowimag[i] = new adcmath_t[windowlength];
	}
	currentChunks = new SonarChunk*[nchannels];
	purge();
}


void purge()
{
	memset(sumreal, 0, sizeof(*sumreal) * nchannels);
	memset(sumimag, 0, sizeof(*sumimag) * nchannels);
	memset(mag, 0, sizeof(*mag) * nchannels);
	memset(sample, 0, sizeof(*sample) * nchannels);
	for (int i = 0 ; i < nchannels ; i ++)
	{
		memset(windowreal[i], 0, sizeof(**windowreal) * windowlength);
		memset(windowimag[i], 0, sizeof(**windowimag) * windowlength);
	}
	curidx = 0;
}


/** Update the one selected Fourier amplitude using a sliding DFT
 */
void updateSlidingDFT()
{
	/*	coefidx is an index into the circular buffers coefreal (sampled cosine)
	 *	and coefimag (sampled sine) that corresponds to curidx.  We apply the 
	 *	modulus operation because the coefficient buffers may be the same size
	 *	as or smaller than the window buffers, provided that the window length
	 *	is an integer multiple of the coefficient buffer length.
	 */
	
	int coefidx = curidx % nearestperiod;
	
	for (int channel = 0 ; channel < nchannels ; channel ++)
	{
		
		/*	For each sample we receive, we only need to compute one new term in
		 *	the DFT sum.  These two lines together compute 
		 *
		 *		f(N-1) x cos(2 pi k (N - 1) / N) 
		 *
		 *	and
		 *
		 *		f(N-1) x sin(2 pi k (N - 1) / N)
		 *
		 *	which are, respectively, the real and imaginary parts of 
		 *	
		 *		f(N-1) x exp(2 pi i k (N - 1) / N)
		 *	
		 *	which is simply the last term of the sum for F(k).
		 */
		
		windowreal[channel][curidx] = coefreal[coefidx] * sample[channel];
		windowimag[channel][curidx] = coefimag[coefidx] * sample[channel];
		
		/*	The next two lines update the real and imaginary part of the complex
		 *	output amplitude.
		 *	
		 *	We add window____[channel][curidx] to sum____[channel] because all 
		 *	of the previous N-1 terms of F(k), numbered 0,1,...,N-3,N-2, were 
		 *	calculated and summed during previous iterations of this function - 
		 *	hence the name "sliding DFT".
		 */
		
		sumreal[channel] += windowreal[channel][curidx];
		sumimag[channel] += windowimag[channel][curidx];
		
		/*	We compute the L1 norm (|a|+|b|) instead of the L2 norm 
		 *	sqrt(a^2+b^2) in order to aovid integer overflow.  Since we are only
		 *	using the magnitude for thresholding, this is an acceptable 
		 *	approximation.
		 */
		
		mag[channel] = abs(sumreal[channel]) + abs(sumimag[channel]);
	}
	
	/*	curidx represents the index into the circular buffers 
	 *	windowreal[channel] and windowimag[channel] at which the just-received
	 *	sample will be added to the DFT sum.
	 */
	
	++curidx;
	if (curidx == windowlength)
		curidx = 0;
	
	for (int channel = 0 ; channel < nchannels ; channel ++)
	{
		/*	We subtract window____[channel][firstidx] because both windowreal[i]
		 *	and windowimag[i] are circular buffers.  On the next call of this 
		 *	function, window____[channel][firstidx] will be overwritten with 
		 *	computations from the next sample.
		 */
		
		sumreal[channel] -= windowreal[channel][curidx];
		sumimag[channel] -= windowimag[channel][curidx];
	}
}


bool sleepTimeIsUp()
{
	assert(getState() == SONAR_SLEEPING);
	return (sampleIndex - indexOfLastWake) > minSamplesBetweenPings;
}


bool listenTimeIsUp()
{
	assert(getState() == SONAR_LISTENING);
	if (listeningChannelCount == nchannels 
		&& (sampleIndex - indexOfLastWake) > maxSamplesToWaitForFirstPing)
		return true;
	else if (listeningChannelCount < nchannels && (sampleIndex - indexOfLastRisingEdge) > maxSamplesTDOA)
		return true;
	else
		return false;
}


void wake()
{
	assert(getState() == SONAR_SLEEPING);
	assert(printf("Wake up\n") || true);
	for (int channel = 0 ; channel < nchannels ; channel ++)
		wakeChannel(channel);
	indexOfLastWake = sampleIndex;
	sonarstate = SONAR_LISTENING;
}


void sleep()
{
	assert(getState() == SONAR_LISTENING);
	assert(printf("Sleep\n") || true);
	for (int channel = 0 ; channel < nchannels ; channel ++)
		stopCapture(channel);
	sonarstate = SONAR_SLEEPING;
}


bool exceedsThreshold(int channel)
{
	return mag[channel] > threshold;
}


void wakeChannel(int channel)
{
	assert(getChannelState(channel) == SONAR_CHANNEL_SLEEPING);
	assert(printf("Waking channel %d\n", channel) || true);
	sonarchannelstate[channel] = SONAR_CHANNEL_LISTENING;
	sleepingChannelCount --;
	listeningChannelCount ++;
}


void sleepChannel(int channel)
{
	stopCapture(channel);
}


void startCapture(int channel)
{
	assert(getChannelState(channel) == SONAR_CHANNEL_LISTENING);
	assert(printf("Starting capture on channel %d at sample %d\n", 
				  channel, sampleIndex) || true);
	currentChunks[channel] = new SonarChunk(sampleIndex);
	sonarchannelstate[channel] = SONAR_CHANNEL_CAPTURING;
	listeningChannelCount --;
	captureChannelCount ++;
	indexOfLastRisingEdge = sampleIndex;
}


void stopCapture(int channel)
{
	switch (getChannelState(channel))
	{
		case SONAR_CHANNEL_CAPTURING:
			assert(printf("Sleeping channel %d with %d samples captured\n", 
						  channel, currentChunks[channel]->size()) || true);
			currentChunks[channel]->setFourierComponents(sumreal[channel], sumimag[channel]);
			oldChunks.push_back(currentChunks[channel]);
			sonarchannelstate[channel] = SONAR_CHANNEL_SLEEPING;
			captureChannelCount --;
			sleepingChannelCount ++;
			break;
			
		case SONAR_CHANNEL_LISTENING:
			assert(printf("Sleeping channel %d with no samples captured\n", 
						  channel) || true);
			sonarchannelstate[channel] = SONAR_CHANNEL_SLEEPING;
			listeningChannelCount --;
			sleepingChannelCount ++;
			break;
	}
}


void captureSample(int channel)
{
	assert(getChannelState(channel) == SONAR_CHANNEL_CAPTURING);
	if (!currentChunks[channel]->append(sample[channel]))
	{
		assert(printf("Channel %d full\n", channel) || true);
		stopCapture(channel);
	}
}


void analyzeChunks()
{
	for (int i = 0 ; i < oldChunks.size() - 1 ; i ++)
	{
		for (int j = i + 1 ; j < oldChunks.size() ; j ++)
		{
			adcsampleindex_t tdoac = tdoa_xcorr(*oldChunks[i] , *oldChunks[j]);
			printf("TDOA between chunks %d and %d of %d samples\n", i, j, tdoac);
		}
	}
}


} // namespace /* internal */


} // namespace sonar
} // namespace ram
