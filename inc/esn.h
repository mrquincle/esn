/*
 * esn.h
 *
 *  Created on: Jun 29, 2009
 *      Author: ted
 */

#ifndef ESN_H_
#define ESN_H_

#include "eigenvalues/nsevd.h"
#include <network.h>

typedef float WEIGHT_TYPE;

//typedef double WEIGHT_TYPE;

enum ESNParameter
{
	INPUT_SIZE,
	OUTPUT_SIZE,
	RESERVOIR_SIZE,
	CONNECTIVITY,
	INPUT_CONNECTIVITY,
	OUTPUT_CONNECTIVITY,
	SPECTRAL_RADIUS,
	INPUT_SCALE,
	OUTPUT_SCALE,
	INPUT_SHIFT,
	OUTPUT_SHIFT,
	RESERVOIR_ACTIVATION,
	OUTPUT_ACTIVATION
};

enum ActivationFunction
{
	IDENTITY_ACTIVATION,
	TANH_ACTIVATION,
	LOGISTIC_ACTIVATION,
	HEAVISIDE_ACTIVATION
};

enum SimulationType
{
	OFFLINE_SEPARATE_INPUT,
	OFFLINE_SIMULTANEOUS_INPUT,
	ONLINE,
	TEACHER_FORCING,
	TEACHER_TESTING,
	PREDICTION // temporary, will be removed!
};

/**
 * The struct "Trial" contains all state information of the ESN for a given all_trials.
 * From the same time series it is namely possible to create several trials. And
 * learn the ESN over them all.
 *
 * @field neuronVal			The states of all neurons
 * @field inputVal			An array of input values
 * @field stateSize
 * @field sampleSize
 * @field classId
 * @field inputSize			The number of input neurons
 */
struct Trial
{
	// Values of all the neurons in the reservoir over time
	// Double array: t * reservoirSize + n (bundled per time step)
	WEIGHT_TYPE *neuronVal;

	// The inputs to the reservoir
	WEIGHT_TYPE const *inputVal;

	// The number of neurons in the reservoir
	int stateSize;

	// The number of samples over time
	int sampleSize;

	// The class id in case the ESN used as a classification task
	int classId;

	// Array to store output values OR teacher values
	WEIGHT_TYPE *outputVal;

	// Number of teacher values before ESN needs to predict on itself in test case
	int teacherTestSize;

	// The dimensionality of the input, the number of input neurons
	int inputSize;

	// Debug values, might be all kind of stuff...
	WEIGHT_TYPE *debug;

	// Destructor removes state array
	~Trial() {
		if (neuronVal != NULL) delete [] neuronVal;
	}
};

/**
 * An echo state reservoir.
 */
class ESN
{
public:

	//! Create default reservoir 2->10->2 with connectivity 0.8
	ESN();

	//! Create flexible reservoir
	ESN(int inputSize, int outputSize, int networkSize, float connectivity);

	//! Run the reservoir with the given parameters
	void Run(Trial *trial, SimulationType simType);

	//! First initialise the reservoir
	void init();

	//! Print all the relevant parameters to stdout
	void printStats();

	//! Store the ESN to a file
	void saveESN(std::string filename);

	//! Load the ESN from a file
	void loadESN(std::string filename);

	//! Destruct ESN
	virtual ~ESN();

	//! Set all parameters from size to ...
	void setParameter(ESNParameter param, void *value);

	void setReservoirActivation(ActivationFunction reservoirActivation);

	void setOutputActivation(ActivationFunction outputActivation);

	WEIGHT_TYPE getDecayRate() const
	{
		return d_decayRate;
	}

	void setDecayRate(WEIGHT_TYPE decayRate)
	{
		this->d_decayRate = decayRate;
	}

	WEIGHT_TYPE getTimeConstant() const
	{
		return d_timeConstant;
	}

	void setTimeConstant(WEIGHT_TYPE timeConstant)
	{
		this->d_timeConstant = timeConstant;
	}

	inline int getInputSize() const
	{
		return d_inputSize;
	}

	inline void setInputSize(int d_inputSize)
	{
		this->d_inputSize = d_inputSize;
	}

	inline int getOutputSize() const
	{
		return d_outputSize;
	}

	inline void setOutputSize(int d_outputSize)
	{
		this->d_outputSize = d_outputSize;
	}

	inline int getReservoirSize() const
	{
		return d_reservoirSize;
	}

	inline void setReservoirSize(int d_reservoirSize)
	{
		this->d_reservoirSize = d_reservoirSize;
	}

	inline ActivationFunction getReservoirActivation() const
	{
		return d_reservoirActivation;
	}


	inline  WEIGHT_TYPE getConnectivity() const
	{
		return d_connectivity;
	}

	inline void setConnectivity(WEIGHT_TYPE d_connectivity)
	{
		this->d_connectivity = d_connectivity;
	}

	inline WEIGHT_TYPE getInConnectivity() const
	{
		return d_inConnectivity;
	}

	inline void setInConnectivity(WEIGHT_TYPE d_inConnectivity)
	{
		this->d_inConnectivity = d_inConnectivity;
	}

