/*
 *   Driver for Midiman Portman2x4 parallel port midi interface
 *
 *   Copyright (c) by Levent Gündogdu <levon@feature-it.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * ChangeLog
 * Feb 20 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - ported from alsa 0.5 to 1.0
 * Mar 17 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - added checks for opened input device in interrupt handler
 * Mar 18 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - added parport_unregister_driver to the startup routine if the driver fails to detect a portman
 *      - added support for all 4 output ports in portman_putmidi
 * Mar 24 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - added 2.6 kernel support
 * Sep 03 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - fixed compilation problem with alsa 1.0.6a (removed MODULE_CLASSES,
 *        MODULE_PARM_SYNTAX and changed MODULE_DEVICES to
 *        MODULE_SUPPORTED_DEVICE)
 * Sep 30 2004 Tobias Gehrig <tobias@gehrig.tk>
 *      - source code cleanup
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>
#include <linux/parport.h>
#include <linux/delay.h>
#include <sound/memalloc.h>

#define chip_t portman_t

MODULE_AUTHOR("Levent Gündogdu, Tobias Gehrig");
MODULE_DESCRIPTION("Midiman Portman2x4");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{Midiman,Portman2x4}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE;	/* Enable switches */

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for Portman2x4 midi interface.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for Portman2x4 midi interface.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable Portman2x4 midi interface.");

typedef struct _snd_portman portman_t;

static portman_t *portman;

struct _snd_portman {
	spinlock_t reg_lock;

	struct snd_card *card;
	int irq;
	unsigned long port;

	unsigned char midi_input_mode[2];
	unsigned char midi_output_mode[4];

	struct snd_rawmidi *rmidi;
	struct snd_rawmidi_substream *midi_input[2];
	struct snd_rawmidi_substream *midi_output[4];
};

/* Definitions for portman driver */
#define	BYTE           unsigned char
#define WORD           unsigned int

/* Standard PC parallel port status register equates. */
#define	PP_STAT_BSY   	0x80	/* Busy status.  Inverted. */
#define	PP_STAT_ACK   	0x40	/* Acknowledge.  Non-Inverted. */
#define	PP_STAT_POUT  	0x20	/* Paper Out.    Non-Inverted. */
#define	PP_STAT_SEL   	0x10	/* Select.       Non-Inverted. */
#define	PP_STAT_ERR   	0x08	/* Error.        Non-Inverted. */

/* Standard PC parallel port command register equates. */
#define	PP_CMD_IEN  	0x10	/* IRQ Enable.   Non-Inverted. */
#define	PP_CMD_SELI 	0x08	/* Select Input. Inverted. */
#define	PP_CMD_INIT 	0x04	/* Init Printer. Non-Inverted. */
#define	PP_CMD_FEED 	0x02	/* Auto Feed.    Inverted. */
#define	PP_CMD_STB      0x01	/* Strobe.       Inverted. */

/* Parallel Port Command Register as implemented by PCP2x4. */
#define	INT_EN	 	PP_CMD_IEN	/* Interrupt enable. */
#define	STROBE	        PP_CMD_STB	/* Command strobe. */

/* The parallel port command register field (b1..b3) selects the 
 * various "registers" within the PC/P 2x4.  These are the internal
 * address of these "registers" that must be written to the parallel
 * port command register.
 */
#define	RXDATA0		(0 << 1)	/* PCP RxData channel 0. */
#define	RXDATA1		(1 << 1)	/* PCP RxData channel 1. */
#define	GEN_CTL		(2 << 1)	/* PCP General Control Register. */
#define	SYNC_CTL 	(3 << 1)	/* PCP Sync Control Register. */
#define	TXDATA0		(4 << 1)	/* PCP TxData channel 0. */
#define	TXDATA1		(5 << 1)	/* PCP TxData channel 1. */
#define	TXDATA2		(6 << 1)	/* PCP TxData channel 2. */
#define	TXDATA3		(7 << 1)	/* PCP TxData channel 3. */

/* Parallel Port Status Register as implemented by PCP2x4. */
#define	ESTB		PP_STAT_POUT	/* Echoed strobe. */
#define	INT_REQ         PP_STAT_ACK	/* Input data int request. */
#define	BUSY            PP_STAT_ERR	/* Interface Busy. */

