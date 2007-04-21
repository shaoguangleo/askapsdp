#include <measurementequation/MESVDSolver.h>

#include <casa/aips.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

#include <iostream>


namespace conrad
{
namespace synthesis
{


void MESVDSolver::init() {
	itsNormalEquations.reset();
	itsDesignMatrix.reset();
}

bool MESVDSolver::solveNormalEquations(MEQuality& quality) {
	uint rank;
	rank = 0;
	return true;
};

bool MESVDSolver::solveDesignMatrix(MEQuality& quality) {

	uint nParameters=0;
	uint nData=0;
	nData=itsDesignMatrix.residual().nelements();
	
	// Find all the scalar parameters
	const vector<string> names(itsParams.names());
	vector<string>::const_iterator it;
	map<string, uint> indices;
	for (it=names.begin();it!=names.end();it++) {
		if(itsParams.isScalar(*it)) {
			indices[*it]=nParameters;
			nParameters++;
		}
	}

    // Convert the design matrix to gsl format
	gsl_matrix * A = gsl_matrix_alloc (nData, nParameters);
	map<string, uint>::iterator indit;
	for (indit=indices.begin();indit!=indices.end();indit++) {
		casa::Vector<double> asVector(itsDesignMatrix.derivative(indit->first));
//		std::cout << "Adding constraints for " << indit->first << std::endl;
		for (uint i=0;i<nData;i++) {
			gsl_matrix_set(A, i, indit->second, asVector(i));
		}
	}
	
	// Call the gsl SVD function, A is overwritten by U
	gsl_matrix * V = gsl_matrix_alloc (nParameters, nParameters);
	gsl_vector * S = gsl_vector_alloc (nParameters);
	gsl_vector * work = gsl_vector_alloc (nParameters);
	gsl_linalg_SV_decomp (A, V, S, work);

	// Now find the solution
	gsl_vector * res = gsl_vector_alloc(nData);
	for (uint i=0;i<nData;i++) {
		gsl_vector_set(res, i, itsDesignMatrix.residual()[i]);
	}
	gsl_vector * x = gsl_vector_alloc(nParameters);
	gsl_linalg_SV_solve (A, V, S, res, x); 
	
	for (indit=indices.begin();indit!=indices.end();indit++) {
		double value=gsl_vector_get(x, indit->second);
		value+=itsParams.scalarValue(indit->first);
		itsParams.update(indit->first, value);
	}
	uint rank=0;
	double smin=1e50;
	double smax=0.0;
	for (uint i=0;i<nParameters;i++) {
		double sValue=abs(gsl_vector_get(S, i));
		if(sValue>0.0) 
		{
			rank++;
			if(sValue>smax) smax=sValue;
			if(sValue<smin) smin=sValue;
		}
	}
	quality.setRank(rank);
	quality.setCond(smax/smin);
	gsl_vector_free(S);
	gsl_vector_free(work);
	gsl_matrix_free(V);
	gsl_matrix_free(A);
	quality.setInfo("SVD decomposition");

	return true;
};

}
}