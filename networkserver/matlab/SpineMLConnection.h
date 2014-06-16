/* -*-c++-*- */

/*
 * This is the connection class for connections to SpineML (as
 * generated by SpineCreator).
 *
 * This code is used by spinemlnetStart.cpp, a matlab mex
 * function. spinemlnetStart.cpp creates a main thread which listens
 * for incoming TCP/IP connections. When a new connection is received,
 * the main thread creates a SpineMLConnection object, which has its
 * own thread.
 *
 * This class contains the data relating to the connection; the
 * numbers being transferred to and from the SpineML experiment. It
 * also holds a reference to its thread and manages the handshake and
 * associated information (data direction, type, etc).
 *
 * The connection state starts out as !established and !failed. Once
 * the handshake with the SpineML client is completed, established is
 * set, and clientDataDirection etc should all be set. If comms with
 * the client fail, failed is set true, which will allow the main
 * thread to clean the connection up.
 */

#include <iostream>
#include <deque>
#include <vector>

extern "C" {
#include <unistd.h>
#include <errno.h>
}

using namespace std;

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
#define NO_DATA_MAX_COUNT     100

/*
 * A connection class. The SpineML client code connects to this server
 * with a separate connection for each stream of data. For example,
 * population A makes one connection to obtain its input, population B
 * makes a second connection for input. population C makes an output
 * connection. This class holds the file descriptor of the connection,
 * plus information (obtained during the connection handshake) about
 * the data direction, data type and data size.
 *
 * Each one of these connections is expected to run on a separate
 * thread, which means you can use blocking i/o for reading and
 * writing to the network.
 */
class SpineMLConnection
{
public:

    /*!
     * The constructor initialises some variables and initialises the
     * data mutex.
     */
    SpineMLConnection()
        : connectingSocket(0)
        , established(false)
        , failed(false)
        , unacknowledgedDataSent(false)
        , noData(0)
        , clientDataDirection (NOT_SET)
        , clientDataType (NOT_SET)
        , clientDataSize (1)
        {
            cout << "SpineMLConnection::SpineMLConnection constructor" << endl;
            pthread_mutex_init (&this->dataMutex, NULL);
        };

    /*!
     * The destructor closes the connecting socket (if necessary) then
     * destroys the data mutex.
     */
    ~SpineMLConnection()
        {
            cout << "SpineMLConnection::SpineMLConnection destructor. close socket..." << endl;
            if (this->connectingSocket > 0) {
                this->closeSocket();
            }
            cout << "SpineMLConnection::SpineMLConnection destructor. destroy mutex..." << endl;
            pthread_mutex_destroy(&this->dataMutex);
        };

    /*!
     * Simple accessors
     */
    //@{
    int getConnectingSocket (void);
    void setConnectingSocket (int i);
    char getClientDataDirection (void);
    char getClientDataType (void);
    unsigned int getClientDataSize (void);
    bool getEstablished (void);
    bool getFailed (void);
    //@}

    /*!
     * Run through the handshake process. This sets
     * clientDataDirection,Type and Size.
     */
    int doHandshake (void);

    /*!
     * Example this->data. If we have data to write, then write it to
     * the client.
     */
    int doWriteToClient (void) volatile;

    /*!
     * Perform input/output with the client. This will call either
     * doWriteToClient or doReadFromClient.
     */
    int doInputOutput (void);

    /*!
     * Close the connecting socket, set the connectingSocket value to
     * 0 and set established to false.
     */
    void closeSocket (void);

public:
    /*!
     * The thread on which this connection will execute.
     */
    pthread_t thread;

private:
    /*!
     * The file descriptor of the TCP/IP socket on which this
     * connection is running.
     */
    int connectingSocket;
    /*!
     * Set to true once the connection is fully established and the
     * handshake is complete.
     */
    bool established;
    /*!
     * Set to true if the connection fails - this will be due to a
     * failed read or write call.
     */
    bool failed;
    /*!
     * Every time data is sent to the client, set this to true. When a
     * RESP_RECVD response is received from the client, set this back
     * to false.
     */
    bool unacknowledgedDataSent;
    /*!
     * Used as a counter for when no data is received via a
     * connection.
     */
    unsigned int noData;
    /*!
     * The data direction, as set by the client. Client sends a flag
     * which is either AM_SOURCE (I am a source) or AM_TARGET (I am a
     * target).
     */
    char clientDataDirection; // AM_SOURCE or AM_TARGET
    /*!
     * There are 3 possible data types; nums(analog), spikes(events)
     * or impulses. Only nums implemented.
     */
    char clientDataType;
    /*!
     * The data size. This is the number of data per timestep. For
     * nums, that means it's the number of doubles per timestep.
     */
    unsigned int clientDataSize;
    /*!
     * A mutex for access to the data.
     */
    pthread_mutex_t dataMutex;
    /*!
     * The data which is accessed on the matlab side.
     */
    volatile std::deque<double> data;
};

/*!
 * Accessor implementations
 */