/* Parallel Port Status Register BUSY and SELECT lines are multiplexed
 * between several functions.  Depending on which 2x4 "register" is
 * currently selected (b1..b3), the BUSY and SELECT lines are
 * assigned as follows:
 *
 *   SELECT LINE:                                                    A3 A2 A1
 *                                                                   --------
 */
#define	RXAVAIL		PP_STAT_SEL	/* Rx Available, channel 0.   0 0 0 */
//  RXAVAIL1    PP_STAT_SEL             /* Rx Available, channel 1.   0 0 1 */
#define	SYNC_STAT	PP_STAT_SEL	/* Reserved - Sync Status.    0 1 0 */
//                                      /* Reserved.                  0 1 1 */
#define	TXEMPTY		PP_STAT_SEL	/* Tx Empty, channel 0.       1 0 0 */
//      TXEMPTY1        PP_STAT_SEL     /* Tx Empty, channel 1.       1 0 1 */
//  TXEMPTY2    PP_STAT_SEL             /* Tx Empty, channel 2.       1 1 0 */
//  TXEMPTY3    PP_STAT_SEL             /* Tx Empty, channel 3.       1 1 1 */

/*   BUSY LINE:                                                      A3 A2 A1
 *                                                                   --------
 */
#define	RXDATA		PP_STAT_BSY	/* Rx Input Data, channel 0.  0 0 0 */
//      RXDATA1         PP_STAT_BSY     /* Rx Input Data, channel 1.  0 0 1 */
#define	SYNC_DATA       PP_STAT_BSY	/* Reserved - Sync Data.      0 1 0 */
					/* Reserved.                  0 1 1 */
#define	DATA_ECHO       PP_STAT_BSY	/* Parallel Port Data Echo.   1 0 0 */
#define	A0_ECHO         PP_STAT_BSY	/* Address 0 Echo.            1 0 1 */
#define	A1_ECHO         PP_STAT_BSY	/* Address 1 Echo.            1 1 0 */
#define	A2_ECHO         PP_STAT_BSY	/* Address 2 Echo.            1 1 1 */

#define PORTMAN2X4_MODE_INPUT_OPENED	 0x01
#define PORTMAN2X4_MODE_OUTPUT_OPENED	 0x02
#define PORTMAN2X4_MODE_INPUT_TRIGGERED	 0x04
#define PORTMAN2X4_MODE_OUTPUT_TRIGGERED 0x08


/* Pointer to parallel port which we actually use */
static struct parport *myPort = NULL;

/* Pointer to pardevice after registration */
static struct pardevice *myParDevice = NULL;

/* static struct parport_operations *myParPortOps = NULL; */
static int portowned = 0;
static int portman_found = 0;

/* Delay settings */
static int gwAddressDelay = 0;
static int gwDataDelay = 0;

/* Useful information */
static char *sDeviceName = "portman2x4";
static char *sVersion = "0.1";

/* State+Cleanup variables */
static unsigned char SAVE_PORTCOMMAND = 0;
static unsigned char SAVE_PORTDATA = 0;
static unsigned char SAVE_PORTVALID = 0;

/* parallel port access mappers */

static inline void portman_writeCommand(unsigned char value)
{
	parport_write_control(myPort, value);
}

static inline unsigned char portman_readCommand(void)
{
	return parport_read_control(myPort);
}

static inline unsigned char portman_readStatus(void)
{
	return parport_read_status(myPort);
}

static inline unsigned char portman_readData(void)
{
	return parport_read_data(myPort);
}

static inline void portman_writeData(unsigned char value)
{
	parport_write_data(myPort, value);
}

