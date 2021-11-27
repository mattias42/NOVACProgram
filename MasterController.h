#pragma once

/** The <b>CMasterController</b> class controlls the whole program.
    It starts and controlls the communication- and evaluation - threads. */
class CMasterController
{
public:
    CMasterController(void);
    ~CMasterController(void);

    /** The running state of the master controller */
    bool  m_fRunning;

    /** Starts the program */
    void  Start();

    /** Stops the program */
    void  Stop();

    /** Checks the settings */
    bool CheckSettings();
};
