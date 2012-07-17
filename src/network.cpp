/**
 * @file Network.cpp
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


// General files
#include <stdlib.h>
#include <algorithm>
#include <time.h>
#include <iostream>
#include <assert.h>
#include <iomanip>

#include <network.h>
#include <ap.h>
#include <eigenvalues/nsevd.h>

using namespace std;
using namespace aNetwork;

/* **************************************************************************************
 * Implementation of Network
 * **************************************************************************************/

Network::Network() {
	srand48(time(NULL));
}

Network::~Network() {
	delete [] indices;
}

//! Initialize reservoir with size width*height
void Network::Init(WEIGHT_TYPE *weights, int width, int height) {
	this->weights = weights;
	this->width = width;
	this->height = height;
	this->size = width * height;
	indices = new int[size];
	for (int i = 0; i < size; i++) {
		indices[i] = i;
	}
}

//! Fill reservoir
bool Network::Run() {
	switch(mode) {
	case CREATE_RANDOM: return fillRandom();
	case CREATE_BALANCED_NETWORK: return createBalancedNetwork();
//	case SCALE_FREE: fillScaleFree(); break;
	case NORMALIZE_SPECTRUM: return normalizeSpectrum();
	default:
		cerr << "This connectivity type does not exist!" << endl;
	}
	return false;
}

void Network::SetParameter(NetworkParameter param, void *value) {
	switch(param) {
	case CONNECTIVITY:
		d_connectivity 		= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case SPECTRAL_RADIUS:
		d_spectralRadius	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	case EXCITATORY_RATIO:
		d_excitatoryRatio	= *reinterpret_cast<WEIGHT_TYPE*>(value);
		break;
	default:
		cout << "Unknown Parameter" << endl;
	}
}

bool Network::fillRandom() {
	if (d_connectivity == 0) return false;
	// First set everything to 0
	for (int x = 0; x < size; ++x) weights[x] = WEIGHT_TYPE(0);

	// And generate just a totally random connected reservoir without spatial characteristics
	std::random_shuffle(indices, indices+size);
	int nof_connections = size * d_connectivity;

	for (int x = 0; x < nof_connections; ++x) {
		uniform(&weights[indices[x]]);
	}
	return true;
}

bool Network::normalizeSpectrum() {
	WEIGHT_TYPE maxEigenValue = 0;
	if (!spectralRadius(maxEigenValue)) return false;
	if(maxEigenValue == 0) return false;

	for (int x = 0; x < size; ++x)
		weights[x] *= (d_spectralRadius/maxEigenValue);
	cout << "Spectral radius becomes: " << d_spectralRadius << endl;
	return true;
}

/**
 * Compute eigenvalues and if it is bigger 0 the reservoir is okay.
 * The weightMatrix will be scaled using the spectral radius
 *
 * NB: the leak rate is not considered in this calculation!
 */
bool Network::spectralRadius(WEIGHT_TYPE & spectralRadius) {
	assert (width == height);
	int nof_nodes = width;

	ap::real_2d_array a;
	a.setlength(nof_nodes, nof_nodes);
	for(int i = 0; i < nof_nodes; i++) {
		for(int j = 0; j < nof_nodes; j++) {
			a(i,j) = weights[i*nof_nodes + j];
		}
	}

	// Real part of the eigen value
	ap::real_1d_array wr;
	wr.setlength(nof_nodes);

	// Imaginary part of the eigen value
	ap::real_1d_array wi;
	wi.setlength(nof_nodes);

	// Eigen vectors, not needed
	ap::real_2d_array evL, evR;

	bool converged = rmatrixevd(a, nof_nodes, 0, wr, wi, evL, evR);

	WEIGHT_TYPE max = 0;
	for(int i = 0; i < nof_nodes; ++i) {
		WEIGHT_TYPE tmp = sqrt( pow(wr(i),2) + pow(wi(i),2) );
		if(tmp > max)
			max = tmp;
	}

	spectralRadius = max;
//	cout << "Spectral radius: " << spectralRadius << endl;
	return converged;
}

//! Implements S << B condition
bool much_smaller(const float small, const float big) {
	float minimal_ratio = 1.5;
	return big / small > minimal_ratio;
}

