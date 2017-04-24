// -*- C++ -*- generated by wxGlade 0.6.3 on Mon Mar 16 15:10:36 2009
/*
  Copyright 2009 Ronald S. Burkey <info@sandroid.org>
  
  This file is part of yaAGC. 

  yaAGC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  yaAGC is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with yaAGC; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Filename:	yaACA3.c
  Purpose:	Apollo LM rotational hand-controller emulation module for 
  		yaAGC.
  Reference:	http://www.ibiblio.org/apollo/index.html
  Mode:		2009-03-28 RSB	Adapted from yaACA and yaACA2.
  		.......... OH	... something ...
		2009-04-25 RSB	Fixed incorrect treatment of joysticks having
				> 3 axes.
  
  This is like yaACA (rather than yaACA2) in that it has a text interface
  (rather than a wxWidgets GUI interface).  It differs from yaACA and yaACA2
  in that it uses SDL to access the joystick rather than wxWidgets and/or
  Allegro.  It incorporates the ideas from yaACA2 that minimize the amount
  of configuration --- namely, for each logical axis it required only a 
  physical axis and the +/- send of the axis.
*/

#define VER(x) #x

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "SDL.h"

#include "../yaAGC/yaAGC.h"
#include "../yaAGC/agc_engine.h"

// Some forward-references.
static void PrintJoy (void);
static int PulseACA (void);
static void UpdateJoystick (void);
static void ServiceJoystick_sdl (void);


////////////////////////////////////////////////////////////////////
// Constants.

// Maximum number of physical joystick axes supported.
#define NUM_AXES 10

// Time between joystick updates, in milliseconds.  Note that the AGC will process
// hand-controller inputs at 100 ms. intervals, so the update interval should be 
// less than 100 ms.; also, the update interval should not be a simple fraction
// (like 50 ms.) to avoid aliasing effects.
#ifdef __APPLE__
#define UPDATE_INTERVAL 91
#else
#define UPDATE_INTERVAL 7
#endif
#define PULSE_INTERVAL UPDATE_INTERVAL


/////////////////////////////////////////////////////////////////////
// Data types.

// Info related to data from the joystick driver, which I'll refer
// to as the "physical" joystick.  For SDL, Maximum, Minimum,
// Midpoint, Deadzone, and Divisor are always the same, but I'll
// keep them variable so as to potentially be able to change to
// other toolsets later if necessary.  After the problems I 
// experienced in yaACA2, I'll never try to mix toolkits again 
// within the same program unless forced to.
typedef struct {
  int Has;
  int Maximum, Minimum;
  int CurrentRawReading;
  // The following parameters are derived from Maximum and Minimum.
  // The are used to convert the raw reading to an adjusted reading
  // by:
  //	1. Cutting out about a +/- 10% deadzone around the middle
  // 	   position.
  //	2. Converting the remainder to a -57 to +57 range.
  int Midpoint;
  int Deadzone;
  int Divisor;
} pAxis_t;

// Info related to the logical joystick (Pitch, Roll, Yaw).
typedef struct {
  int Axis;
  int PositiveSense;
  int CurrentAdjustedReading;
  int LastRawReading;
} Axis_t;


//////////////////////////////////////////////////////////////////////
// Global variables.

int IoErrorCount = 0;
pAxis_t Axes[NUM_AXES] = { { 0 } };
int Initialization = 0;
Axis_t Pitch, Roll, Yaw = { 0 };

static char DefaultHostname[] = "localhost";
static char *Hostname = DefaultHostname;
static char NonDefaultHostname[129];
#ifdef WIN32
static int StartupDelay = 500;
#else
static int StartupDelay = 0;
#endif
extern int Portnum;
static int ServerSocket = -1, LastServerSocket = -1;
static int ControllerNumber = 0;
static int JoystickConnectionTimeout = 0;
static int CfgExisted = 0;
static int LastRoll = 1000, LastPitch =1000, LastYaw = 1000;
static int First = 1;

// Sets parameters to all defaults.  These parameters are
// for my Logitech 3D Extreme.
void
DefaultParameters (void)
{
    Roll.Axis = 0;
    Roll.PositiveSense = 1;
    Pitch.Axis = 1;
    Pitch.PositiveSense = 1;
#if defined (WIN32) || defined (__FreeBSD__)
    Yaw.Axis = 3;
    Yaw.PositiveSense = 0;
#else
    Yaw.Axis = 2;
    Yaw.PositiveSense = 0;
#endif
}


