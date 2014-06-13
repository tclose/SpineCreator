/*
 * Start tcpip server
 */

// This is a Matlab mex function.
#include "mex.h"

#include <iostream>
#include <deque>
#include <vector>

extern "C" {
#include <pthread.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
}

using namespace std;

// thread handle and termination flag, must be global
pthread_t thread;
// flags
volatile bool stopRequested; // User requested stop from matlab space
volatile bool threadFinished;// The thread finished executing (maybe it failed), need to inform matlab space
volatile bool updated;       // Data was updated. May or may not need this - comes from mkinect.
volatile char clientDataDirection; // AM_SOURCE or AM_TARGET
volatile char clientDataType;      // nums(analog), spikes(events) or impulses. Only nums implemented.
volatile char clientDataSize;      // If clientDataType is nums, this is the size of those nums in number of bytes per num.
// mutexes
pthread_mutex_t bufferMutex;
// data structures
volatile char* bufferData;
// Whatever data type is used by SpineML:
volatile std::deque<int> data;

// flag for if we are all up and running
volatile bool initialised;

// A connected socket.
volatile int connecting_socket;

// Allow up to 1024 bytes in the listen queue.
#define LISTENQ 1024

// SpineML tcp/ip comms flags.
#define RESP_DATA_NUMS     31 // a non-printable character
#define RESP_DATA_SPIKES   32 // ' ' (space)
#define RESP_DATA_IMPULSES 33 // '!'
#define RESP_HELLO         41 // ')'
#define RESP_RECVD         42 // '*'
#define RESP_ABORT         43 // '+'
#define RESP_FINISHED      44 // ','
#define AM_SOURCE          45 // '-'
#define AM_TARGET          46 // '.'
#define NOT_SET            99 // 'c'

// SpineML tcp/ip comms data types
enum dataTypes {
    ANALOG,
    EVENT,
    IMPULSE
};

// Handshake stages:
#define CS_HS_GETTINGTARGET     0
#define CS_HS_GETTINGDATATYPE   1
#define CS_HS_GETTINGDATASIZE   2
#define CS_HS_DONE              3

// How many times to fail to read a byte before calling the session a
// failure:
#define NO_DATA_MAX_COUNT   10000

/*
 * Go through the handshake process, as defined in protocol.txt.
 *
 * The client.h code in SpineML_2_BRAHMS refers to 3 stages in the
 * handshake process: "initial handshake", "set datatype", "set
 * datasize". Here, these 3 stages are referred to in combination as
 * the "handshake".
 */
