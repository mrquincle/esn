/**
 * @file ESNPrediction.cpp
 * @brief 
 *
 * This file is created at Almende B.V. It is open-source software and part of the Common 
 * Hybrid Agent Platform (CHAP). A toolbox with a lot of open-source tools, ranging from 
 * thread pools and TCP/IP components to control architectures and learning algorithms. 
 * This software is published under the GNU Lesser General Public license (LGPL).
 *
 * It is not possible to add usage restrictions to an open-source license. Nevertheless,
 * we personally strongly object against this software used by the military, in the
 * bio-industry, for animal experimentation, or anything that violates the Universal
 * Declaration of Human Rights.
 *
 * Copyright © 2010 Anne van Rossum <anne@almende.com>
 *
 * @author 	Anne C. van Rossum
 * @date	May 23, 2011
 * @project	Replicator FP7
 * @company	Almende B.V.
 * @case	
 */


// General files
#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include <esn_train.h>
#include <inv.h>
#include <vector>

using namespace std;

// Show debug information or neuron states
#define SHOW_DEBUG			0

#if SHOW_DEBUG == 0
#undef SHOW_DEBUG
#endif

/* **************************************************************************************
 * Implementation of ESNPrediction
 * **************************************************************************************/

/**
 * For prediction purposes we assume a single input and a single output.
 */
ESNPrediction::ESNPrediction(int reservoirSize,
		float connectivity):
		esn(1, 1, reservoirSize, connectivity),
		all_trials(),
		set(NULL) {
	esn.setFbConnectivity(1);
	esn.setFeedbackScale(0.56);
	esn.setInputScale(1);
	esn.setDecayRate(0.9); // 0.9
	esn.setTimeConstant(0.44);
	esn.setSpectralRadius(0.79); // 0.79
	esn.init();
//	esn.printStats();
	all_trials.clear();
//	cout << "Prediction unit initialised" << endl;
}

/**
 * TODO: Remove trials with neuronVal[]s.
 */
ESNPrediction::~ESNPrediction() {

}

/**
 * The number of neurons is defined previously as the reservoir size. In case of a teacher
 * only 1/5 of the output will be used for teacher forcing.
 *
 * @param input			Input sequence
 * @param output		Requested or enforced output sequence
 * @param len			Time length of these sequences
 * @param id			Identifier, only used in classification
 */
void ESNPrediction::AddTrial(WEIGHT_TYPE *input, WEIGHT_TYPE *output, int len, int id) {
	Trial *t = new Trial();
	t->stateSize  		= esn.getReservoirSize();
	t->neuronVal  		= new WEIGHT_TYPE[esn.getReservoirSize()*len];
	t->classId			= id; // "misused" for identification purposes
	t->inputVal			= input;
	t->inputSize		= 1;
	t->sampleSize		= len;
	t->outputVal    	= output;
	t->teacherTestSize 	= len / 5;
	t->debug   			= new WEIGHT_TYPE[esn.getReservoirSize()*len];
	all_trials.push_back(t);
}

/**
 * Create vector "set" with values "1" for every all_trials that is a test set. All the others will be
 * used for training.
 */
void ESNPrediction::InitSets() {
	if (set != NULL) free (set);
	set = NULL;
	int n = all_trials.size();
	if (n <= 1) {
		cerr << "We at least need one training set and one test set" << endl;
		return;
	}
	set = (int *) calloc(n, sizeof(int));

	bool hit = false;
	float p = 20; // % of trials being a test set
	for (int i = 0; i < n; i++) {
		int r = rand() % 100;
		if (r < p) {
			set[i] = 1;
			hit = true;
		}
	}

	// If not hitting anything, just make one of the sets a test set
	if (!hit) {
		int r = rand() % n;
		set[r] = 1;
	}

	for (int i = 0; i < n; i++) {
		if (set[i] == 1)
			cout << "Test: " << all_trials[i]->classId << endl;
		else
			cout << "Train: " << all_trials[i]->classId << endl;
	}

}

std::vector<Trial*> & ESNPrediction::GetTrainingSet() {
	std::vector<Trial*> * trainSet = new vector<Trial*>();
	trainSet->clear();
	for (unsigned int i = 0; i < all_trials.size(); i++) {
		if (set[i] == 0) {
			trainSet->push_back(all_trials[i]);
		}
	}
	return *trainSet;
}

std::vector<Trial*> & ESNPrediction::GetTestSet() {
	std::vector<Trial*> * testSet = new vector<Trial*>();
	testSet->clear();
	for (unsigned int i = 0; i < all_trials.size(); i++) {
		if (set[i] == 1) {
			testSet->push_back(all_trials[i]);
		}
	}
	return *testSet;
}

/**
 * Run all trials for training.
 */