static void portman_putmidi(int port, unsigned char mididata)
{
	int command = ((port + 4) << 1);
	unsigned long flags;

	/* Get entering data byte and port number in BL and BH respectively.
	 * Set up Tx Channel address field for use with PP Cmd Register.
	 * Store address field in BH register.
	 * Inputs:      AH = Output port number (0..3).
	 *              AL = Data byte.
	 *    command = TXDATA0 | INT_EN;
	 * Align port num with address field (b1...b3),
	 * set address for TXDatax, Strobe=0
	 */
	command |= INT_EN;

	/* Disable interrupts so that the process is not interrupted, then 
	 * write the address associated with the current Tx channel to the 
	 * PP Command Reg.  Do not set the Strobe signal yet.
	 */

      hbpTxWait:
	local_irq_save(flags);
	portman_writeCommand(command);
	udelay(gwAddressDelay);

	/* While the address lines settle, write parallel output data to 
	 * PP Data Reg.  This has no effect until Strobe signal is asserted.
	 */

	portman_writeData(mididata);

	udelay(gwDataDelay);

	/* If PCP channel's TxEmpty is set (TxEmpty is read through the PP
	 * Status Register), then go write data.  Else go back and wait.
	 */

	if ((portman_readStatus() & TXEMPTY) == TXEMPTY) /* Is channel's TxEmpty set? */
		goto hpbEmpty;	                         /* Y: Go write data then. */

	local_irq_restore(flags);                        /* Allow small window for ints. */
	goto hbpTxWait;

	/* TxEmpty is set.  Maintain PC/P destination address and assert
	 * Strobe through the PP Command Reg.  This will Strobe data into
	 * the PC/P transmitter and set the PC/P BUSY signal.
	 */

      hpbEmpty:
	portman_writeCommand(command | STROBE);
	udelay(gwAddressDelay);

	/* Wait for strobe line to settle and echo back through hardware.
	 * Once it has echoed back, assume that the address and data lines
	 * have settled!
	 */

	while ((portman_readStatus() & ESTB) == 0);

	/* Release strobe and immediately re-allow interrupts. */
	portman_writeCommand(command);
	udelay(gwAddressDelay);
	local_irq_restore(flags);

	while ((portman_readStatus() & ESTB) == ESTB);

	/* PC/P BUSY is now set.  We must wait until BUSY resets itself.
	 * We'll reenable ints while we're waiting.
	 */

	while ((portman_readStatus() & BUSY) == BUSY);

	/* Data sent. */
}


static unsigned char portman_readmidi(int port)
{
  /***************************************************************************
   * Attempt to read input byte from specified hardware input port (0..).
   * Return -1 if no data.
   ***************************************************************************/

	unsigned char midiValue = 0;
	unsigned char cmdout;	/* Saved address+IE bit. */

	/* Make sure clocking edge is down before starting... */
	portman_writeData(0);	/* Make sure edge is down. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Set destination address to PCP. */
	cmdout = (port << 1) | INT_EN;	/* Address + IE + No Strobe. */
	portman_writeCommand(cmdout);
	udelay(gwAddressDelay);	/* Address settle. */

	while ((portman_readStatus() & ESTB) == ESTB);	/* Wait for strobe echo. */

	/* After the address lines settle, check multiplexed RxAvail signal.
	 * If data is available, read it.
	 */
	if ((portman_readStatus() & RXAVAIL) == 0)
		return -1;	/* No data. */

	/* Set the Strobe signal to enable the Rx clocking circuitry. */
	portman_writeCommand(cmdout | STROBE);	/* Write address+IE+Strobe. */
	udelay(gwAddressDelay);	/* Strobe settle. */

	while ((portman_readStatus() & ESTB) == 0); /* Wait for strobe echo. */

	/* The first data bit (msb) is already sitting on the input line. */
	midiValue = (portman_readStatus() & 128);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */


	/* Data bit 6. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 1) & 64);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 5. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 2) & 32);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 4. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 3) & 16);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 3. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 4) & 8);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 2. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 5) & 4);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 1. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 6) & 2);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */

	/* Data bit 0. */
	portman_writeData(0);	/* Cause falling edge while data settles. */
	udelay(gwDataDelay);	/* Settle clock. */
	midiValue = midiValue | ((portman_readStatus() >> 7) & 1);
	portman_writeData(1);	/* Cause rising edge, which shifts data. */
	udelay(gwDataDelay);	/* Settle clock. */
	portman_writeData(0);	/* Return data clock low. */
	udelay(gwDataDelay);	/* Settle clock. */


	/* De-assert Strobe and return data. */
	portman_writeCommand(cmdout);	/* Output saved address+IE. */
	udelay(gwAddressDelay);	/* Settle clock. */

	/* Wait for strobe echo. */
	while ((portman_readStatus() & ESTB) == ESTB);

	return (midiValue & 255);	/* Shift back and return value. */
}

/*
 *  Checks if any input data on the given channel is available
 *  Checks RxAvail 
 */

