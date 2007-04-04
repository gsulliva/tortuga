#include <p30fxxxx.h>

_FOSC( CSW_FSCM_OFF & EC_PLL8 );
_FWDT ( WDT_OFF );


#define TRIS_OUT 0
#define TRIS_IN  1
#define byte unsigned char

/*
 * Bus = D1 D0 E5-E0
 * Akn = D3
 * RW  = E8
 */

/* Bus pin assignments */
#define IN_AKN      _RD3
#define LAT_AKN     _LATD3
#define TRIS_AKN    _TRISD3

#define IN_RW       _RE8
#define TRIS_RW     _TRISE8
#define LAT_RW      _LATE8


#define RW_READ     0
#define RW_WRITE    1

/*
 * Bus Constants
 * BUS_TIMEOUT - how many iterations to wait when waiting for AKN to change state
 *
 * BUS_ERROR   - AKN failed to go high when talking to Slave. Most likely indicates a
 *               Slave fault. Can also mean that some Slave is forcibly holding AKN
 *               low, but this is very unlikely to happen.
 *
 * BUS_FAILURE - A Slave is holding the AKN line high, preventing any further bus
 *               operations. This is catastrophic failure and a Reset may be needed.
 *               It may be possible to retry (in case Slave bus code got interrupted
 *               in the middle of an operation, but this is extremely unlikely (and
 *               should be avoided by disabling that interrupt anyway).
 */
#define BUS_TIMEOUT     100000
#define BUS_ERROR       -1
#define BUS_FAILURE     -2

/*
 * Commands Master can send to Slaves. Command numbers among different Slaves can overlap
 * but common commands like Ping/Identify should be the same everywhere.
 * Kind of obvious what order I did these in, huh?
 */
#define BUS_CMD_PING            0
#define BUS_CMD_ID              1
#define BUS_CMD_READ_REG        2
#define BUS_CMD_WRITE_REG       3
#define BUS_CMD_MARKER1         4
#define BUS_CMD_DEPTH           5
#define BUS_CMD_LCD_WRITE       6
#define BUS_CMD_LCD_REFRESH     7
#define BUS_CMD_LCD_LIGHT_ON    8
#define BUS_CMD_LCD_LIGHT_OFF   9
#define BUS_CMD_THRUSTERS_ON    10
#define BUS_CMD_THRUSTERS_OFF   11
#define BUS_CMD_MARKER2         12

#define NUM_SLAVES  3


/* Read byte from bus */
byte readBus()
{
    return (PORTE & 0x3F) | (_RD0 << 6) | (_RD1 << 7);
}


/* Take bus out of high-impedance state and write a byte to it */
void writeBus(byte b)
{
    TRISE = TRISE & 0xFFC0;
    _TRISD1 = TRIS_OUT;
    _TRISD0 = TRIS_OUT;

     LATE = (LATE & 0xFFC0) | (b & 0x3F);
    _LATD0 = (b & 0x40) >> 6;
    _LATD1 = (b & 0x80) >> 7;
}


/* Put bus in high-impedance state */
void freeBus()
{
    _TRISD1 = TRIS_IN;
    _TRISD0 = TRIS_IN;
    TRISE = TRISE | 0x3F;
}


/* Wait for a byte on the serial console */
unsigned char waitchar()
{
    byte x;
    U1STAbits.OERR = 0;
    U1STAbits.FERR = 0;
    U1STAbits.PERR = 0;
    U1STAbits.URXDA = 0;

    while(U1STAbits.URXDA == 0);
    x = U1RXREG;
    U1STAbits.URXDA = 0;
    return x & 0x7F;
}


/* Initialize bus */
void initBus()
{
    /* Put everything in high-impedance state */
    freeBus();
    TRIS_RW = TRIS_OUT;
    TRIS_AKN = TRIS_IN;
}


/* Set the given Slave's Req line to the given value */
void setReq(byte req, byte val)
{
    if(req == 0)
        _LATB0 = val;

    if(req == 1)
        _LATB1 = val;

    if(req == 2)
        _LATB2 = val;
}


/* Read a byte from a given Slave */
/* Returns BUS_ERROR or BUS_FAILURE on error */
int busReadByte(byte req)
{
    byte data=0;
    long timeout = 0;

    /* Set RW to read */
    LAT_RW = RW_READ;

    /* Raise Req */
    setReq(req, 1);

    /* Wait for AKN to go high */
    /* Need timeout to detect Slave fault */
    while(IN_AKN == 0)
    {
        if(timeout++ == BUS_TIMEOUT)
        {
            setReq(req, 0);
            return BUS_ERROR;
        }
    }

    /* Read the data */
    data = readBus();

    /* Drop Req */
    setReq(req, 0);

    /* Wait for Slave to release bus */
    timeout=0;
    while(IN_AKN == 1)
    {
        if(timeout++ == BUS_TIMEOUT)
            return BUS_FAILURE;     /* We're totally screwed */
    }

    return data;
}


