/**
 * @file ESNPrediction.h
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


#ifndef ESN_TRAIN_H_
#define ESN_TRAIN_H_

// General files
#include <esn.h>
#include <ap.h>
#include <vector>
#include <string.h>

/* **************************************************************************************
 * Interface of ESNPrediction
 * **************************************************************************************/

class ESNPrediction {
public:
	//! Constructor ESNPrediction
	ESNPrediction(int reservoirSize, float connectivity);

	//! Destructor ~ESNPrediction
	virtual ~ESNPrediction();

	//! Apply ridge regression on given set
	void RidgeRegression(std::vector<Trial*> & trials, ap::real_2d_array *W);

	//! Add all_trials
	void AddTrial(WEIGHT_TYPE *input, WEIGHT_TYPE *output, int len, int id = -1);

	//! Run trials
	void RunTrials();

	//! Run a test from the test set
	void RunTest(int index, float *input, float *result);

	//! Same run of a test, but returns also states
	void RunTest(int index, float *input, float *result, float *states);

	//! Get results from test set
	std::vector<Trial*> & GetTestSet();

	inline ESN & GetESN() { return esn; }

protected:
	//! Divide all trials in test and training sets
	void InitSets();

	std::vector<Trial*> & GetTrainingSet();

	void WriteToFile(ap::real_2d_array *W, std::string file);
private:
	//! The echo state reservoir
	ESN esn;

	//! A series of "trials", pieces of the same time series
	std::vector<Trial*> all_trials;

	int *set;
};

#endif /* ESN_TRAIN_H_ */