int
doHandshake (void)
{
    ssize_t b = 0;
    char buf[16];
    // There are three stages in the handshake process:
    int handshakeStage = CS_HS_GETTINGTARGET;
    int noData = 0; // Incremented everytime we get no data. Used so
                    // that we don't spin forever waiting for the
                    // handshake.
    while (handshakeStage != CS_HS_DONE && noData < NO_DATA_MAX_COUNT) {

        if (handshakeStage == CS_HS_GETTINGTARGET) {
            if (!noData) {
                cout << "SpineMLNet: start-doHandshake: CS_HS_GETTINGTARGET. connecting_socket: " << connecting_socket << endl;
            }
            b = read (connecting_socket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLNet: start-doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == AM_SOURCE || buf[0] == AM_TARGET) {
                    clientDataDirection = buf[0];
                    // Write response.
                    buf[0] = RESP_HELLO;
                    if (write (connecting_socket, buf, 1) != 1) {
                        cout << "SpineMLNet: start-doHandshake: Failed to write RESP_HELLO to client." << endl;
                        return -1;
                    }
                    cout << "SpineMLNet: start-doHandshake: Wrote RESP_HELLO to client." << endl;
                    handshakeStage++;
                    noData = 0; // reset the "no data" counter
                } else {
                    // Wrong data direction.
                    clientDataDirection = NOT_SET;
                    cout << "SpineMLNet: start-doHandshake: Wrong data direction in first handshake byte from client." << endl;
                    return -1;
                }
            } else {
                // No byte read, increment the no_data counter.
                ++noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATATYPE) {
            if (!noData) {
                cout << "SpineMLNet: start-doHandshake: CS_HS_GETTINGDATATYPE." << endl;
            }
            cout << "SpineMLNet: start-doHandshake: call read()" << endl;
            b = read (connecting_socket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLNet: start-doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == RESP_DATA_NUMS || buf[0] == 'a') { // a is for test/debug
                    clientDataType = buf[0];
                    buf[0] = RESP_RECVD;
                    if (write (connecting_socket, buf, 1) != 1) {
                        cout << "SpineMLNet: start-doHandshake: Failed to write RESP_RECVD to client." << endl;
                        return -1;
                    }
                    cout << "SpineMLNet: start-doHandshake: Wrote RESP_RECVD to client." << endl;
                    handshakeStage++;
                    noData = 0; // reset the "no data" counter

                } else if (buf[0] == RESP_DATA_SPIKES || buf[0] == RESP_DATA_IMPULSES) {
                    // These are not yet implemented.
                    cout << "SpineMLNet: start-doHandshake: Spikes/Impulses not yet implemented." << endl;
                    return -1;

                } else {
                    // Wrong/unexpected character.
                    cout << "SpineMLNet: start-doHandshake: Character '" << buf[0] << "'/0x" << (int)buf[0] << " is unexpected here." << endl;
                    return -1;
                }
            } else {
                if (noData < 10) {
                    cout << "SpineMLNet: start-doHandshake: Got " << b << " bytes, not 1" << endl;
                }
                ++noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATASIZE) {
            if (!noData) {
                cout << "SpineMLNet: start-doHandshake: CS_HS_GETTINGDATASIZE." << endl;
            }
            b = read (connecting_socket, (void*)buf, 4);
            if (b == 4) {
                // Got 4 bytes. This is the data size.

                clientDataSize =
                    (unsigned char)buf[0]
                    | (unsigned char)buf[1] << 8
                    | (unsigned char)buf[2] << 16
                    | (unsigned char)buf[3] << 24;
                // Note: clientDataSize is the number of doubles per datum.
                cout << "SpineMLNet: start-doHandshake: client data size: " << clientDataSize << endl;

                buf[0] = RESP_RECVD;
                if (write (connecting_socket, buf, 1) != 1) {
                    cout << "SpineMLNet: start-doHandshake: Failed to write RESP_RECVD to client." << endl;
                    return -1;
                }
                handshakeStage++;
                noData = 0;

            } else if (b > 0) {
                // Wrong number of bytes.
                cout << "SpineMLNet: start-doHandshake: Read " << b << " bytes, expected 4." << endl;
                for (ssize_t i = 0; i<b; ++i) {
                    cout << "buf[" << i << "]: '" << buf[i] << "'  0x" << hex << buf[i] << dec << endl;
                }
                return -1;
            } else {
                ++noData;
            }

        } else if (handshakeStage == CS_HS_DONE) {
            cout << "SpineMLNet: start-doHandshake: Handshake finished." << endl;
        } else {
            cout << "SpineMLNet: start-doHandshake: Error: Invalid handshake stage." << endl;
            return -1;
        }
    }
    if (noData >= NO_DATA_MAX_COUNT) {
        cout << "SpineMLNet: start-doHandshake: Error: Failed to get data from client." << endl;
        return -1;
    }
    return 0;
}

/*
 * Close the network sockets. Uses a global for connecting_socket;
 * listening socket is passed in.
 */
void
closeSockets (int& listening_socket)
{
    cout << "SpineMLNet: start-closeSockets: Called" << endl;
    if (close (connecting_socket)) {
        int theError = errno;
        cout << "SpineMLNet: start-closeSockets: Error closing connecting socket: " << theError << endl;
    }
    if (close (listening_socket)) {
        int theError = errno;
        cout << "SpineMLNet: start-closeSockets: Error closing listening socket: " << theError << endl;
    }
    cout << "SpineMLNet: start-closeSockets: Returning" << endl;
}