/* Write a byte to a given slave */
/* Returns BUS_ERROR or BUS_FAILURE on error */
int busWriteByte(byte data, byte req)
{
    long timeout=0;

    /* Set RW to write */
    LAT_RW = RW_WRITE;

    /* Put the data on the bus */
    writeBus(data);

    /* Raise Req */
    setReq(req, 1);

    /* Wait for AKN to go high */
    /* Need timeout to detect Slave fault */
    while(IN_AKN == 0)
    {
        if(timeout++ == BUS_TIMEOUT)
        {
            setReq(req, 0);
            freeBus();
            return BUS_ERROR;
        }
    }

    /* Release bus */
    freeBus();

    /* Drop Req */
    setReq(req, 0);

    /* Wait for Slave to release bus */
    timeout=0;
    while(IN_AKN == 1)
    {
        if(timeout++ == BUS_TIMEOUT)
            return BUS_FAILURE;     /* We're totally screwed */
    }

    return 0;
}


void initUart()
{
    U1MODE = 0x0000;
    U1BRG = 185;
    U1MODEbits.ALTIO = 1;   // Use alternate IO
    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;   // Enable transmit
}


/* Send a byte to the serial console */
void sendByte(byte i)
{
    long j;
    while(U1STAbits.UTXBF);
    U1TXREG = i;
    while(U1STAbits.UTXBF);
    for(j=0; j<10000; j++); /* This line can be removed, but my uart was being weird. */
}


/* Send a string to the serial console */
void sendString(unsigned char str[])
{
    byte i=0;
    for(i=0; str[i]!=0; i++)
        sendByte(str[i]);
}

/* General purpose bus receive buffer */
byte rxBuf[60];


/*
 * Read data from bus into rxBuf and return number of bytes read.
 * Returns BUS_ERROR or BUS_FAILURE on error
 */
int readDataBlock(byte req)
{
    int rxPtr, rxLen, rxData;
    rxBuf[0]=0;
    rxLen = busReadByte(req);

    if(rxLen < 0)
        return rxLen;

    for(rxPtr=0; rxPtr<rxLen; rxPtr++)
    {
        rxData = busReadByte(req);

        if(rxData < 0)
            return rxData;

        rxBuf[rxPtr] = rxData;
    }

    rxBuf[rxLen]=0;
    return rxLen;
}