//! Implements S << M << B condition
bool much_smaller(const float small, const float medium, const float big) {
	return much_smaller(small, medium) && much_smaller(medium, big);
}

/**
 * Creates a balanced reservoir as in [1]. It
 *
 * [1] Chaotic Balanced State in a Model of Cortical Circuits (1998), Van Vreeswijk, Sompolinksy
 */
bool Network::createBalancedNetwork() {
	assert (width == height);
	int nof_nodes = width;
	int N_E = d_excitatoryRatio * nof_nodes;
	int N_I = nof_nodes - N_E;

	std::random_shuffle(indices, indices+size);
	int K = d_connectivity * nof_nodes;
	cout << "K (connectivity index): " << K << endl;
	cout << "N_E=" << N_E << " and N_I=" << N_I << endl;
	cout << " K/N_E is e.g.: " << K/(float)N_E << endl;
	cout << " K/N_I is e.g.: " << K/(float)N_I << endl;
	cout << " scaling is by 1.0 / sqrt((float)K: " << 1.0 / sqrt((float)K) << endl;

	// crank up N if K/N_k becomes to close to unity
	assert (much_smaller(1.0, K, N_I)); // equation 3.1
	assert (much_smaller(1.0, K, N_E));

	// authors use "inverted" notation, J_target,source
	float J_EE = 1.0; float J_IE = 1.0; // equation 2.6, set to 1
	float J_EI = -2.0; // values from Fig.17
	float J_II = -1.8;

	float J_E = -J_EI; // equation 2.7
	float J_I = -J_II;
	assert (J_E > 0); assert (J_I > 0);

	float avg_E = J_EE * K*(1/sqrt(K)) + J_EI * K*(1/sqrt(K));
	float avg_I = J_IE * K*(1/sqrt(K)) + J_II * K*(1/sqrt(K));
	cout << "All inputs activated for excitatory neurons: " << avg_E << endl;
	cout << "All inputs activated for inhibitory neurons: " << avg_I << endl;
	cout << "Expected average for whatever neuron: " << (N_E * avg_E + N_I * avg_I ) / (N_E + N_I) << endl;
	// balance is obtained by comparing with actual input to the reservoir!
//	assert (((E/I > J_E / J_I) && (J_E / J_I > 1)) || (J_E / J_I < 1));

	// Iterate over target neurons "n", and then over source neurons "i"
	for (int n = 0; n < N_E; ++n) {
		// source is excitatory
		for (int i = 0; i < N_E; ++i) {
			weights[(n*nof_nodes) + i] = 0;
			bool connect = drand48() <= (K / (float)N_E);
			if (connect) {
				weights[(n*nof_nodes) + i] = J_EE / sqrt((float)K);
			}
		}
		// source is inhibitory
		for (int i = N_E; i < nof_nodes; i++) {
			weights[(n*nof_nodes) + i] = 0;
			bool connect = drand48() <= (K / (float)N_I);
			if (connect) {
				weights[(n*nof_nodes) + i] = J_EI / sqrt((float)K);
			}
		}
	}
	for (int n = N_E; n < nof_nodes; ++n) {
		// source is excitatory
		for (int i = 0; i < N_E; ++i) {
			weights[(n*nof_nodes) + i] = 0;
			bool connect = drand48() <= (K / (float)N_E);
			if (connect) {
				weights[(n*nof_nodes) + i] = J_IE / sqrt((float)K);
			}
		}
		// source is inhibitory
		for (int i = N_E; i < nof_nodes; i++) {
			weights[(n*nof_nodes) + i] = 0;
			bool connect = drand48() <= (K / (float)N_I);
			if (connect) {
				weights[(n*nof_nodes) + i] = J_II / sqrt((float)K);
			}
		}
	}

//	printWeights(2);
	printDegrees();
	printActivity();

	WEIGHT_TYPE maxEigenValue = 0;
	spectralRadius(maxEigenValue);
	cout << "Spectral radius: " << maxEigenValue << endl;
	return true;
}

/**
 * Get weight value between given minimum and maximum. The default is -1 and +1.
 */