static int portman_dataavail(int channel)
{
	int command = INT_EN;
	switch (channel) {
	case 0:
		command = command | RXDATA0;
		break;
	case 1:
		command = command | RXDATA1;
		break;
	}
	/* Write hardware (assumme STROBE=0) */
	portman_writeCommand(command);
	/* Allow settling. */
	udelay(gwAddressDelay);
	/* Check multiplexed RxAvail signal */
	if ((portman_readStatus() & RXAVAIL) == RXAVAIL)
		return 1;	/* Data available */

	/* No Data available */
	return 0;
}


/*
 *  Flushes any input
 */

static void portman_flushInput(unsigned char port)
{
	/* Local variable for counting things */
	unsigned int iCounter = 0;
	unsigned char command = 0;

	switch (port) {
	case 0:
		command = RXDATA0;
		break;
	case 1:
		command = RXDATA1;
		break;
	default:
		snd_printk("portman_flushInput() Won't flush port %i\n",
			   port);
		return;
	}

	/* Set address for specified channel in port and allow to settle. */
	portman_writeCommand(command);
	udelay(gwAddressDelay);

	/* Assert the Strobe and wait for echo back. */
	portman_writeCommand(command | STROBE);

	/* Wait for ESTB */
	while ((portman_readStatus() & ESTB) == 0)
	  udelay(gwAddressDelay);



	/* Output clock cycles to the Rx circuitry. */
	portman_writeData(0);
	udelay(gwDataDelay);

	/* Flush 250 bits... */
	for (iCounter = 0; iCounter < 250; iCounter++) {
		udelay(gwDataDelay);
		portman_writeData(1);
		udelay(gwDataDelay);
		portman_writeData(0);
	}

	/* Deassert the Strobe signal of the port and wait for it to settle. */
	portman_writeCommand(command | INT_EN);

	/* Wait for settling */
	while ((portman_readStatus() & ESTB) == ESTB)
	  udelay(gwAddressDelay);
}


/* clean up code */

static void cleanup(void)
{
	/* local data */
	unsigned long flags;

	/* ====================
	 * CLEANUP PARALLELPORT
	 * ====================
	 */

	/* check if port was owned by this driver */
	if (portowned == 1) {
		/* restore port state */

		/* Disable interrupt while giving up port */
		local_irq_save(flags);

		/* Restore parallel port status */
		if (SAVE_PORTVALID == 1) {

			portman_writeCommand(SAVE_PORTCOMMAND);
			portman_writeData(SAVE_PORTDATA);
		}

		parport_release(myParDevice);
		portowned = 0;

		/* Reenable interrupts */
		local_irq_restore(flags);
	}

	if (myParDevice != NULL) {
		parport_unregister_device(myParDevice);
		myParDevice = NULL;
	}

	/* ===========================================
	 * CLEANUP KERNEL MEMORY & DEVICE REGISTRATION
	 * ===========================================
	 */

	kfree(portman);
	portman = NULL;
}

/* irq handler */


/*
 * IRQ-HANDLER
 *
 * @param  int              Number of IRQ
 * @param  void*            User data (struct parport* related to this interrupt)
 * @param  struct pt_regs*  Pointer to pt_regs
 */

static void portman_handler_irq(int irq, void *userdata, struct pt_regs *regs)
{
	unsigned long flags;
	/* Test if output gets written if interrupts are disabled! */
	unsigned char midivalue = 0;

	/* Disable interrupts (we must not be disturbed while processing here...) */
	local_irq_save(flags);

	/* While any input data is waiting */
	while ((portman_readStatus() & INT_REQ) == INT_REQ) {
		/* If data available on channel 0, read it and stuff it into the queue. */
		if (portman_dataavail(0)) {
			/* Read Midi */
			midivalue = portman_readmidi(0);
			/* put midi into queue... */
			if (portman->
			    midi_input_mode[0] &
			    PORTMAN2X4_MODE_INPUT_TRIGGERED)
				snd_rawmidi_receive(portman->midi_input[0],
						    &midivalue, 1);

		}
		/* If data available on channel 1, read it and stuff it into the queue. */
		if (portman_dataavail(1)) {
			/* Read Midi */
			midivalue = portman_readmidi(1);
			/* put midi into queue... */
			if (portman->
			    midi_input_mode[1] &
			    PORTMAN2X4_MODE_INPUT_TRIGGERED)
				snd_rawmidi_receive(portman->midi_input[1],
						    &midivalue, 1);
		}

	}

	/* Reenable interrupts */
	local_irq_restore(flags);
}