// Gets parameters from a cfg file if available, from defaults otherwise.
void
GetParameters (int ControllerNumber)
{
  char Filename[33];
  char s[81];
  FILE *File;
  int iValue;
  
  DefaultParameters ();
  
  sprintf (Filename, "yaACA3-%d.cfg", ControllerNumber);
  File = fopen (Filename, "r");
  if (File != NULL)
    {
      CfgExisted = 1;
      while (NULL != fgets (s, sizeof (s), File))
        {
	  if (1 == sscanf (s, "Roll.Axis=%d", &iValue))
	    Roll.Axis = iValue;
	  else if (1 == sscanf (s, "Roll.PositiveSense=%d", &iValue))
	    Roll.PositiveSense = iValue;
	  else if (1 == sscanf (s, "Pitch.Axis=%d", &iValue))
	    Pitch.Axis = iValue;
	  else if (1 == sscanf (s, "Pitch.PositiveSense=%d", &iValue))
	    Pitch.PositiveSense = iValue;
	  else if (1 == sscanf (s, "Yaw.Axis=%d", &iValue))
	    Yaw.Axis = iValue;
	  else if (1 == sscanf (s, "Yaw.PositiveSense=%d", &iValue))
	    Yaw.PositiveSense = iValue;
	}
      fclose (File);
    }
}


// Writes parameters to a cfg file.
void
WriteParameters (int ControllerNumber)
{
  char Filename[33];
  FILE *File;
  sprintf (Filename, "yaACA3-%d.cfg", ControllerNumber);
  File = fopen (Filename, "w");
  if (File != NULL)
    {
      fprintf (File, "Roll.Axis=%d\n", Roll.Axis);
      fprintf (File, "Roll.PositiveSense=%d\n", Roll.PositiveSense);
      fprintf (File, "Pitch.Axis=%d\n", Pitch.Axis);
      fprintf (File, "Pitch.PositiveSense=%d\n", Pitch.PositiveSense);
      fprintf (File, "Yaw.Axis=%d\n", Yaw.Axis);
      fprintf (File, "Yaw.PositiveSense=%d\n", Yaw.PositiveSense);
      fclose (File);
    }
  else
    {
      fprintf (stdout, "Cannot create file %s, Configuration not saved.\n", Filename);
    }
}

