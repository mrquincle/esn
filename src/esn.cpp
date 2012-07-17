/*
 * ESN.cpp
 *
 * This class implements an Echo State reservoir
 * Parameters are based on works of Jaeger, Holzmann and Morse
 *
 *  Created on: Jun 30, 2009
 *      Author: ted
 */

#include "esn.h"
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <assert.h>

using namespace std;

// Full input connectivity (not targetted to inhibitory/excitatory neurons)
#define DEFAULT_INPUT_CONN		1

// Non-default follows Jaeger with specific distribution + scale for inputs
#define DEFAULT_INPUT_SCALE		0

// Have leftover (1) or not (0)
// without leftover it does not work...
#define DEFAULT_LEFTOVER		1

// Reservoir type can be a random (1) or inhibitory/excitatory balanced network (0)
#define RESERVOIR_TYPE			1

// HEAVISIDE_ACTIVATION or TANH_ACTIVATION

// The heaviside activation actually only makes sense with refractory dynamics
// or the maximum spiking rate is only related to the timestep size
//#define ACTIVATION_TYPE			HEAVISIDE_ACTIVATION
#define ACTIVATION_TYPE			TANH_ACTIVATION

#define THRESHOLD_VALUE			0.0
//#define THRESHOLD_VALUE			0.4

//#define ADD_NOISE

#if DEFAULT_INPUT_CONN == 0
#undef DEFAULT_INPUT_CONN
#endif

#if DEFAULT_INPUT_SCALE == 0
#undef DEFAULT_INPUT_SCALE
#endif

#if DEFAULT_LEFTOVER == 0
#undef DEFAULT_LEFTOVER
#endif


ESN::ESN()
{
	ESN(2,2,10,0.8);
}

/**
 * The ESN receives the dimensions of the reservoir as input and the connectivity
 * degree.
 * @param inputSize			number of input neurons
 * @param outputSize		number of output neurons
 * @param reservoirSize		reservoir size (without input and output neurons)
 * @param connectivity		the degree of connectedness (1.0 is fully connected)
 */
ESN::ESN(int inputSize, int outputSize, int reservoirSize,
		float connectivity):
		d_inputSize(inputSize),
		d_outputSize(outputSize),
		d_reservoirSize(reservoirSize),
		d_reservoirActivation(ACTIVATION_TYPE),
		d_outputActivation(IDENTITY_ACTIVATION),
		d_connectivity(connectivity),
		d_inConnectivity(1.0), // 0.1
		d_fbConnectivity(0),
		d_spectralRadius(0.8), // 0.79 is best general value by Venayagamoorthy and Shishir
		d_inputScale(1),
		d_feedbackScale(1),
		d_inputShift(0),
		d_feedbackShift(0),
		d_timeConstant(1),
		d_decayRate(1), // timeConstant = 1, decayRate = 1, means no leftover...
		d_excitatory(0.7)
{
	d_inputWeights 		= NULL;
	d_outputWeights		= NULL;
	d_feedbackWeights 	= NULL;
	d_reservoirWeights 	= NULL;
	d_output			= NULL;

	setReservoirActivation(d_reservoirActivation);
	setOutputActivation(d_outputActivation);

	// Reservoir size of 1, then init() hangs
	assert(reservoirSize > 1);
}

void ESN::setParameter(ESNParameter param, void * value)
{
	switch(param)
	{
	case INPUT_SIZE:
		d_inputSize 		= *reinterpret_cast<int*>(value);
		break;
	case OUTPUT_SIZE:
		d_outputSize 		= *reinterpret_cast<int*>(value);
		break;
	case RESERVOIR_SIZE:
		d_reservoirSize 	= *reinterpret_cast<int*>(value);
		break;
	case CONNECTIVITY:
		d_connectivity 		= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case INPUT_CONNECTIVITY:
		d_inConnectivity 	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case OUTPUT_CONNECTIVITY:
		d_fbConnectivity 	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case SPECTRAL_RADIUS:
		d_spectralRadius 	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case INPUT_SCALE:
		d_inputScale 		= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case OUTPUT_SCALE:
		d_feedbackScale 	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case INPUT_SHIFT:
		d_inputShift 		= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case OUTPUT_SHIFT:
		d_feedbackShift		= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case OUTPUT_ACTIVATION:
		d_outputActivation 	= *reinterpret_cast<ActivationFunction*>(value);
		break;
	case RESERVOIR_ACTIVATION:
		d_reservoirActivation = *reinterpret_cast<ActivationFunction*>(value);
		break;
	default:
		cout << "Unknown Parameter" << endl;
	}
}
// Network Initialization //