/* portman detection function */

/****************************************************************************
 *  Initialize the MIDI driver software and port hardware.
 *
 *  Inputs:     BASE = Base I/O address of port.
 *              IRQ  = IRQ number.
 *
 *  Returns:    0  if successful.
 *	       1     Strobe echo failure.
 *	       2     Transmitter stuck.
 ****************************************************************************/

static int hwOpen(void)
{
	int delay = gwAddressDelay;

	/* Initialize the parallel port data register.  Will set Rx clocks
	 * low in case we happen to be addressing the Rx ports at this time.
	 */
	/* 1 */
	portman_writeData(0);
	udelay(delay);

	/* Initialize the parallel port command register, thus initializing
	 * hardware handshake lines to midi box:
	 *
	 *                                  Strobe = 0
	 *                                  Interrupt Enable = 0            
	 */
	/* 2 */
	portman_writeCommand(0);
	udelay(delay);

	/* Check if Portman PC/P 2x4 is out there. */
	/* 3 */
	portman_writeCommand(RXDATA0);	/* Write Strobe=0 to command reg. */
	udelay(delay);

	/* Check for ESTB to be clear */
	/* 4 */
	if ((portman_readStatus() & ESTB) == ESTB)
		return 1;	/* CODE 1 - Strobe Failure. */

	/* Set for RXDATA0 where no damage will be done. */
	/* 5 */
	portman_writeCommand(RXDATA0 + STROBE);	/* Write Strobe=1 to command reg. */
	udelay(delay);

	/* 6 */
	if ((portman_readStatus() & ESTB) != ESTB)
		return 1;	/* CODE 1 - Strobe Failure. */

	/* 7 */
	portman_writeCommand(0);	/* Reset Strobe=0. */
	udelay(delay);


	/* Check if Tx circuitry is functioning properly.  If initialized 
	 * unit TxEmpty is false, send out char and see if if goes true.
	 */
	/* 8 */
	portman_writeCommand(TXDATA0);	/* Tx channel 0, strobe off. */
	udelay(delay);

	/* If PCP channel's TxEmpty is set (TxEmpty is read through the PP
	 * Status Register), then go write data.  Else go back and wait.
	 */
	/* 9 */
	if ((portman_readStatus() & TXEMPTY) == 0)
		return 2;

	/* Return OK status. */
	return 0;
}

/* test */
static void portman_testInterrupts(void)
{
	int i;

	for (i = 0; i < 5; i++) {
		if ((portman_readCommand() & INT_EN) == INT_EN) {
			snd_printk(">>> Interrupts enabled :)\n");
			return;
		}

		snd_printk("portman2x4: %i\n", portman_readCommand());
		snd_printk("portman2x4: %i\n", INT_EN);
		snd_printk(">>> Interrupts are not enabled!");

		udelay(gwAddressDelay);

		snd_printk(">>> Trying to enable IRQ...\n");
		myPort->ops->enable_irq(myPort);
		udelay(gwAddressDelay);
	}
}


/* parport handler functions */