/*
 * Write a datum to the client and then read the acknowledgement byte.
 */
int
doWriteToClient (void)
{
    // This is just me putting some static data in for testing/development.
    vector<double> dbls;
    for (int i = 0; i < clientDataSize; ++i) {
        dbls.push_back(5.0);
    }

    cout << "SpineMLNet: start-doWriteToClient: write... connecting_socket: " << connecting_socket << endl;
    ssize_t bytesWritten = write (connecting_socket, &(dbls[0]), clientDataSize*sizeof(double));
    if (bytesWritten != clientDataSize*sizeof(double)) {
        int theError = errno;
        cout << "SpineMLNet: start-doWriteToClient: Failed. Wrote " << bytesWritten
             << " bytes. Tried to write " << (clientDataSize*sizeof(double)) << ". errno: "
             << theError << endl;
        return -1;
    }
    cout << "SpineMLNet: start-doWriteToClient: wrote " << bytesWritten << " bytes." << endl;
    // Expect an acknowledgement now from the client
    char buf[16];
    int b = read (connecting_socket, (void*)buf, 1);
    if (b == 1) {
        // Good.
        cout << "SpineMLNet: start-doWriteToClient: Read '" << buf[0] << "' from client" << endl;
    } else {
        cout << "SpineMLNet: start-doWriteToClient: Failed to read 1 byte from client." << endl;
        return -1;
    }

    return 0;
}

/*
 * thread function - this is where the TCP/IP comms happens. This is a
 * matlab-free zone.
 */