void ESNPrediction::RunTrials() {
	InitSets();
	std::vector<Trial*> & trainSet = GetTrainingSet();

	for (unsigned int i = 0; i < trainSet.size(); i++) {
		esn.Run(trainSet[i], TEACHER_FORCING);
	}

	ap::real_2d_array W;
	RidgeRegression(trainSet,&W);

	int len = W.gethighbound(1) - W.getlowbound(1) + 1;
	float weights[len];
	for (int i = W.getlowbound(1); i <= W.gethighbound(1); i++) {
		weights[i] = W(i,0);
	}

	esn.setOutputWeights(weights, len);

	// calculate the error

}

/**
 * Run the indicated test. The TEACHER_TESTING mode forces teacher input for the
 * first so-many samples and then let the system continue for itself.
 */
void ESNPrediction::RunTest(int index, float *input, float *result) {
	std::vector<Trial*> & testSet = GetTestSet();
	int len = testSet[index]->sampleSize;
	for (int i = 0; i < len; i++) input[i] = testSet[index]->outputVal[i];
	esn.Run(testSet[index], TEACHER_TESTING);
	for (int i = 0; i < len; i++) result[i] = testSet[index]->outputVal[i];

	// test if the first samples are indeed the same
    bool test_teacher_forcing = false;
    if (test_teacher_forcing) {
    	len = testSet[index]->teacherTestSize;
    	for (int i = 0; i < len; i++) assert(input[i] == result[i]);
    }
}

/**
 * See RunTest
 * This function includes also the states of all the neurons. For e.g. visualisation
 * purposes.
 */
void ESNPrediction::RunTest(int index, float *input, float *result, float *states) {
	std::vector<Trial*> & testSet = GetTestSet();
	int len = testSet[index]->sampleSize;
	for (int i = 0; i < len; i++) input[i] = testSet[index]->outputVal[i];
	esn.Run(testSet[index], TEACHER_TESTING);
	for (int i = 0; i < len; i++) result[i] = testSet[index]->outputVal[i];

	for (int i = 0; i < len * esn.getReservoirSize(); i++) {
#ifdef SHOW_DEBUG
		states[i] = testSet[index]->debug[i];
#else
		states[i] = testSet[index]->neuronVal[i];
//		assert (states[i] >= 0.0); // not true
#endif
	}
}

/**
 * Same function but with "size" arguments, rather than "boundary" arguments. Just take a
 * look at the matrixmatrixmultiply arguments and you will understand...
 * This function also captures the error rather than segfaults if you hand over the wrong
 * dimensions for the matrices.
 * TODO: Just indicate what dimensions to expect for cx and cy. And warn if they are
 * incorrect.
 */
void matrixmatrixmultiply(const ap::real_2d_array& a,
	     int ax,
	     int ay,
	     bool transa,
	     const ap::real_2d_array& b,
	     int bx,
	     int by,
	     bool transb,
	     double alpha,
	     ap::real_2d_array& c,
	     int cx,
	     int cy,
	     double beta,
	     ap::real_1d_array& work) {
	try {
		matrixmatrixmultiply(a, 0, ax-1, 0, ay-1, transa,
				b, 0, bx-1, 0, by-1, transb, alpha,
				c, 0, cx-1, 0, cy-1, beta, work);
	}
	catch (ap::ap_error err) {
		cout << "Exception raised: " << string(err.msg) << endl;
		return;
	}
}

/**
 * Use ridge regression to compute the weights using the projected reservoir node values as desired value.
 * A Tikhonov matrix T = λI is chosen. If λI = 0 this reduces to unregularized least squares.
 * Ridge regression for echo state networks [1]:
 *   J_ridge(ω) = 1/2 (Aω − B)' (Aω − B) + 1/2λ ||ω||^2
 * This leads to the following matrix solution:
 *   ω_ridge = (A'A + λI)^−1 A'B
 * Here A denotes all inputs to the readout, in other words, all the reservoir states. And B is the matrix
 * that contains the desired outputs.
 *
 * [1] Stable Output Feedback in Reservoir Computing Using Ridge Regression by Wyffels et al. (2008)
 */