static void portman_attach(struct parport *port)
{
	unsigned long flags;
	/* local data */
	int result = 0;

	/* output information on used port */
	if (port != NULL)
		snd_printk("Using port at 0x%lx, IRQ %i.\n", port->base,
			   port->irq);
	else {
		snd_printk("Ooops! port is NULL\n");
		return;
	}

	/* Test if device supports irq. Abort initialization of not. */
	if (port->irq == -1) {
		snd_printk
		    ("Error. Parallel port does not support IRQ. \n");
		cleanup();
		return;
	}

	/* keep pointer to port */
	myPort = port;

	/* Now register device with IRQ enabled */
	myParDevice = (struct pardevice *) parport_register_device(port,	/* ptr to port to register at */
								   sDeviceName,	/* ptr to device name */
								   NULL,	/* ptr to preemption handler */
								   NULL,	/* ptr to wakeup handler */
								   portman_handler_irq,	/* ptr to irq handler */
								   0,	/* no flags */
								   NULL);	/* no handle */
	/* Check if device registration was successful */
	if (myParDevice == NULL) {
		snd_printk("Error. Pardevice could not be registered.\n");
		cleanup();
		return;
	}

	/* Claim parport device */

	/* Disable interrupts (we must not be disturbed while processing here...) */
	local_irq_save(flags);

	/* Claim the device */
	if (parport_claim(myParDevice) != 0) {
		local_irq_restore(flags);
		snd_printk("Device is busy.\n");
		cleanup();
		return;
	}

	portowned = 1;

	/* Save current port status... */
	SAVE_PORTCOMMAND = portman_readCommand();
	SAVE_PORTDATA = portman_readData();
	SAVE_PORTVALID = 1;

	/* check for portman existence */
	result = 0;
	result = hwOpen();

	/* Reenable interrupts */
	local_irq_restore(flags);

	switch (result) {
	case 0:
		snd_printk("Portman found.\n");
		portman_found = 1;
		break;
	case 1:
		snd_printk("Probe error. Portman not found.\n");
		cleanup();
		return;
		break;
	case 2:
		snd_printk("TX Error. Hardware test fail.\n");
		cleanup();
		return;
		break;
	}

	/* Module initialization complete... */

	/* Later: flushAllInputs()  (number of ports variable - possible compatibility with PortMan 4x4 (untested)) */
	/* save flags, disable interrupts */
	/* Flush inputs 0 and 1 */
	portman_flushInput(0);
	portman_flushInput(1);
	/* restore flags */

	/* Test if interrupts are enabled. */

	portman_testInterrupts();

	return;
}

static void portman_detach(struct parport *port)
{
}

static struct parport_driver portman_driver = {
	.name = "portman2x4",
	.attach = portman_attach,
	.detach = portman_detach,
};

static int snd_portman_midi_input_open(struct snd_rawmidi_substream *substream)
{
	unsigned long flags;

	spin_lock_irqsave(&portman->reg_lock, flags);
	portman->midi_input_mode[substream->number] |=
	    PORTMAN2X4_MODE_INPUT_OPENED;
	portman->midi_input[substream->number] = substream;
	spin_unlock_irqrestore(&portman->reg_lock, flags);
	return 0;
}

static int snd_portman_midi_input_close(struct snd_rawmidi_substream *
					substream)
{
	unsigned long flags;

	spin_lock_irqsave(&portman->reg_lock, flags);
	portman->midi_input_mode[substream->number] &=
	    (~PORTMAN2X4_MODE_INPUT_OPENED);
	portman->midi_input[substream->number] = NULL;
	spin_unlock_irqrestore(&portman->reg_lock, flags);
	return 0;
}

static int snd_portman_midi_output_open(struct snd_rawmidi_substream *
					substream)
{
	unsigned long flags;

	spin_lock_irqsave(&portman->reg_lock, flags);
	portman->midi_output_mode[substream->number] |=
	    PORTMAN2X4_MODE_OUTPUT_OPENED;
	portman->midi_output[substream->number] = substream;
	spin_unlock_irqrestore(&portman->reg_lock, flags);
	return 0;
}

static int snd_portman_midi_output_close(struct snd_rawmidi_substream *
					 substream)
{
	unsigned long flags;

	spin_lock_irqsave(&portman->reg_lock, flags);
	portman->midi_output_mode[substream->number] &=
	    ~PORTMAN2X4_MODE_OUTPUT_OPENED;
	portman->midi_output[substream->number] = NULL;
	spin_unlock_irqrestore(&portman->reg_lock, flags);
	return 0;
}

static void snd_portman_midi_input_trigger(struct snd_rawmidi_substream *
					   substream, int up)
{
	unsigned long flags;

	spin_lock_irqsave(&portman->reg_lock, flags);
	if (up) {
		portman->midi_input_mode[substream->number] |=
		    PORTMAN2X4_MODE_INPUT_TRIGGERED;
	} else {
		portman->midi_input_mode[substream->number] &=
		    ~PORTMAN2X4_MODE_INPUT_TRIGGERED;
	}
	spin_unlock_irqrestore(&portman->reg_lock, flags);
}