int
main (int argc, char *argv[])
{
  int i, Sense;
  long lj;
  char s[513];

  GetParameters (ControllerNumber);
  Roll.CurrentAdjustedReading = 0;
  Roll.LastRawReading = 0;
  Pitch.CurrentAdjustedReading = 0;
  Pitch.LastRawReading = 0;
  Yaw.CurrentAdjustedReading = 0;
  Yaw.LastRawReading = 0;

  fprintf (stdout, "yaACA3 Apollo ACA simulation, ver " VER(NVER) ", built " __DATE__ " " __TIME__ "\n");
  fprintf (stdout, "Copyright 2009 by Ronald S. Burkey\n");
  fprintf (stdout, "Refer to http://www.ibiblio.org/apollo/index.html for more information.\n");
	
  Portnum = 19803;  
  for (i = 1; i < argc; i++)
  {
    
    if (1 == sscanf (argv[i], "--ip=%s", s))
      {
	strcpy (NonDefaultHostname, s);
	Hostname = NonDefaultHostname;
      }
    else if (1 == sscanf (argv[i], "--port=%ld", &lj))
      {
	Portnum = lj;
	if (Portnum <= 0 || Portnum >= 0x10000)
	  {
	    fprintf (stdout, "The --port switch is out of range.  Must be 1-64K.\n");
	    goto Help;
	  }
      }
    else if (1 == sscanf (argv[i], "--delay=%ld", &lj))
      {
	StartupDelay = lj;
      }
    else if (1 == sscanf (argv[i], "--controller=%ld", &lj))
      {
	if (lj < 0 || lj > 1)
	  {
	    fprintf (stdout, "Only --controller=0 and --controller=1 are allowed.\n");
	    goto Help;
	  }
	else
	  ControllerNumber = lj;
      }
    else if (1 == sscanf (argv[i], "--pitch=-%ld", &lj))
      {
         Pitch.PositiveSense = 0;
         Pitch.Axis = lj;
	 CfgExisted = 0;
      }
    else if (1 == sscanf (argv[i], "--pitch=+%ld", &lj) ||
    	     1 == sscanf (argv[i], "--pitch=%ld", &lj))
      {
         Pitch.PositiveSense = 1;
         Pitch.Axis = lj;
	 CfgExisted = 0;
      }
    else if (1 == sscanf (argv[i], "--roll=-%ld", &lj))
      {
         Roll.PositiveSense = 0;
         Roll.Axis = lj;
	 CfgExisted = 0;
      }
    else if (1 == sscanf (argv[i], "--roll=+%ld", &lj) ||
    	     1 == sscanf (argv[i], "--roll=%ld", &lj))
      {
         Roll.PositiveSense = 1;
         Roll.Axis = lj;
	 CfgExisted = 0;
      }
    else if (1 == sscanf (argv[i], "--yaw=-%ld", &lj))
      {
         Yaw.PositiveSense = 0;
         Yaw.Axis = lj;
	 CfgExisted = 0;
      }
    else if (1 == sscanf (argv[i], "--yaw=+%ld", &lj) ||
    	     1 == sscanf (argv[i], "--yaw=%ld", &lj))
      {
         Yaw.PositiveSense = 1;
         Yaw.Axis = lj;
	 CfgExisted = 0;
      }
    else
      {
      Help:
	fprintf (stdout, "USAGE:\n");
	fprintf (stdout, "\tyaACA3 [OPTIONS]\n");
	fprintf (stdout, "The available options are:\n");
	fprintf (stdout, "--ip=Hostname\n");
	fprintf (stdout, "\tThe yaACA2 program and the yaAGC Apollo Guidance Computer simulation\n");
	fprintf (stdout, "\texist in a \"client/server\" relationship, in which the yaACA2 program\n");
	fprintf (stdout, "\tneeds to be aware of the IP address or symbolic name of the host \n");
	fprintf (stdout, "\tcomputer running the yaAGC program.  By default, this is \"localhost\",\n");
	fprintf (stdout, "\tmeaning that both yaACA2 and yaAGC are running on the same computer.\n");
	fprintf (stdout, "--port=Portnumber\n");
	fprintf (stdout, "\tBy default, yaACA2 attempts to connect to the yaAGC program using port\n");
	fprintf (stdout, "\tnumber %d.  However, if more than one instance of yaACA2 is being\n",
		Portnum);
	fprintf (stdout, "\trun, or if yaAGC has been configured to listen on different ports, then\n");
	fprintf (stdout, "\tdifferent port settings for yaACA2 are needed.  Note that by default,\n");
	fprintf (stdout, "\tyaAGC listens for new connections on ports %d-%d.\n",
		19697, 19697 + 10 - 1);
	fprintf (stdout, "--delay=N\n");
	fprintf (stdout, "\t\"Start-up delay\", in ms.  Defaults to %d.  What the start-up\n", StartupDelay);
	fprintf (stdout, "\tdelay does is to prevent yaACA2 from attempting to communicate with\n");
	fprintf (stdout, "\tyaAGC for a brief time after power-up.  This option is really only\n");
	fprintf (stdout, "\tuseful in Win32, to work around a problem with race-conditions at\n");
	fprintf (stdout, "\tstart-up.\n");
	fprintf (stdout, "--roll=N\n");
	fprintf (stdout, "--pitch=N\n");
	fprintf (stdout, "--yaw=N\n");
	fprintf (stdout, "\tThese options allow you to relate the axis numbers (0, 1, ...) by\n");
	fprintf (stdout, "\twhich the joystick controller works to physical axes (pitch, roll,\n");
	fprintf (stdout, "\tyaw) by which the spacecraft works.  In almost all cases --roll=0\n");
	fprintf (stdout, "\tand --pitch=1 (the defaultswill be correct, but the --yaw=N setting\n");
	fprintf (stdout, "\tvaries from target platform to target platform, and joystick model\n");
	fprintf (stdout, "\tto joystick model.  Once you use these command-line switches once,\n");
	fprintf (stdout, "\tthe settings are saved to a configuration file and you don't have\n");
	fprintf (stdout, "\tto use them again.  The axis-number N can optionally be preceded by\n");
	fprintf (stdout, "\ta \'-\' to indicate that the sense of the axis is reversed from the\n");
	fprintf (stdout, "\tdefault expectation.  For example, if axis 5 was used for yaw, but\n");
	fprintf (stdout, "\tyou found you were getting yaw left where you expected yaw-right, you\n");
	fprintf (stdout, "\tshould use--yaw=-5 rather than --yaw=5.\n");
	fprintf (stdout, "--controller=N\n");
	fprintf (stdout, "\tIn case there are two joystick controllers attached, this allows\n");
	fprintf (stdout, "\tselection of just one of them.  The default is N=0, but N=1 is also\n");
	fprintf (stdout, "\tallowed.  If there are more than two attached, only the first two can\n");
	fprintf (stdout, "\tbe accessed.\n");
	return (1);
      }	
  }
  if (!CfgExisted)
    WriteParameters (ControllerNumber);

  // Now we start polling the joystick from time to time.  The way Allegro works is to 
  // maintain an array containing info on each joystick.  This array is updated only when
  // poll_joystick is called (which you have to do explicitly).  To see what has changed,
  // we maintain a copy of the joy[] array.
  while (1)
    {
      // Sleep for a while so that this job won't monopolize CPU cycles.
#ifdef WIN32
      Sleep (UPDATE_INTERVAL);	    
#else // WIN32
#include <time.h>
      struct timespec req, rem;
      req.tv_sec = 0;
      req.tv_nsec = 1000000 * UPDATE_INTERVAL;
      nanosleep (&req, &rem);
#endif // WIN32

      ServiceJoystick_sdl ();			// Get joystick physical values.
      if (Initialization >= 2)
        {
	  UpdateJoystick ();			// Translate physical to logical values.
	  PrintJoy ();				// Display them locally.
        }
      PulseACA ();				// Manage server connection.
    }

#ifndef SOLARIS
  return (0);
#endif
}