void*
theThread (void* nothing)
{
    // INIT CODE
    threadFinished = false;

    // Set up and await connection from a TCP/IP client. Use
    // the socket(), bind(), listen() and accept() calls.
    cout << "SpineMLNet: start-theThread: Open a socket." << endl;
    int listening_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        // error.
        cout << "SpineMLNet: start-theThread: Failed to open listening socket." << endl;
        threadFinished = true;
        return NULL;
    }

    // This is the port on which this server will listen.
    int port = 50099;

    struct sockaddr_in servaddr;
    bzero((char *) &servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    int bind_rtn = bind (listening_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bind_rtn < 0) {
        int theError = errno;
        cout << "SpineMLNet: start-theThread: Failed to bind listening socket (error " << theError << ")." << endl;
        threadFinished = true;
        return NULL;
    }

    int listen_rtn = listen (listening_socket, LISTENQ);
    if (listen_rtn < 0) {
        cout << "SpineMLNet: start-theThread: Failed to listen to listening socket." << endl;
        threadFinished = true;
        return NULL;
    }

    // MAIN LOOP CODE

    // We poll for activity on the connection, so that if the user
    // Ctrl-Cs we don't block on an accept() call.
    struct pollfd p;
    p.fd = listening_socket;
    p.events = POLLIN|POLLPRI;
    p.revents = 0;
    int retval = 0;

    // loop until we get the termination signal
    bool connected = false;
    while (!stopRequested) {

        if (!connected) {

            p.revents = 0;

            if ((retval = poll (&p, 1, 0)) > 0) {
                // This is ok.
                // cout << "Got positive value from select() or poll()"<< endl;
            } else if (retval == -1) {
                cout << "SpineMLNet: start-theThread: error with poll()/select()" << endl;
            } else {
                // This is ok.
                // cout << "poll returns 0." << endl;
            }

            if (p.revents & POLLIN || p.revents & POLLPRI) {
                // Data ready to read...
                connecting_socket = accept (listening_socket, NULL, NULL);
                if (connecting_socket < 0) {
                    cout << "SpineMLNet: start-theThread: Failed to accept on listening socket." << endl;
                    threadFinished = true;
                    return NULL;
                }

                connected = true;
                cout << "SpineMLNet: start-theThread: Accepted a connection." << endl;

                // Now we have a connection, we can proceed to carry out
                // the SpineML TCP/IP network comms handshake
                //
                if (doHandshake() < 0) {
                    cout << "SpineMLNet: start-theThread: Failed to complete SpineML handshake." << endl;
                    cout << "SpineMLNet: start-theThread: Set threadFinished to true." << endl;
                    threadFinished = true;
                    cout << "SpineMLNet: start-theThread: Direct call to closeSockets..." << endl;
                    closeSockets (listening_socket);
                    cout << "SpineMLNet: start-theThread: return NULL from theThread." << endl;
                    return NULL;
                }
                cout << "SpineMLNet: start-theThread: Completed handshake." << endl;

            } else {
                // cout << "no data available." << endl;
            }

        } else { // connected

            ///////// update some data in memory
            pthread_mutex_lock (&bufferMutex);

            // Update the buffer by reading/writing from network.
            if (clientDataDirection == AM_TARGET) {
                // Client is a target, I need to write data to the client, if I have anything to write.
                cout << "SpineMLNet: start-theThread: clientDataDirection: AM_TARGET." << endl;
                if (doWriteToClient()) {
                    cout << "SpineMLNet: start-theThread: Error writing to client." << endl;
                    break;
                }
                usleep (1000);

            } else if (clientDataDirection == AM_SOURCE) {
                // Client is a source, I need to read data from the client.
                //doReadFromClient();
                cout << "SpineMLNet: start-theThread: clientDataDirection: AM_SOURCE." << endl;
            } else {
                // error.
                cout << "SpineMLNet: start-theThread: clientDataDirection error." << endl;
            }

            pthread_mutex_unlock (&bufferMutex);
            initialised = true;
        }

    } // while (!stopRequested)

    // Shutdown code
    cout << "SpineMLNet: start-theThread: Close sockets." << endl;
    threadFinished = true; // Here, its "threadFinished" really
    closeSockets (listening_socket);
    cout << "SpineMLNet: start-theThread: At end of thread." << endl;
    return NULL;
}

void
mexFunction (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // initialise flags
    stopRequested = false;
    initialised = false;
    updated = false;
    threadFinished = false;

    // set up mutexes
    pthread_mutex_init (&bufferMutex, NULL);

    // create the thread
    cout << "SpineMLNet: start-mexFunction: creating thread..." << endl;
    int rtn = pthread_create (&thread, NULL, &theThread, NULL);
    if (rtn < 0) {
        cout << "SpineMLNet: start-mexFunction: Failed to create thread." << endl;
        return;
    }

    // wait until initialisation in the thread is complete before continuing
    do {
        usleep (1000);
        if (threadFinished == true) {
            // Shutdown as we have an error.
            cout << "SpineMLNet: start-mexFunction: Shutdown due to error." << endl;
            // destroy mutexes
            pthread_mutex_destroy(&bufferMutex);
            // clear the loop
            initialised = true;
        }
    } while (!initialised);

    // details of output
    mwSize dims[2] = { 1, 16 };
    plhs[0] = mxCreateNumericArray (2, dims, mxUINT64_CLASS, mxREAL);
    // pointer to pass back the context to matlab
    unsigned long long int* threadPtr = (unsigned long long int*) mxGetData (plhs[0]);

    // store thread information - this passed back to matlab as threadPtr is plhs[0].
    threadPtr[0] = (unsigned long long int)&thread;
    threadPtr[1] = (unsigned long long int)&stopRequested;
    threadPtr[2] = (unsigned long long int)&updated;
    threadPtr[3] = (unsigned long long int)bufferData;
    threadPtr[4] = (unsigned long long int)&bufferMutex;
    threadPtr[5] = (unsigned long long int)&threadFinished;
}