void Network::uniform(WEIGHT_TYPE * value, float min, float max) {
	WEIGHT_TYPE tmp = rand() / (WEIGHT_TYPE(RAND_MAX)+1); // between [0|1)
	*value = tmp*(max-min) + min;
}

/**
 * Print reservoir weights. There are several types of printing given by parameter
 * type:
 * @param type    	0: print weights as floating points
 * 					1: print 1 for connection, 0 for absence of connection
 * 					2: print +/- weights (positive versus negative)
 *
 */
void Network::printWeights(int type) {
	cout << "Network weights " << width << "x" << height << endl;
	// prints incoming weights over rows
	for (int n = 0; n < width; ++n) {
		for (int i = 0; i < height; ++i) {
			switch(type) {
			case 0:
				cout << weights[(n*width) + i] << " ";
				break;
			case 1:
				cout << (weights[(n*width) + i] != 0) << " ";
				break;
			case 2:
				WEIGHT_TYPE w = weights[(n*width) + i];
				if (w == 0) cout << "  "; // continue;
				else if (w < 0) cout << "- ";
				else cout << "+ ";
				break;
			}
		}
		cout << endl;
	}
}

void Network::printDegrees() {
	assert (width == height);
	int nof_nodes = width;
	int print_nof_nodes = nof_nodes;
//	cout << "Count degree (out)" << endl;
	int degree[nof_nodes];
	for (int n = 0; n < nof_nodes; ++n) {
		degree[n] = 0;
		for (int i = 0; i < nof_nodes; ++i) {
			if((weights[(n*nof_nodes) + i]) != 0 )
				degree[n]++;
		}
	}

//	cout << "Sum per degree" << endl;
	int degree_cnt[nof_nodes];
	for (int n = 0; n < nof_nodes; ++n) degree_cnt[n] = 0;
	for (int n = 0; n < nof_nodes; ++n) {
		degree_cnt[degree[n]]++;
	}
#ifdef PRINT_ALL
	cout << "Print per degree:";
	for (int n = 0; n < print_nof_nodes; ++n) {
		if (!(n % 10)) cout << endl << setw(3) << n << ": ";
		cout << setw(3) << degree_cnt[n] << " ";
	}
	cout << endl;
#endif
	float avg_degree = 0; int cnt = 0;
	for (int n = 0; n < nof_nodes; ++n) {
		avg_degree += degree_cnt[n] * n;
		cnt += degree_cnt[n];
	}
	avg_degree /= cnt;
	cout << "Average degree is " << avg_degree << " (should be 2*K)" << endl;
}

void Network::printActivity() {
	assert (width == height);
	int nof_nodes = width;
	int print_nof_nodes = nof_nodes;
	WEIGHT_TYPE activity[nof_nodes];
	for (int n = 0; n < nof_nodes; ++n) {
		activity[n] = 0;
		for (int i = 0; i < nof_nodes; ++i) {
			activity[n] += weights[(n*nof_nodes) + i]; // these are outgoing weights
//			activity[n] += weights[(i*nof_nodes) + n];
		}
	}

	int N_E = d_excitatoryRatio * nof_nodes;

	float avg_E = 0, avg_I = 0, avg = 0;
	for (int n = 0; n < N_E; ++n) {
		avg_E += activity[n];
		avg += activity[n];
	}
	for (int n = N_E; n < nof_nodes; ++n) {
		avg_I += activity[n];
		avg += activity[n];
	}
	avg_E /= N_E;
	avg_I /= (nof_nodes - N_E);
	avg /= nof_nodes;

	cout << "Average input activity for excitatory neurons: " << avg_E << endl;
	cout << "Average input activity for inhibitory neurons: " << avg_I << endl;
	cout << "Average for whatever neuron: " << avg << endl;

#ifdef PRINT_ALL
	cout << "Print activity per node (total=" << nof_nodes << "): ";
	cout.setf(ios::fixed,ios::floatfield);
	for (int n = 0; n < print_nof_nodes; ++n) {
		if (!(n % 10)) cout << endl << setw(3) << n << ": ";
		cout << setw(4) << activity[n] << " ";
	}
	cout << endl;
#endif
}