void ESN::generateConnections(WEIGHT_TYPE connectivity, int weightSize, WEIGHT_TYPE * weights, WEIGHT_TYPE min, WEIGHT_TYPE max)
{
	// Generate  Weights between [-1,1]

	// TODO: There is an overhead on positions and used connections
	// can be reduced by creating an array with only used connections
	// and use a pair to associate neurons and weights
	// the connectivity is then for every neuron the same
	// but what's the extra cost of using a pair

	srand(time(NULL));

	// Holzmann and Jaeger use -0.5 on the connectionSize
	int connectionSize = WEIGHT_TYPE(weightSize) * connectivity;

	for (int x = 0; x < connectionSize; ++x)
	{
		int conn = x;
		if(connectivity != 1.0)
		{
			conn = rand() % weightSize;
			// Maybe exclude options from randomness,
			// can take for ever to find an empty spot
			if(weights[conn] != WEIGHT_TYPE(0))
			{
				--x;
				continue;
			}
		}
		uniform(weights+conn, min, max);
	}

}

void ESN::scaleAndShift(WEIGHT_TYPE * weights, int weightSize, WEIGHT_TYPE scale, WEIGHT_TYPE shift)
{
	for (int x = 0; x < weightSize; ++x)
	{
		weights[x] *= scale;
		weights[x] += shift;
	}
}

/* Generates the input, feedback and reservoir weights
 * Set the desired parameters before initializing
 * The convention for the weight array's are:
 * A block with all the input neurons to one reservoir neurons
 * from [0,inputsize] = all connections from the input to one reservoir neuron
 * For the feedback is [0, outputsize] = all the connections from the output to one reservoir neuron
 * For the output weights first [0,reservoirsize] all the connection from the reservoir to 1st output neuron
 * from [reservoirsize, reservoirsize+inputsize] all the connection from the input neurons to the 1st output neuron
 *  Optionally an bias for the reservoir can be used, for a constant input to reservoir neurons (D. Verstraeten, K. Braeckman)
 */