//@{
int
SpineMLConnection::getConnectingSocket (void)
{
    return this->connectingSocket;
}
void
SpineMLConnection::setConnectingSocket (int i)
{
    this->connectingSocket = i;
}
char
SpineMLConnection::getClientDataDirection (void)
{
    return this->clientDataDirection;
}
char
SpineMLConnection::getClientDataType (void)
{
    return this->clientDataType;
}
unsigned int
SpineMLConnection::getClientDataSize (void)
{
    return this->clientDataSize;
}
bool
SpineMLConnection::getEstablished (void)
{
    return this->established;
}
bool
SpineMLConnection::getFailed (void)
{
    return this->failed;
}
//@}

/*
 * Go through the handshake process, as defined in protocol.txt.
 *
 * The client.h code in SpineML_2_BRAHMS refers to 3 stages in the
 * handshake process: "initial handshake", "set datatype", "set
 * datasize". Here, these 3 stages are referred to in combination as
 * the "handshake".
 */
int
SpineMLConnection::doHandshake (void)
{
    ssize_t b = 0;
    char buf[16];
    // There are three stages in the handshake process:
    int handshakeStage = CS_HS_GETTINGTARGET;

    this->noData = 0;
    while (handshakeStage != CS_HS_DONE && this->noData < NO_DATA_MAX_COUNT) {

        if (handshakeStage == CS_HS_GETTINGTARGET) {
            if (!this->noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGTARGET. this->connectingSocket: "
                     << this->connectingSocket << endl;
            }
            b = read (this->connectingSocket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLConnection::doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == AM_SOURCE || buf[0] == AM_TARGET) {
                    this->clientDataDirection = buf[0];
                    // Write response.
                    buf[0] = RESP_HELLO;
                    if (write (this->connectingSocket, buf, 1) != 1) {
                        cout << "SpineMLConnection::doHandshake: Failed to write RESP_HELLO to client." << endl;
                        this->failed = true;
                        return -1;
                    }
                    cout << "SpineMLConnection::doHandshake: Wrote RESP_HELLO to client." << endl;
                    handshakeStage++;
                    this->noData = 0; // reset the "no data" counter
                } else {
                    // Wrong data direction.
                    this->clientDataDirection = NOT_SET;
                    cout << "SpineMLConnection::doHandshake: Wrong data direction in first handshake byte from client."
                         << endl;
                    this->failed = true;
                    return -1;
                }
            } else {
                // No byte read, increment the no_data counter.
                ++this->noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATATYPE) {
            if (!this->noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGDATATYPE." << endl;
            }
            cout << "SpineMLConnection::doHandshake: call read()" << endl;
            b = read (this->connectingSocket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLConnection::doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == RESP_DATA_NUMS || buf[0] == 'a') { // a is for test/debug
                    this->clientDataType = buf[0];
                    buf[0] = RESP_RECVD;
                    if (write (this->connectingSocket, buf, 1) != 1) {
                        cout << "SpineMLConnection::doHandshake: Failed to write RESP_RECVD to client." << endl;
                        this->failed = true;
                        return -1;
                    }
                    cout << "SpineMLConnection::doHandshake: Wrote RESP_RECVD to client." << endl;
                    handshakeStage++;
                    this->noData = 0; // reset the "no data" counter

                } else if (buf[0] == RESP_DATA_SPIKES || buf[0] == RESP_DATA_IMPULSES) {
                    // These are not yet implemented.
                    cout << "SpineMLConnection::doHandshake: Spikes/Impulses not yet implemented." << endl;
                    this->failed = true;
                    return -1;

                } else {
                    // Wrong/unexpected character.
                    cout << "SpineMLConnection::doHandshake: Character '" << buf[0]
                         << "'/0x" << (int)buf[0] << " is unexpected here." << endl;
                    this->failed = true;
                    return -1;
                }
            } else {
                if (this->noData < 10) {
                    cout << "SpineMLConnection::doHandshake: Got " << b << " bytes, not 1" << endl;
                }
                ++noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATASIZE) {
            if (!this->noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGDATASIZE." << endl;
            }
            b = read (this->connectingSocket, (void*)buf, 4);
            if (b == 4) {
                // Got 4 bytes. This is the data size - the number of
                // doubles to transmit during each timestep. E.g.: If
                // a population has 10 neurons, then this is probably
                // 10. Interpret as an unsigned int, with the first
                // byte in the buffer as the least significant byte:
                this->clientDataSize =
                    (unsigned char)buf[0]
                    | (unsigned char)buf[1] << 8
                    | (unsigned char)buf[2] << 16
                    | (unsigned char)buf[3] << 24;

                cout << "SpineMLConnection::doHandshake: client data size: "
                     << this->clientDataSize << " doubles/timestep" << endl;

                buf[0] = RESP_RECVD;
                if (write (this->connectingSocket, buf, 1) != 1) {
                    cout << "SpineMLConnection::doHandshake: Failed to write RESP_RECVD to client." << endl;
                    this->failed = true;
                    return -1;
                }
                handshakeStage++;
                this->noData = 0;

            } else if (b > 0) {
                // Wrong number of bytes.
                cout << "SpineMLConnection::doHandshake: Read " << b << " bytes, expected 4." << endl;
                for (ssize_t i = 0; i<b; ++i) {
                    cout << "buf[" << i << "]: '" << buf[i] << "'  0x" << hex << buf[i] << dec << endl;
                }
                this->failed = true;
                return -1;
            } else {
                ++this->noData;
            }

        } else if (handshakeStage == CS_HS_DONE) {
            cout << "SpineMLConnection::doHandshake: Handshake finished." << endl;
        } else {
            cout << "SpineMLConnection::doHandshake: Error: Invalid handshake stage." << endl;
            this->failed = true;
            return -1;
        }
    }
    if (this->noData >= NO_DATA_MAX_COUNT) {
        cout << "SpineMLConnection::doHandshake: Error: Failed to get data from client." << endl;
        this->failed = true;
        return -1;
    }

    // This connection is now established:
    this->established = true;

    return 0;
}