// This function translates a raw joystick reading to the -57..+57 range expected by AGC.
void
Translate (Axis_t *AxisPtr)
{	  
  int i, Axis;
  int PositiveSense;
  if (Initialization < 2)
    i = 0;
  else
    {
      i = AxisPtr->LastRawReading;
      Axis = AxisPtr->Axis;
      PositiveSense = AxisPtr->PositiveSense;
      // Center around midpoint.
      i -= Axes[Axis].Midpoint;
      // Polarity.
      if (!PositiveSense)
	i = -i;
      // Cut out the deadzone around the midpoint.
      if (i > 0)
	{
	  i -= Axes[Axis].Deadzone;
	  if (i < 0)
	    i = 0;
	}
      else if (i < 0)
	{
	  i += Axes[Axis].Deadzone;
	  if (i > 0)
	    i = 0;
	}
      // Scale.
      i /= Axes[Axis].Divisor;
      // Sanity check.
      if (i > 57)
	i = 57;
      else if (i < -57)
	i = -57;
    }
  // Output!
  AxisPtr->CurrentAdjustedReading = i;
}


static int NumAxes;
// Service the joystick using the sdl toolkit.
void
ServiceJoystick_sdl (void)
{

  int i, j, k;
  static SDL_Joystick *OurJoy = NULL;
  
  if (Initialization < 1)
    {
      if (Initialization == -1) // Permanent error.
        return;
      Initialization = SDL_Init (SDL_INIT_JOYSTICK);
      if (Initialization == 0)
        {
          Initialization = 1;
	  fprintf (stdout, "SDL library initialized.\n");
	}
      else
        {
	  Initialization = -1;
	  fprintf (stdout, "Cannot initialize joystick driver.\n");
	  return;
	}
    }

  // If no joystick has been attached yet, then try to attach one. 
  // We only check for connections about once per second.
  if (Initialization < 2)	// No joystick found so far?
    {
      if (JoystickConnectionTimeout <= 0)
        {
	  JoystickConnectionTimeout = 1000;
	  
          if (SDL_NumJoysticks () > ControllerNumber)
	    {
	      fprintf (stdout, "Checking if can talk to joystick.\n");
	      OurJoy = SDL_JoystickOpen (ControllerNumber);
	      if (OurJoy == NULL)
	        {
		  fprintf (stdout, "... Cannot, trying again later.\n");
	          return;
		}
	      
	      // Get the parameters of the joystick controller.
	      fprintf (stdout, "Joystick controller model:  %s\n", SDL_JoystickName (ControllerNumber));
	      NumAxes = SDL_JoystickNumAxes (OurJoy);
	      if (NumAxes > NUM_AXES)
	        NumAxes = NUM_AXES;
		
	      if (NumAxes < 2)
	        {
		  fprintf (stdout, "Not enough axes on this joystick, retrying later.\n");
		  SDL_JoystickClose (OurJoy);
	          return;
		}
		
	      Initialization = 2;
	    
	      fprintf (stdout, "Joystick axes are:");
	      for (k = 0; k < NumAxes; k++)
		{
		  Axes[k].Has = 1;
		  Axes[k].Minimum = -32768;
		  Axes[k].Maximum = 32767;
		  Axes[k].CurrentRawReading = 0;
		  fprintf (stdout, " #%d", k);
		}
	      fprintf (stdout, "\n");
	      // Compute parameters that derive from Maximum and Minimum.
	      for (i = 0; i < NUM_AXES; i++)
	        {
		  Axes[i].Midpoint = (Axes[i].Minimum + Axes[i].Maximum) / 2;
		  Axes[i].Deadzone = (Axes[i].Maximum - Axes[i].Minimum) / 20;
		  Axes[i].Divisor = ((Axes[i].Maximum - Axes[i].Minimum) * 9 + 10 * 57) / (20 * 57);
		}
	      
	      // Make sure the current position will update ASAP.
	      Roll.LastRawReading = 100000;
	      Pitch.LastRawReading = 100000;
	      Yaw.LastRawReading = 100000;
	      
	    }
	  else
	    {
	      fprintf (stdout, "Not enough joystick controllers attached, trying later.\n");
	      // The following step is a little extreme, but without it we'll never
	      // detect when a joystick controller is attached.  Fortunately, this
	      // program has the sole task (SDL-wise) of reading joysticks, so we
	      // can get away with doing this.
	      SDL_Quit ();
	      Initialization = 0;
	    }
	}
      else
        {
	  JoystickConnectionTimeout -= PULSE_INTERVAL;
	}
    }
    
  // Find the current position.
  if (Initialization == 2)
    {
      // Update the readings in the driver itself.
      SDL_JoystickUpdate ();
    
      // Read all of the axes. 
      Axes[Roll.Axis].CurrentRawReading = SDL_JoystickGetAxis (OurJoy, Roll.Axis);
      Axes[Pitch.Axis].CurrentRawReading = SDL_JoystickGetAxis (OurJoy, Pitch.Axis);
      Axes[Yaw.Axis].CurrentRawReading = SDL_JoystickGetAxis (OurJoy, Yaw.Axis);
      
    }
}