void ESN::init()
{
	destroy();

	// Generate Input Weights between [-1,1]
	int connectionSize = d_inputSize*d_reservoirSize;
	d_inputWeights = new WEIGHT_TYPE[connectionSize];
	for (int x = 0; x < connectionSize; ++x)
		d_inputWeights[x] = WEIGHT_TYPE(0);

#ifdef DEFAULT_INPUT_CONN
	generateConnections(d_inConnectivity, connectionSize, d_inputWeights);
#else
	cout << "Adjusted input connections" << endl;
	int nof_excited_neurons = d_excitatory * connectionSize;
	// generate input connections for excitatory neurons
//	generateConnections(d_inConnectivity, nof_excited_neurons, d_inputWeights, 0.9, 1);
	// generate input connections for inhibitory neurons
//	generateConnections(d_inConnectivity, connectionSize - nof_excited_neurons, d_inputWeights + nof_excited_neurons, 0.45, 0.5);

	// assume a fixed bias of 1.0, then use param values E = 1, and I = 0.8 from Fig.3, Van Vreeswijk
	for (int x = 0; x < nof_excited_neurons; ++x) {
		d_inputWeights[x] = 1.0;
	}
	for (int x = nof_excited_neurons; x < connectionSize; ++x) {
		d_inputWeights[x] = 0.8;
	}

#endif


#ifdef DEFAULT_INPUT_SCALE
	if(d_inputScale != 1 || d_inputShift != 0 )
		scaleAndShift(d_inputWeights, connectionSize, d_inputScale, d_inputShift);
#else
//	The input connections were randomly chosen to be 0, 0.14,−0.14 with probabilities 0.5, 0.25, 0.25.
// [1] The “echo state” approach to analysing and training recurrent neural networks – with an Erratum note by Jaeger (2010)
	for (int x = 0; x < connectionSize; ++x) {
		if (rand() % 2) continue;
		d_inputWeights[x] = 0.14 * ((rand() % 2)*2 - 1);
	}
#endif

	// Generate Feedback Weights between [-1,1]
	connectionSize = d_outputSize*d_reservoirSize;
	d_feedbackWeights = new WEIGHT_TYPE[connectionSize];
	for (int x = 0; x < connectionSize; ++x)
		d_feedbackWeights[x] = WEIGHT_TYPE(0);

	generateConnections(d_fbConnectivity, connectionSize, d_feedbackWeights);
	if(d_feedbackScale != 1 || d_feedbackShift != 0 )
		scaleAndShift(d_feedbackWeights, connectionSize, d_feedbackScale, d_feedbackShift);

	// Generate Output Weights between [-1,1]
	connectionSize = d_outputSize*d_reservoirSize + d_outputSize*d_inputSize;
	d_outputWeights = new WEIGHT_TYPE[connectionSize];
	for (int x = 0; x < connectionSize; ++x)
		d_outputWeights[x] = WEIGHT_TYPE(0);
	generateConnections(1, connectionSize, d_outputWeights);

	// generate thresholds for reservoir neurons
	d_thresholds = new WEIGHT_TYPE[d_reservoirSize];
#ifdef DEFAULT_INPUT_CONN
	for (int i = 0; i < d_reservoirSize; i++) d_thresholds[i] = THRESHOLD_VALUE;
#else
	for (int i = 0; i < nof_excited_neurons; i++) d_thresholds[i] = 1.0;
	for (int i = nof_excited_neurons; i < connectionSize; i++) d_thresholds[i] = 0.7;
#endif

	// Generate Reservoir Weights between [-1,1]
	generateReservoirConnections();

	// settling time (ST) must be reduced for high frequency use but it does
	// not improve the performance

	// set output values to 0
	d_output = new WEIGHT_TYPE[d_outputSize];
	for (int x = 0; x < d_outputSize; ++x)
		d_output[x] = WEIGHT_TYPE(0);
}

/**
 * The recurrent connections internal to the reservoir are by default independently
 * taken from a uniform distribution [-1 ... 1].
 *
 * The eigenvalue has to be near the predefined spectral radius for good echoing
 * Morse divides the weights by the spectral radius + 0.0001
 * Holzmann and Jaeger multiplies the weights with (spectral radius alpha/max Eigenval)
 */
void ESN::generateReservoirConnections() {
//	WEIGHT_TYPE maxEigenvalue = 0;
//	WEIGHT_TYPE max = 0;

	// Size of the reservoir proper
	int connectionSize = d_reservoirSize*d_reservoirSize;
	d_reservoirWeights = new WEIGHT_TYPE[connectionSize];

	reservoir.Init(d_reservoirWeights, d_reservoirSize, d_reservoirSize);

	reservoir.SetParameter(aNetwork::CONNECTIVITY, &d_connectivity);
	reservoir.SetParameter(aNetwork::SPECTRAL_RADIUS, &d_spectralRadius);
	reservoir.SetParameter(aNetwork::EXCITATORY_RATIO, &d_excitatory);

	bool okay = false;
	while (!okay)
	{
		int reservoirType = RESERVOIR_TYPE;
		switch (reservoirType) {
		case 0:
			reservoir.SetMode(aNetwork::CREATE_BALANCED_NETWORK);
			okay = reservoir.Run();
			reservoir.SetMode(aNetwork::NORMALIZE_SPECTRUM);
			okay = okay && reservoir.Run();
			break;
		case 1:
			reservoir.SetMode(aNetwork::CREATE_RANDOM);
			reservoir.Run();
			reservoir.SetMode(aNetwork::NORMALIZE_SPECTRUM);
			okay = reservoir.Run();
			break;
		}
	}
//	WEIGHT_TYPE new_max = 0;
//	spectralRadius(d_reservoirWeights, d_reservoirSize, &new_max);
//	cout << "After scaling the maximum eigen value is " << new_max << endl;
}