/*
 * Write a datum to the client and then read the acknowledgement byte.
 */
int
SpineMLConnection::doWriteToClient (void) volatile
{
    // Expect an acknowledgement from the client if we sent data:
    if (this->unacknowledgedDataSent == true) {
        char buf[16];
        int b = read (this->connectingSocket, (void*)buf, 1);
        if (b == 1) {
            // Good; we got data.
            if (buf[0] != RESP_RECVD) {
                cout << "SpineMLConnection::doWriteToClient: Wrong response from client." << endl;
                return -1;
            }
            // Got the acknowledgement, set this to false again:
            this->unacknowledgedDataSent = false;
            // And reset our noData variable:
            this->noData = 0;

        } else if (b == 0 && this->noData < NO_DATA_MAX_COUNT) {
            // No data available yet, so simply return 0; that means
            // we'll come back to trying to read the acknowledgement
            // later.
            //cout << "SpineMLConnection::doWriteToClient: No data on wire right now." << endl;
            this->noData++;
            return 0;

        } else if (b == 0 && this->noData == NO_DATA_MAX_COUNT) {
            int theError = errno;
            cout << "SpineMLConnection::doWriteToClient: Failed to read data from client. Hit max number of tries. errno: "
                 << theError << endl;
            return -1;

        } else {
            int theError = errno;
            cout << "SpineMLConnection::doWriteToClient: Failed to read 1 byte from client. errno: "
                 << theError << " bytes read: " << b << endl;
            return -1;
        }
    } // else we're not waiting for a RESP_RECVD response from the server.

    // This is just me putting some static data in for testing/development.
    vector<double> dbls;
    for (int i = 0; i < this->clientDataSize; ++i) {
        dbls.push_back(5.0);
    }

    // Write some data to the client:
    //cout << "SpineMLConnection::doWriteToClient: write... this->connectingSocket: " << this->connectingSocket << endl;
    ssize_t bytesWritten = write (this->connectingSocket, &(dbls[0]), this->clientDataSize*sizeof(double));
    if (bytesWritten != this->clientDataSize*sizeof(double)) {
        int theError = errno;
        cout << "SpineMLConnection::doWriteToClient: Failed. Wrote " << bytesWritten
             << " bytes. Tried to write " << (this->clientDataSize*sizeof(double)) << ". errno: "
             << theError << endl;
        return -1;
    }
    //cout << "SpineMLConnection::doWriteToClient: wrote " << bytesWritten << " bytes." << endl;

    // Set that we now need an acknowledgement from the client:
    this->unacknowledgedDataSent = true;

    return 0;
}

int
SpineMLConnection::doInputOutput (void)
{
    // Check if this is an established connection.
    if (this->established == false) {
        cout << "SpineMLConnection::doInputOutput: connection is not established, returning 0." << endl;
        return 0;
    }

    // We're going to update some data in memory
    pthread_mutex_lock (&this->dataMutex);

    // Update the buffer by reading/writing from network.
    if (this->clientDataDirection == AM_TARGET) {
        // Client is a target, I need to write data to the client, if I have anything to write.
        //cout << "SpineMLConnection::doInputOutput: clientDataDirection: AM_TARGET." << endl;
        if (this->doWriteToClient()) {
            cout << "SpineMLConnection::doInputOutput: Error writing to client." << endl;
            this->failed = true;
            return -1;
        }

    } else if (this->clientDataDirection == AM_SOURCE) {
        // Client is a source, I need to read data from the client.
        // this->doReadFromClient();
        cout << "SpineMLConnection::doInputOutput: clientDataDirection: AM_SOURCE." << endl;
    } else {
        // error.
        cout << "SpineMLConnection::doInputOutput: clientDataDirection has wrong value: "
             << (int)this->clientDataDirection << endl;
    }

    pthread_mutex_unlock (&this->dataMutex);
    return 0;
}

void
SpineMLConnection::closeSocket (void)
{
    if (close (this->connectingSocket)) {
        int theError = errno;
        cout << "SpineMLConnection::closeSocket: Error closing connecting socket: " << theError << endl;
    } else {
        this->connectingSocket = 0;
    }
    this->established = false;
}