// Update screen and/or output changed joystick data to yaAGC.
void
UpdateJoystick (void)
{
      if (Roll.LastRawReading != Axes[Roll.Axis].CurrentRawReading)
        {	
	  Roll.LastRawReading = Axes[Roll.Axis].CurrentRawReading;
	  Translate (&Roll);
	}
      if (Pitch.LastRawReading != Axes[Pitch.Axis].CurrentRawReading)
        {	
	  Pitch.LastRawReading = Axes[Pitch.Axis].CurrentRawReading;
	  Translate (&Pitch);
	}
      if (NumAxes >= 3 && Yaw.LastRawReading != Axes[Yaw.Axis].CurrentRawReading)
        {	
	  Yaw.LastRawReading = Axes[Yaw.Axis].CurrentRawReading;
	  Translate (&Yaw);
	}
}


//--------------------------------------------------------------------------
// These functions output the new orientation both to local display and to
// the server (yaAGC).  OutputTranslated() is called only by PrintJoy().
// Note that we send the angular data to the CPU whenever it changes, and
// not just when the CPU has enabled it or requested it.  This is done in
// order to give the CPU the ability to retransmit the data to other 
// potential consumers such as the AGS.  It's the CPU's responsibility
// to mask the data from itself until requested.

static void
OutputTranslated (int Channel, int Value)
{
  unsigned char Packet[4];
  int j;
  if (ServerSocket == -1)
    return;
  if (Value < 0)			// Make negative values 1's-complement.
    Value = (077777 & (~(-Value)));
  FormIoPacket (Channel, Value, Packet);
  j = send (ServerSocket, Packet, 4, MSG_NOSIGNAL);
  if (j == SOCKET_ERROR && SOCKET_BROKEN)
    {
      close (ServerSocket);
      ServerSocket = -1;
      fprintf (stdout, "\nServer connection lost.\n");
    }
}