static void snd_portman_midi_output_trigger(struct snd_rawmidi_substream *
					    substream, int up)
{
	unsigned long flags;
	unsigned char byte = 0;

	spin_lock_irqsave(&portman->reg_lock, flags);
	if (up) {
		while ((snd_rawmidi_transmit(substream, &byte, 1) == 1))
		  portman_putmidi(substream->number, byte);
	}
	spin_unlock_irqrestore(&portman->reg_lock, flags);
}

static struct snd_rawmidi_ops snd_portman_midi_output = {
	.open =		snd_portman_midi_output_open,
	.close =	snd_portman_midi_output_close,
	.trigger =	snd_portman_midi_output_trigger,
};

static struct snd_rawmidi_ops snd_portman_midi_input = {
	.open =		snd_portman_midi_input_open,
	.close =	snd_portman_midi_input_close,
	.trigger =	snd_portman_midi_input_trigger,
};

static int __init snd_portman_midi(portman_t * portman, int device,
				   struct snd_rawmidi ** rrawmidi)
{
	struct snd_rawmidi *rmidi;
	int err;

	if (rrawmidi)
		*rrawmidi = NULL;

	if ((err =
	     snd_rawmidi_new(portman->card, "Portman2x4", device, 4, 2,
			     &rmidi)) < 0)
		return err;

	strcpy(rmidi->name, "Portman2x4");
	snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_OUTPUT,
			    &snd_portman_midi_output);
	snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_INPUT,
			    &snd_portman_midi_input);
	rmidi->info_flags |=
	    SNDRV_RAWMIDI_INFO_OUTPUT | SNDRV_RAWMIDI_INFO_INPUT |
	    SNDRV_RAWMIDI_INFO_DUPLEX;
	rmidi->private_data = portman;
	portman->rmidi = rmidi;
	if (rrawmidi)
		*rrawmidi = rmidi;
	return 0;
}


/* init midiman portman */

static int __init alsa_card_portman2x4_init(void)
{
	/* LOCAL VARIABLES */
	static int dev = 0;
	struct snd_card *card;
	int err;
	int result = 0;

	/* Display copyright notice and driver version */
	snd_printk("Driverversion is: %s\n", sVersion);

	portman = kzalloc(sizeof(portman_t), GFP_KERNEL);
	if (portman == NULL) {
		snd_printk
		    ("Error allocating memory for portman. Exiting.\n");
		return 1;
	}

	spin_lock_init(&portman->reg_lock);

	portman->midi_input_mode[0] = 0;
	portman->midi_input_mode[1] = 0;
	portman->midi_output_mode[0] = 0;
	portman->midi_output_mode[1] = 0;
	portman->midi_output_mode[2] = 0;
	portman->midi_output_mode[3] = 0;

	/* Initialize parport */
	/* Register a new high-level driver. */
	result = parport_register_driver(&portman_driver);

	/* Test if there is a portman available. Exit if not. */
	if (portman_found == 0) {
		snd_printk("Portman not found. Exiting.\n");
		parport_unregister_driver(&portman_driver);
		return 1;
	}

	for (; dev < SNDRV_CARDS; dev++) {
		if (!enable[dev]) {
			dev++;
			snd_printk("Could not enable card. Exiting.\n");
			cleanup();
			return -ENOENT;
		}
		break;
	}

	if (dev >= SNDRV_CARDS) {
		snd_printk("Could not enable card. Exiting.\n");
		cleanup();
		return -ENODEV;
	}

	/* alsa: create new sound card */
	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL) {
		snd_printk
		    ("Fatal. Cannot allocate memory for sound card. Exiting.\n");
		cleanup();
		return -ENOMEM;
	}

	portman->card = card;

	/* register midi functions */
	if ((err = snd_portman_midi(portman, 0, NULL)) < 0) {
		snd_card_free(card);
		return err;
	}
	strcpy(card->driver, "Portman");
	strcpy(card->shortname, "Portman2x4");
	sprintf(card->longname, "%s %s at 0x%lx, irq %i",
		card->shortname, "2x4", portman->port, portman->irq);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}

	dev++;
	return 0;
}

static void __exit alsa_card_portman2x4_exit(void)
{
	if (portman == NULL)
		return;
	if (portman->card)
		snd_card_free(portman->card);
	cleanup();
	parport_unregister_driver(&portman_driver);
}

module_init(alsa_card_portman2x4_init)
module_exit(alsa_card_portman2x4_exit)