void ESN::uniform(WEIGHT_TYPE * value, float min, float max)
{
	WEIGHT_TYPE tmp = rand() / (WEIGHT_TYPE(RAND_MAX)+1); // between [0|1)
	*value = tmp*(max-min) + min;
}
// End Network initialization //


// Activation functions //

WEIGHT_TYPE ESN::act_heaviside(WEIGHT_TYPE value)
{
	return (value > 0 ? 1.0 : 0.0);
}

WEIGHT_TYPE ESN::act_logistic(WEIGHT_TYPE value)
{
	return 1.0 / (1.0 + exp(value) );
}

WEIGHT_TYPE ESN::act_invlogistic(WEIGHT_TYPE value)
{
	return log( 1.0/( value) - 1.0 );
}

WEIGHT_TYPE ESN::act_tanh(WEIGHT_TYPE value)
{
	return tanh( value );
}

WEIGHT_TYPE ESN::act_invtanh(WEIGHT_TYPE value)
{
	return atanh( value );
}

void ESN::setReservoirActivation(ActivationFunction reservoirActivation)
{
	this->d_reservoirActivation = reservoirActivation;

	switch(reservoirActivation)
	{
	case IDENTITY_ACTIVATION:
		resActFunc = &ESN::act_identity;
		break;
	case LOGISTIC_ACTIVATION:
		resActFunc = &ESN::act_logistic;
		break;
	case TANH_ACTIVATION:
		resActFunc = &ESN::act_tanh;
		break;
	case HEAVISIDE_ACTIVATION:
		resActFunc = &ESN::act_heaviside;
		break;
	default:
		cout << "Activation function is unknown" << endl;
	}
}

void ESN::setOutputActivation(ActivationFunction outputActivation)
{
	this->d_outputActivation = outputActivation;

	switch(outputActivation)
	{
	case IDENTITY_ACTIVATION:
		outInvActFunc = &ESN::act_invidentity;
		outActFunc = &ESN::act_identity;
		break;
	case LOGISTIC_ACTIVATION:
		outActFunc = &ESN::act_logistic;
		outInvActFunc = &ESN::act_invlogistic;
		break;
	case TANH_ACTIVATION:
		outActFunc = &ESN::act_tanh;
		outInvActFunc = &ESN::act_invtanh;
		break;
	default:
		cout << "Activation function is unknown" << endl;
	}
}
// End Activation Functions //

/**
 * Runs the echo state reservoir. It needs input as one of the fields in "all_trials". If teacher
 * forcing is used as "simType", then all_trials->outputVal needs to be set to the teacher values.
 * This array needs to be of the same dimensionality as the input. There is nothing learned
 * in this function, the ESN is just run. You will need to adapt the weights by e.g. linear
 * regression after you got the response of the reservoir on the given input.
 * This function uses the generic methods of Jaeger, with Holzmann parameters
 */
