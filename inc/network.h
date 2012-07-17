/**
 * @file reservoir.h
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
 * @date	Jun 8, 2011
 * @project	Replicator FP7
 * @company	Almende B.V.
 * @case	
 */


#ifndef NETWORK_H_
#define NETWORK_H_

// General files

namespace aNetwork {

/* **************************************************************************************
 * Interface of Network
 * **************************************************************************************/

/**
 * Different modes of operation.
 */
enum Mode {CREATE_RANDOM, SCALE_FREE, NORMALIZE_SPECTRUM, CREATE_BALANCED_NETWORK};

enum NetworkParameter {
	CONNECTIVITY,
	SPECTRAL_RADIUS,
	EXCITATORY_RATIO
};

//! Actually I'd prefer template < typename WEIGHT_TYPE > however, then separation between
//! .h and .cpp files gets lost and compilation time goes up...
typedef float WEIGHT_TYPE;

/**
 * Network with weights on the edges / bonds. The representation is in double array format,
 * so fits better fully connected networks than sparsely connected networks.
 */
class Network {
public:
	//! Constructor reservoir
	Network();

	//! Destructor ~reservoir
	virtual ~Network();

	//! Initialize reservoir with size width*height
	void Init(WEIGHT_TYPE *weights, int width, int height);

	//! Fill reservoir
	bool Run();

	//! Set connectivity type
	inline void SetMode(const Mode mode) { this->mode = mode; }

	//! One function for all possible reservoir settings (different sets per reservoir type)
	void SetParameter(NetworkParameter param, void *value);
protected:
	//! Randomly connected reservoir
	bool fillRandom();

	bool normalizeSpectrum();

	bool createBalancedNetwork();

	bool spectralRadius(WEIGHT_TYPE & spectralRadius);

	//! Random variable between min and max
	void uniform(WEIGHT_TYPE *value, float min=-1, float max=1);

	void printWeights(int type = 0);

	void printActivity();

	void printDegrees();
private:
	//! Array with reservoir weights
	WEIGHT_TYPE *weights;

	//! Network connectivity (default CREATE_RANDOM)
	Mode mode;

	//! Connectivity
	WEIGHT_TYPE d_connectivity;

	//! Spectral radius can be used for normalisation
	WEIGHT_TYPE d_spectralRadius;

	//! Ratio of excitatory neurons
	WEIGHT_TYPE d_excitatoryRatio;

	//! Keep an array of indices around
	int *indices;

	//! Width, height, and size of matrix
	int width, height, size;
};

}

#endif /* NETWORK_H_ */