	inline WEIGHT_TYPE getFbConnectivity() const
	{
		return d_fbConnectivity;
	}

	inline void setFbConnectivity(WEIGHT_TYPE d_fbConnectivity)
	{
		this->d_fbConnectivity = d_fbConnectivity;
	}

	inline WEIGHT_TYPE getSpectralRadius() const
	{
		return d_spectralRadius;
	}

	inline void setSpectralRadius(WEIGHT_TYPE d_spectralRadius)
	{
		this->d_spectralRadius = d_spectralRadius;
	}

	inline WEIGHT_TYPE getInputScale() const
	{
		return d_inputScale;
	}

	inline void setInputScale(WEIGHT_TYPE d_inputScale)
	{
		this->d_inputScale = d_inputScale;
	}

	inline WEIGHT_TYPE getFeedbackScale() const
	{
		return d_feedbackScale;
	}

	inline void setFeedbackScale(WEIGHT_TYPE d_feedbackScale)
	{
		this->d_feedbackScale = d_feedbackScale;
	}

	inline WEIGHT_TYPE getInputShift() const
	{
		return d_inputShift;
	}

	inline void setInputShift(WEIGHT_TYPE d_inputShift)
	{
		this->d_inputShift = d_inputShift;
	}

	inline WEIGHT_TYPE getFeedbackShift() const
	{
		return d_feedbackShift;
	}

	inline void setFeedbackShift(WEIGHT_TYPE d_feedbackShift)
	{
		this->d_feedbackShift = d_feedbackShift;
	}

	inline WEIGHT_TYPE *getInputWeights() const
	{
		return d_inputWeights;
	}

	inline  WEIGHT_TYPE *getOutputWeights() const
	{
		return d_outputWeights;
	}

	void setOutputWeights( WEIGHT_TYPE *weights, int len);

	inline WEIGHT_TYPE *getFeedbackWeights() const
	{
		return d_feedbackWeights;
	}

	inline WEIGHT_TYPE *getReservoirWeights() const
	{
		return d_reservoirWeights;
	}

protected:
	WEIGHT_TYPE (ESN::* resActFunc)(WEIGHT_TYPE value);
	WEIGHT_TYPE (ESN::* outActFunc)(WEIGHT_TYPE value);
	WEIGHT_TYPE (ESN::* outInvActFunc)(WEIGHT_TYPE value);

	WEIGHT_TYPE act_heaviside(WEIGHT_TYPE value);
	WEIGHT_TYPE act_logistic(WEIGHT_TYPE value);
	WEIGHT_TYPE act_invlogistic(WEIGHT_TYPE value);
	WEIGHT_TYPE act_tanh(WEIGHT_TYPE value);
	WEIGHT_TYPE act_invtanh(WEIGHT_TYPE value);

	inline WEIGHT_TYPE act_identity(WEIGHT_TYPE value) { return value; 	}

	inline WEIGHT_TYPE act_invidentity(WEIGHT_TYPE value) { return value; }

	void generateReservoirConnections();
private:
	int d_inputSize;
	int d_outputSize;
	int d_reservoirSize;
	ActivationFunction d_reservoirActivation, d_outputActivation;

	WEIGHT_TYPE d_connectivity;
	WEIGHT_TYPE d_inConnectivity;
	WEIGHT_TYPE d_fbConnectivity;

	WEIGHT_TYPE d_spectralRadius;

	WEIGHT_TYPE d_inputScale;
	WEIGHT_TYPE d_feedbackScale;

	WEIGHT_TYPE d_inputShift;
	WEIGHT_TYPE d_feedbackShift;

	WEIGHT_TYPE *d_inputWeights;
	WEIGHT_TYPE *d_outputWeights;
	WEIGHT_TYPE *d_feedbackWeights;

	//! Incoming weights retrieved as d_reservoirWeights[this*d_reservoirSize + other]
	//! So, incoming weights are stored row-wise, outgoing weights are stored column-wise
	WEIGHT_TYPE *d_reservoirWeights;

	WEIGHT_TYPE *d_output;

	// A threshold per neuron, needed for heaviside activation function
	WEIGHT_TYPE *d_thresholds;

	WEIGHT_TYPE d_timeConstant;
	WEIGHT_TYPE d_decayRate;

	WEIGHT_TYPE d_excitatory;

	aNetwork::Network reservoir;

	void destroy();
	void scaleAndShift(WEIGHT_TYPE * weights, int weightSize, WEIGHT_TYPE scale, WEIGHT_TYPE shift);
	void generateConnections(WEIGHT_TYPE connectivity, int weightSize, WEIGHT_TYPE * weights, WEIGHT_TYPE min = -1, WEIGHT_TYPE max = +1);
//	bool spectralRadius(WEIGHT_TYPE* reservoirWeights, int reservoirSize, WEIGHT_TYPE* spectralRadius);
	void uniform(WEIGHT_TYPE *value, float min=-1, float max=1);
	void saveWeights(std::ofstream *outputFile, int matrixSize, WEIGHT_TYPE *matrix);
	void loadWeights(std::ifstream *inputFile, int matrixSize, WEIGHT_TYPE *matrix);
};

#endif /* ESN_H_ */