// Print current logical joystick readings in terminal.
static void
PrintJoy (void)
{
  int j, Changed = 0, New31;
  static int Last31 = 0;
  
  // Report to yaAGC.
  New31 = 077777;
  if (Roll.CurrentAdjustedReading < 0)
    New31 &= ~040040;
  else if (Roll.CurrentAdjustedReading > 0)
    New31 &= ~040020;
  if (Yaw.CurrentAdjustedReading < 0)
    New31 &= ~040010;
  else if (Yaw.CurrentAdjustedReading > 0)
    New31 &= ~040004;
  if (Pitch.CurrentAdjustedReading < 0)
    New31 &= ~040002;
  else if (Pitch.CurrentAdjustedReading > 0)
    New31 &= ~040001;
  if (ServerSocket != -1 && (New31 != Last31 || First))
    {
      unsigned char Packet[8];
      Last31 = New31;
      // First, create the mask which will tell the CPU to only pay attention to
      // relevant bits of channel (031).
      FormIoPacket (0431, 040077, Packet);
      FormIoPacket (031, New31, &Packet[4]);
      // And, send it all.
      j = send (ServerSocket, Packet, 8, MSG_NOSIGNAL);
      if (j == SOCKET_ERROR && SOCKET_BROKEN)
        {
	  close (ServerSocket);
	  ServerSocket = -1;
          fprintf (stdout, "\nServer connection lost.\n");
	  return;
	}
    }
  if (Roll.CurrentAdjustedReading != LastRoll || First)
    {
      OutputTranslated (0170, Roll.CurrentAdjustedReading);
      Changed = 1;
    }
  if (Pitch.CurrentAdjustedReading != LastPitch || First)
    {
      OutputTranslated (0166, Pitch.CurrentAdjustedReading);
      Changed = 1;
    }
  if (Yaw.CurrentAdjustedReading != LastYaw || First)
    {
      OutputTranslated (0167, Yaw.CurrentAdjustedReading);
      Changed = 1;
    }
  // Local display.
  if (Changed)
    {
      fprintf (stdout, "\rRoll=%+03d\tPitch=%+03d\tYaw=%+03d", 
      	      LastRoll = Roll.CurrentAdjustedReading, 
	      LastPitch = Pitch.CurrentAdjustedReading, 
	      LastYaw = Yaw.CurrentAdjustedReading);
      fflush (stdout);
      First = 0;
    }
}


//-------------------------------------------------------------------------
// Manage server connection.  I copied this right out of yaACA, and then
// modified to take care of a wacky Windows problem.  As long as PulseACA 
// is called every so often the connection to the server will be managed 
// automatically.

static int
PulseACA (void)
{
  if (StartupDelay > 0)
    {
      StartupDelay -= UPDATE_INTERVAL;
      return (1);
    }
  // Try to connect to the server (yaAGC) if not already connected.
  if (ServerSocket == -1)
    {
      ServerSocket = CallSocket (Hostname, Portnum);
      if (ServerSocket != -1)
        fprintf (stdout, "\nConnection to server established on socket %d\n", ServerSocket);
      else
        StartupDelay = 1000;
    }
      
  if (ServerSocket != -1)
    {
      static unsigned char Packet[4];
      static int PacketSize = 0;
      int i;
      unsigned char c;
      for (;;)
        {
	  i = recv (ServerSocket, &c, 1, MSG_NOSIGNAL);
	  if (i == -1)
	    {
	      // The condition i==-1,errno==0 or 9 occurs only on Win32, and 
	      // I'm not sure exactly what it corresponds to---I assume 
	      // means "no data" rather than error.
	      if (errno == EAGAIN || errno == 0 || errno == 9)
	        i = 0;
	      else
	        {	
		  fprintf (stdout, "\nyaACA3 reports server error %d\n", errno);
		  close (ServerSocket);
		  ServerSocket = -1;
		  break;
	        }
	    }
	  if (i == 0)
	    break;
	  if (0 == (0xc0 & c))
	    PacketSize = 0;
	  if (PacketSize != 0 || (0xc0 & c) == 0)	      
	    { 
	      Packet[PacketSize++] = c;
	      if (PacketSize >= 4)
		{
		  PacketSize = 0;   
		  // *** Do something with the input packet. ***
		  
		}  
	    }
	}
    }

  return (1);    
}