void ESN::Run(Trial *trial, SimulationType simType)
{
	assert (trial != NULL);

	WEIGHT_TYPE const * const input		= trial->inputVal;
	int inputSize 						= trial->inputSize;
	int timespan	 					= trial->sampleSize;
	int reservoirSize					= trial->stateSize;
	WEIGHT_TYPE * output				= trial->outputVal;
	WEIGHT_TYPE * states				= trial->neuronVal;

	assert (input != NULL);
	assert (output != NULL);
	assert (states != NULL);
	assert (d_thresholds != NULL);

	// For all the samples compute the states of all the Reservoir neurons
	for (int t = 0; t < timespan; ++t) {

		// For all the reservoirs neurons compute their activation
		for (int n = 0; n < reservoirSize; ++n) {
			WEIGHT_TYPE input2ResVal 	= 0;
			WEIGHT_TYPE res2ResVal 		= 0;
			WEIGHT_TYPE fb2ResVal 		= 0;

			// Compute the input from all the input neurons, W_in u(t)
			for (int inputNr = 0; inputNr < inputSize; ++inputNr) {
				input2ResVal += input[(t*inputSize) + inputNr]*d_inputWeights[(n*inputSize) + inputNr];
			}

			// From reservoir neurons also add one state..., W x(t-1)
			for (int innerResNr = 0; innerResNr < d_reservoirSize; ++innerResNr) {
				if(t > 0)
					res2ResVal += states[((t-1)*d_reservoirSize)+innerResNr] * d_reservoirWeights[(n*d_reservoirSize) + innerResNr];
			}

			// Compute the input from the output neurons (Feedback), W_back y(t-1)
			if(d_fbConnectivity > 0)
				for (int outputNNr = 0; outputNNr < d_outputSize; ++outputNNr) {
					if(t > 0)
						fb2ResVal += output[((t-1)*d_outputSize)+outputNNr]*d_feedbackWeights[(n*d_outputSize)+outputNNr];
				}

			// Calculate the reservoir neuron Activation
			// N.B. No noise term used
			// The leak term used by Verstraeten is different then that of Holzmann.
			// Verstraeten: Left over of the last state is used, thats normal, then the leak rate is multiplied with the new activation,
			// thats the part that got away. But Holzmann does not use the leak part as quotient for the inner reservoir neuron values
			// leftover = (1 − δCa)x(t-1)
			WEIGHT_TYPE leftOver = 0;
#ifdef DEFAULT_LEFTOVER
			if(t > 0) leftOver = (1-d_timeConstant*d_decayRate) * states[((t-1)*d_reservoirSize) + n];
#endif
			WEIGHT_TYPE noise = 0;
#ifdef ADD_NOISE
			if (simType == TEACHER_FORCING) noise = (drand48() - 0.5) / 5000;
#endif
			// x(t) = (1 − δCa)x(t-1) + δC(f (W_in u(t) + W x(t-1) + W_back y(t-1) + ν(t-1))
			// forget noise for now, also assume δ=1
			WEIGHT_TYPE total_input = (*this.*resActFunc)(input2ResVal + res2ResVal*d_timeConstant + fb2ResVal - d_thresholds[n] + noise);

			// For now a spike event is registered as interesting for debugging visually
			trial->debug[(t*d_reservoirSize)+n] = total_input;

			// add noise in case of teacher forcing, sampled from [-0.00001,+0.00001]
			states[(t*d_reservoirSize)+n] = leftOver + total_input;
		}

		// For all output neurons calculate their states
		bool setOutput = (d_fbConnectivity > 0);
		if (simType == TEACHER_FORCING) setOutput = false; // on teacher forcing do not adapt output
		if ((simType == TEACHER_TESTING) && (t < trial->teacherTestSize)) setOutput = false;
		if ( setOutput ) {
			for (int outputNNr = 0; outputNNr < d_outputSize; ++outputNNr) {
				WEIGHT_TYPE res2outputVal 	= 0;
				WEIGHT_TYPE input2outputVal = 0;

				for (int resNNr = 0; resNNr < d_reservoirSize; ++resNNr) {
					res2outputVal += states[(t*d_reservoirSize)+ resNNr]*d_outputWeights[(outputNNr*(d_reservoirSize + d_inputSize))+resNNr];
				}

				for (int inputNNr = 0; inputNNr < d_inputSize; ++inputNNr) {
					input2outputVal += input[(t*inputSize) + inputNNr]*d_outputWeights[(outputNNr*(d_reservoirSize + d_inputSize))+d_reservoirSize+inputNNr];
				}

				output[(t*d_outputSize) + outputNNr] = (*this.*outActFunc)(res2outputVal + input2outputVal);
				//				cout << "State output neuron becomes: " << outputStates->neuronVal[(x*d_outputSize) + outputNNr] << endl;
			}
		}

	}
}