int main(void)
{
    long j=0;
    long t=0, b=0;
    byte i;

    byte tmp[60];
    byte rxPtr = 0;
    byte rxLen = 0;

    initBus();

    setReq(0, 0);
    setReq(1, 0);
    setReq(2, 0);
    initUart();

    ADPCFG = 0xFFFF;
    LATB = 0;
    TRISB = 0;


    initUart();

    sendString("\n\rMaster starting...\n\r");
    for(j=0; j<100000; j++);


    /*
     * Key Commands:
     * P - ping each slave
     * I - identify each slave and print ID
     * R - read a given config register of a given slave. Default values='A'
     * W - write a given config register of a given slave to a given value
     * S - send 15000 Identify commands to slave 0, read result of each. Used for testing timing.
     * M - tell slave 0 to drop the first marker. mainly used for testing timers, but works too
     * m - tell slave 0 to drop the second marker. mainly used for testing timers, but works too
     * B - turn LCD backlight on
     * b - turn LCD backlight off
     * L - write text to LCD
     * (running out of meaningful letters over here!)
     * T - thruster safety on
     * t - thruster safety off
     *
     */

    while(1)
    {
        sendString("\n\r\n\r>: ");
        byte c = waitchar();
        sendByte(c);    /* Local Echo */

        switch(c)
        {

            case 'L':
            {
                sendString("\n\r\n\rEnter up to 32 characters:\n\r>");


                byte data[] = "                                ";

                for(i=0; i<32; i++)
                {
                    data[i] = waitchar();
                    sendByte(data[i]);
                    if(data[i] =='\r')
                    {
                        data[i] = ' ';
                        break;
                    }
                }
                sendString("\n\r\n\rWriting data to LCD");

                for(i=0; i<32; i++)
                {
                    busWriteByte(BUS_CMD_LCD_WRITE, 2);
                    busWriteByte(i, 2);
                    busWriteByte(data[i], 2);
                }

                busWriteByte(BUS_CMD_LCD_REFRESH, 2);

                sendString("\n\rDone");

                break;
            }


            case 'B':
            {
                sendString("\n\rLCD Backlight on");
                busWriteByte(BUS_CMD_LCD_LIGHT_ON, 2);
                break;
            }


            case 'b':
            {
                sendString("\n\rLCD Backlight off");
                busWriteByte(BUS_CMD_LCD_LIGHT_OFF, 2);
                break;
            }

            case 'T':
            {
                sendString("\n\rThruster safety on");
                busWriteByte(BUS_CMD_THRUSTERS_ON, 0);
                break;
            }

            case 't':
            {
                sendString("\n\rThruster safety off");
                busWriteByte(BUS_CMD_THRUSTERS_OFF, 0);
                break;
            }

            case 'P':
            {
                sendString("\n\rPinging all slaves");
                for(i=0; i<NUM_SLAVES; i++)
                {
                    sprintf(tmp, "\n\rSlave #%d: ", i);
                    sendString(tmp);


                    switch(busWriteByte(BUS_CMD_PING, i))
                    {
                        case BUS_ERROR:
                            sendString("Comm Error: TX Timeout");
                        break;

                        case BUS_FAILURE:
                            sendString("Catastrophic bus failure: AKN held high");
                        break;

                        case 0:
                        {
                            byte len = readDataBlock(i);

                            switch(len)
                            {
                                case 0:
                                    sendString("Reply OK");
                                break;

                                case BUS_ERROR:
                                    sendString("Comm Error: RX Timeout");
                                break;

                                case BUS_FAILURE:
                                    sendString("Comm Error: Bus Failure during reply");
                                break;

                                default:
                                    sendString("Unknown Reply");
                            }
                        }
                        break;
                    }
                }

                break;
            }

            case 'I':
            {
                sendString("\n\rIdentifying slaves");
                for(i=0; i<NUM_SLAVES; i++)
                {
                    busWriteByte(BUS_CMD_ID, i);
                    byte len = readDataBlock(i);
                    sprintf(tmp, "\n\rSlave #%d replies (%d bytes): <", i, len);
                    sendString(tmp);
                    sendString(rxBuf);
                    sendString(">");
                }
                break;
            }

            case 'R':
            {
                sendString("\n\rId=");
                byte id = waitchar() & 0x0F;
                sendByte(id+48);

                sendString("\n\rReg=");
                byte addr = waitchar() & 0x0F;
                sendByte(addr+48);


                sendString("\n\rReading config...");
                busWriteByte(BUS_CMD_READ_REG, id);
                busWriteByte(addr, id);

                readDataBlock(id);

                sprintf(tmp, "\n\rSlave #%d config register %d is <%c", id, addr, rxBuf[0]);
                sendString(tmp);
                sendByte('>');
                break;
            }

            case 'W':
            {
                sendString("\n\rId=");
                byte id = waitchar() & 0x0F;
                sendByte(id+48);

                sendString("\n\rReg=");
                byte addr = waitchar() & 0x0F;
                sendByte(addr+48);

                sendString("\n\rVal=");
                byte val = waitchar();
                sendByte(val);

                sendString("\n\rWriting config...");
                busWriteByte(BUS_CMD_WRITE_REG, id);
                busWriteByte(addr, id);
                busWriteByte(val, id);

                sprintf(tmp, "\n\rSlave #%d config register %d set to <%c>", id, addr, val);
                sendString(tmp);
                break;
            }


            case 'S':
            {
                sendString("\n\rSpeed test starting now.");
                int j;
                for(j=0; j<15000; j++)
                {

                    if(busWriteByte(BUS_CMD_ID, 0) != 0)
                    {
                        sprintf(tmp, "\n\rTransmit error at iteration %i", j);
                        sendString(tmp);
                        break;
                    }

                    /* Invalidate these */
                    rxBuf[0] = 65;
                    rxBuf[1] = 65;
                    byte len = readDataBlock(0);

                    if(len != 22)
                    {
                        sprintf(tmp, "\n\rWrong data length at iteration %i: read %i bytes", j, len);
                        sendString(tmp);
                        break;
                    }
                }

                sendString("\n\rDone");
                break;
            }

            case 'M':
            {
                sendString("\n\rDropping first marker");
                busWriteByte(BUS_CMD_MARKER1, 0);
                sendString("\n\rDone.");
                break;
            }

            case 'm':
            {
                sendString("\n\rDropping second marker");
                busWriteByte(BUS_CMD_MARKER2, 0);
                sendString("\n\rDone.");
                break;
            }

            case 'D':
            {
                sendString("\n\rAverage of last 100 depth measurements on Slave 0: ");
                busWriteByte(BUS_CMD_DEPTH, 0);
                readDataBlock(0);

                int depth = (rxBuf[0]<<8) | rxBuf[1];
                sprintf(tmp, "%u", depth);
                sendString(tmp);
                sendString("\n\rDone.");
                break;
            }


            default:
            {
                sendString("\n\rUnknown command: ");
                sendByte(c);
            }
        }
    }

    while(1);
}