void ESNPrediction::RidgeRegression(std::vector<Trial*> & trials, ap::real_2d_array *W) {
	if (trials.empty()) return;
	cout << "Ridge regression on trial set of size " << trials.size() << endl;

	// for now assume that all trials are of the same length...
//	assert

	// assume output is desired output of dimension 1
	assert ( esn.getOutputSize() == 1);

	// set dimensions of matrices / vectors
	int nof_trials 		= trials.size();
	int trial_len 		= trials[0]->sampleSize;
	int nof_neurons		= esn.getReservoirSize() + esn.getInputSize();
	int nof_out_neurons = esn.getOutputSize();

	int skip_samples	= trial_len / 4;

	// lambda (or alpha) is actually not allowed to be fixed but depends on reservoir
	double lambda		= 0.2;

	cout << "Skip " << skip_samples << " sample" << ((skip_samples == 1) ? "" : "s") << endl;

	// The ap library expects first rows, than columns
	ap::real_2d_array A, B;

	// Fill the matrix A with all the reservoir states
	A.setlength(nof_trials * (trial_len - skip_samples), nof_neurons);
	for (unsigned int tr = 0; tr < trials.size(); tr++) {
		for (int t = skip_samples; t < trial_len; t++) {
			int i = tr * (trial_len - skip_samples) + t - skip_samples;
			for (int n = 0; n < esn.getReservoirSize(); n++) {
				// row, column
				A(i,n) = trials[tr]->neuronVal[t*esn.getReservoirSize()+n];
			}
			// We also add the inputs to the input matrix
			for (int n = 0; n < esn.getInputSize(); n++) {
				A(i,n+esn.getReservoirSize()) = trials[tr]->inputVal[t*esn.getInputSize()+n];
			}

		}
	}

//	WriteToFile(&A, "esn_A.txt");

	// Fill the "matrix" B with all the outputs, in this case the desired ones
	B.setlength(nof_trials * (trial_len - skip_samples), nof_out_neurons);
	for (unsigned int tr = 0; tr < trials.size(); tr++) {
		for (int t = skip_samples; t < trial_len; t++) {
			for (int n = 0; n < nof_out_neurons; n++) {
				int i = tr * (trial_len - skip_samples) + t - skip_samples;
				B(i,0) = trials[tr]->outputVal[t*nof_out_neurons+n];
			}
		}
	}

	cout << "Create correlation matrix A'*A" << endl;
	ap::real_2d_array AtA;

	AtA.setlength(nof_neurons, nof_neurons);
	ap::real_1d_array work;
	work.setlength(nof_trials * (trial_len - skip_samples) + nof_neurons);
	matrixmatrixmultiply(
			A,   nof_trials * (trial_len - skip_samples), nof_neurons, true,
			A,   nof_trials * (trial_len - skip_samples), nof_neurons, false, 1.0,
			AtA, nof_neurons, nof_neurons, 0.0, work);

//	WriteToFile(&AtA, "esn_AtA.txt");

	// Add smoothing factor λI (so only to diagonal elements)
	cout << "Add \\lambda*I to A'*A" << endl;
	for (int x = 0; x < nof_neurons; ++x) AtA(x,x) += lambda;

	cout << "Create inverse of A'*A" << endl;
	rmatrixinverse(AtA, nof_neurons);

	cout << "Calculate A'*B" << endl;
	ap::real_2d_array AtB;
	AtB.setlength(nof_neurons, nof_out_neurons);
	matrixmatrixmultiply(
			A, nof_trials * (trial_len - skip_samples), nof_neurons,  true,
			B, nof_trials * (trial_len - skip_samples), nof_out_neurons, false, 1.0,
			AtB, nof_neurons, nof_out_neurons, 0.0, work);

//	WriteToFile(&AtB, "esn_AtB.txt");

	cout << "Obtaining W" << endl;
	W->setlength(nof_neurons, nof_out_neurons);
	matrixmatrixmultiply(
			AtA, nof_neurons, nof_neurons,     false,
			AtB, nof_neurons, nof_out_neurons, false, 1.0,
			*W,  nof_neurons, nof_out_neurons, 0.0, work);
}

/**
 * Just default function to write a matrix to a file. Values on the same row are separated by
 * spaces. Implementation detail: observe how the index need to run not only till, but also
 * including the higher bounds. Observe also that Matlab notation is used starting from an
 * index of "1" for the horizontal coordinate, and "2" for the vertical (instead of starting
 * with "0").
 */
void ESNPrediction::WriteToFile(ap::real_2d_array *W, string file) {
	bool numbering = false;
	if (W == NULL) {
		cerr << "Weight matrix is NULL" << endl;
		return;
	}
	ofstream outputFile(file.c_str(), ios::out );
	if(!outputFile) {
		printf( "Cannot open output file.\n");
		return;
	}

#ifdef TEST_SERIALISATION_ARRAY
	if (test) {
		// Serialize
		outputFile << W;

		// Deserialize
		outputFile.close();

		ifstream inputFile(file.c_str(), ios::in);
		ap::real_2d_array W0;
		inputFile >> W0;
		for (int i = W->getlowbound(1); i <= W->gethighbound(1); i++) {
			for (int j = W->getlowbound(2); j <= W->gethighbound(2); j++) {
				assert ((*W)(i,j) == (W0)(i,j));
			}
		}
		return;
	}
#endif

	for (int i = W->getlowbound(1); i <= W->gethighbound(1); i++) {
		if (numbering)
			outputFile << i << "[" << W->gethighbound(2) - W->getlowbound(2) + 1 << "]: ";

		for (int j = W->getlowbound(2); j <= W->gethighbound(2); j++) {
			std::stringstream ss; ss.clear(); ss.str("");
			ss << (*W)(i,j) << " ";
			outputFile.write((char *) ss.str().c_str(), ss.str().length());
		}
		outputFile.put('\n');
	}
	outputFile.close();
}