void ESN::printStats()
{
	cout<< "___________Echo State Network__________"		<< endl
			<< "Reservoir size: " 			<< d_reservoirSize 		<< endl
			<< "Connectivity: " 			<< d_connectivity 		<< endl
			<< "Spectral Radius: " 			<< d_spectralRadius		<< endl
			<< "Activation function: " 		<< d_reservoirActivation<< endl
			<< "Input size: " 				<< d_inputSize 			<< endl
			<< "Input connectivity: "		<< d_inConnectivity		<< endl
			<< "Input Shift: "				<< d_inputShift			<< endl
			<< "Input Scale: "				<< d_inputScale			<< endl
			<< "Output size: " 				<< d_outputSize 		<< endl
			<< "Output activation funct: " 	<< d_outputActivation	<< endl
			<< "Feedback connectivity: "	<< d_fbConnectivity		<< endl
			<< "Feedback Shift: "			<< d_feedbackShift		<< endl
			<< "Feedback Scale: "			<< d_feedbackScale		<< endl
			<< "_______________________________________"		<< endl;
}

/* Saves the ESN to a binary file
 *
 */
void ESN::saveESN(string filename)
{
	ofstream outputFile(filename.c_str(), ios::out | ios::binary);
	if(!outputFile)
		printf( "Cannot open output file.\n");
	else
	{
		outputFile.write((char *) &d_inputSize, sizeof(int));
		outputFile.write((char *) &d_outputSize, sizeof(int));
		outputFile.write((char *) &d_reservoirSize, sizeof(int));

		outputFile.write((char *) &d_reservoirActivation, sizeof(ActivationFunction));
		outputFile.write((char *) &d_outputActivation, sizeof(ActivationFunction));

		outputFile.write((char *) &d_connectivity, sizeof(WEIGHT_TYPE));
		outputFile.write((char *) &d_inConnectivity, sizeof(WEIGHT_TYPE));
		outputFile.write((char *) &d_fbConnectivity, sizeof(WEIGHT_TYPE));

		outputFile.write((char *) &d_spectralRadius, sizeof(WEIGHT_TYPE));

		outputFile.write((char *) &d_inputScale, sizeof(WEIGHT_TYPE));
		outputFile.write((char *) &d_feedbackScale, sizeof(WEIGHT_TYPE));

		outputFile.write((char *) &d_inputShift, sizeof(WEIGHT_TYPE));
		outputFile.write((char *) &d_feedbackShift, sizeof(WEIGHT_TYPE));

		outputFile.write((char *) &d_timeConstant, sizeof(WEIGHT_TYPE));

		outputFile.write((char *) &d_feedbackScale, sizeof(WEIGHT_TYPE));

		// Store the weights
		saveWeights(&outputFile, d_inputSize*d_reservoirSize, d_inputWeights);
		saveWeights(&outputFile, d_outputSize*d_reservoirSize, d_feedbackWeights);
		saveWeights(&outputFile, d_outputSize*d_reservoirSize + d_outputSize*d_inputSize, d_outputWeights);
		saveWeights(&outputFile, d_reservoirSize*d_reservoirSize, d_reservoirWeights);
		outputFile.close();
	}
}

void ESN::setOutputWeights( WEIGHT_TYPE *weights, int len) {
	int nof_output_weights = d_outputSize*d_reservoirSize + d_outputSize*d_inputSize;
	assert (nof_output_weights == len);
	for (int i = 0; i < nof_output_weights; i++) {
		d_outputWeights[i] = weights[i];
	}
}

