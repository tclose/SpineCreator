/*
 * Query TCP/IP server thread.
 */

#include "mex.h"
#include <iostream>
#include <string.h>

using namespace std;

void
mexFunction (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    //cout << "spinemlnetQuery Called" << endl;
    unsigned long long int *threadPtr; // Or some other type?
    if (nrhs==0) {
        mexErrMsgTxt("failed");
    }

    //cout << "getting threadPtr..." << endl;
    threadPtr = (unsigned long long int*)mxGetData(prhs[0]);
    pthread_t *thread = ((pthread_t*) threadPtr[0]);
    volatile bool *stopRequested = ((volatile bool*) threadPtr[1]);
    volatile bool *updated = ((volatile bool*) threadPtr[2]);
    volatile bool *threadFailed = ((volatile bool*) threadPtr[5]);
    pthread_mutex_t *bufferMutex = ((pthread_mutex_t *) threadPtr[4]);

    bool tf = *threadFailed; // Seems to be necessary to make a copy
                             // of the thing pointed to by
                             // threadFailed.

    // The data for the output array. 2 numbers for now.
    const mwSize res[2] = { 1, 2 }; // 1 by 2 matrix

    // create the output array
    // args: ndims, const mwSize *dims, mxClassID classid, complexity flag
    //cout << "create numeric array..." << endl;
    plhs[0] = mxCreateNumericArray (2, res, mxUINT16_CLASS, mxREAL); 

    // set up a pointer to the output array
    unsigned short *outPtr = (unsigned short*) mxGetData(plhs[0]);
    
    // copy new data into the output structure
    unsigned short rtn[2] = { (unsigned short)*threadFailed, (unsigned short)*updated };
    //cout << "memcpy..." << endl;
    memcpy(outPtr, (const void*)rtn, 4); // 2 bytes per short * 2 elements in rtn. 
}