void ESN::saveWeights(ofstream *outputFile, int matrixSize, WEIGHT_TYPE *matrix)
{
	for (int x = 0; x < matrixSize; ++x)
	{
		(*outputFile).write((char *) &(matrix[x]), sizeof(WEIGHT_TYPE));
	}
}

/* Loads an ESN from a binary file
 *
 */
void ESN::loadESN(std::string filename)
{
	destroy();
	ifstream inputFile(filename.c_str(),std::ios::in | std::ios::binary);

	// Load from file
	if(!inputFile.fail())
	{
		printf("Loading ESN from file\n");
		inputFile.read((char *) &d_inputSize, sizeof(int));
		inputFile.read((char *) &d_outputSize, sizeof(int));
		inputFile.read((char *) &d_reservoirSize, sizeof(int));

		inputFile.read((char *) &d_reservoirActivation, sizeof(ActivationFunction));
		inputFile.read((char *) &d_outputActivation, sizeof(ActivationFunction));

		inputFile.read((char *) &d_connectivity, sizeof(WEIGHT_TYPE));
		inputFile.read((char *) &d_inConnectivity, sizeof(WEIGHT_TYPE));
		inputFile.read((char *) &d_fbConnectivity, sizeof(WEIGHT_TYPE));

		inputFile.read((char *) &d_spectralRadius, sizeof(WEIGHT_TYPE));

		inputFile.read((char *) &d_inputScale, sizeof(WEIGHT_TYPE));
		inputFile.read((char *) &d_feedbackScale, sizeof(WEIGHT_TYPE));

		inputFile.read((char *) &d_inputShift, sizeof(WEIGHT_TYPE));
		inputFile.read((char *) &d_feedbackShift, sizeof(WEIGHT_TYPE));

		inputFile.read((char *) &d_timeConstant, sizeof(WEIGHT_TYPE));

		inputFile.read((char *) &d_feedbackScale, sizeof(WEIGHT_TYPE));

		// Load the weights
		d_inputWeights = new WEIGHT_TYPE[d_inputSize*d_reservoirSize];
		loadWeights(&inputFile, d_inputSize*d_reservoirSize, d_inputWeights);
		d_feedbackWeights = new WEIGHT_TYPE[d_outputSize*d_reservoirSize];
		loadWeights(&inputFile, d_outputSize*d_reservoirSize, d_feedbackWeights);
		d_outputWeights = new WEIGHT_TYPE[d_outputSize*d_reservoirSize + d_outputSize*d_inputSize];
		loadWeights(&inputFile, d_outputSize*d_reservoirSize + d_outputSize*d_inputSize, d_outputWeights);
		d_reservoirWeights = new WEIGHT_TYPE[d_reservoirSize*d_reservoirSize];
		loadWeights(&inputFile, d_reservoirSize*d_reservoirSize, d_reservoirWeights);

	}
	else
		printf("Failed loading ESN input file\n");

	inputFile.close();
}

void ESN::loadWeights(std::ifstream *inputFile, int matrixSize, WEIGHT_TYPE *matrix)
{
	for (int x = 0; x < matrixSize; ++x)
	{
		(*inputFile).read((char *) &(matrix[x]), sizeof(WEIGHT_TYPE));
	}
}

void ESN::destroy()
{
	if(d_inputWeights != NULL)
	{
		delete [] d_inputWeights;
		d_inputWeights = NULL;
	}

	if(d_outputWeights != NULL)
	{
		delete [] d_outputWeights;
		d_outputWeights = NULL;
	}

	if(d_output != NULL)
	{
		delete [] d_output;
		d_output = NULL;
	}

	if(d_feedbackWeights != NULL)
	{
		delete [] d_feedbackWeights;
		d_feedbackWeights = NULL;
	}

	if(d_reservoirWeights != NULL)
	{
		delete [] d_reservoirWeights;
		d_reservoirWeights = NULL;
	}
}

ESN::~ESN()
{
	destroy();
}
