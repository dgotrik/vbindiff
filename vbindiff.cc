//--------------------------------------------------------------------
// $Id: vbindiff.cc,v 1.3 1995/11/28 22:50:48 Madsen Exp $
//--------------------------------------------------------------------
//
//   Visual Binary DIFF
//   Copyright 1995 by Christopher J. Madsen
//
//   Visual display of differences in binary files
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//--------------------------------------------------------------------

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream.h>
#include <sys/winmgr.h>
#include <sys/kbdscan.h>

//====================================================================
// Type definitions:

typedef unsigned char   Byte;
typedef unsigned short  Word;

typedef Byte  Command;

enum Boolean {False, True};

//====================================================================
// Constants:

const Byte  cBackground = F_WHITE|B_BLUE;
const Byte  cPromptWin  = F_WHITE|B_BLUE;
const Byte  cPromptKey  = F_WHITE|B_BLUE|INTENSITY;
const Byte  cPromptBdr  = F_WHITE|B_BLUE|INTENSITY;
const Byte  cFileName   = F_BLACK|B_WHITE;
const Byte  cFileWin    = F_WHITE|B_BLUE;
const Byte  cFileDiff   = F_RED|B_BLUE|INTENSITY;

const Command  cmmMove        = 0x80;

const Command  cmmMoveSize    = 0x03;
const Command  cmmMoveForward = 0x04;
const Command  cmmMoveTop     = 0x08;
const Command  cmmMoveBottom  = 0x10;

const Command  cmmMoveByte    = 0x00; // Move 1 byte
const Command  cmmMoveLine    = 0x01; // Move 1 line
const Command  cmmMovePage    = 0x02; // Move 1 page
const Command  cmmMoveAll     = 0x03; // Not used

const Command  cmmMoveBoth    = cmmMoveTop|cmmMoveBottom;

const Command  cmNothing      = 0;
const Command  cmNextDiff     = 1;
const Command  cmQuit         = 2;

const int  numLines  = 9;       // Number of lines of each file to display
const int  lineWidth = 16;      // Number of bytes displayed per line
const int  bufSize   = numLines * lineWidth;

const int  maxPath = 260;

// The number of bytes to move for each possible step size:
//   See cmmMoveByte, cmmMoveLine, cmmMovePage
const int  steps[4] = {1, lineWidth, bufSize-lineWidth, 0};

//====================================================================
// Class Declarations:

class Difference;

class FileDisplay
{
  friend Difference;

 protected:
  int                bufContents;
  const Difference*  diffs;
  ifstream           file;
  char               fileName[maxPath];
  streampos          offset;
  wm_handle          win;
  int                yPos;
  union {
    Byte             line[numLines][lineWidth];
    Byte             buffer[bufSize];
  };
 public:
  FileDisplay();
  ~FileDisplay();
  void         init(int y, const Difference* aDiff=NULL,
                    const char* aFileName=NULL);
  void         shutDown();
  void         display()   const;
  const Byte*  getBuffer() const { return buffer; };
  void         move(int step);
  void         setFile(const char* aFileName);
}; // end FileDisplay

class Difference
{
  friend FileDisplay::display() const;

 protected:
  const FileDisplay*  file1;
  const FileDisplay*  file2;
  int                 numDiffs;
  union {
    Byte              line[numLines][lineWidth];
    Byte              table[bufSize];
  };
 public:
  Difference(const FileDisplay* aFile1, const FileDisplay* aFile2);
  int  compute();
  int  getNumDiffs() const { return numDiffs; };
}; // end Difference

//====================================================================
// Inline Functions:

template <class T> inline const T& min(const T& t1, const T& t2)
{
  if  (t1 < t2)
    return t1;
  else
    return t2;
}

template <class T> inline const T& max(const T& t1, const T& t2)
{
  if  (t1 > t2)
    return t1;
  else
    return t2;
}

//====================================================================
// Global Variables:

wm_handle    bgWin, promptWin;
FileDisplay  file1, file2;
Difference   diffs(&file1, &file2);

//====================================================================
// Class Difference:
//
// Member Variables:
//   file1, file2:
//     The FileDisplay objects being compared
//   numDiffs:
//     The number of differences between the two FileDisplay buffers
//   line/table:
//     An array of Booleans for each byte in the FileDisplay buffers
//     True marks differences
//
//--------------------------------------------------------------------
// Constructor:
//
// Input:
//   aFile1, aFile2:
//     Pointers to the FileDisplay objects to compare

Difference::Difference(const FileDisplay* aFile1, const FileDisplay* aFile2)
: file1(aFile1),
  file2(aFile2)
{
} // end Difference::Difference

//--------------------------------------------------------------------
// Compute differences:
//
// Input Variables:
//   file1, file2:  The files to compare
//
// Returns:
//   The number of differences between the buffers
//   -1 if both buffers are empty
//
// Output Variables:
//   numDiffs:  The number of differences between the buffers

int Difference::compute()
{
  memset(table, 0, sizeof(table)); // Clear the difference table

  int  different = 0;

  const Byte*  buf1 = file1->buffer;
  const Byte*  buf2 = file2->buffer;

  int  size = min(file1->bufContents, file2->bufContents);

  for (int i = 0; i < size; i++)
    if (*(buf1++) != *(buf2++)) {
      table[i] = True;
      ++different;
    }

  size = max(file1->bufContents, file2->bufContents);

  if (i < size) {
    // One buffer has more data than the other:
    different += size - i;
    for (; i < size; i++)
      table[i] = True;          // These bytes are only in 1 buffer
  } else if (!size)
    return -1;                  // Both buffers are empty

  numDiffs = different;

  return different;
} // end Difference::compute

//====================================================================
// Class FileDisplay:
//
// Member Variables:
//   bufContents:
//     The number of bytes in the file buffer
//   diffs:
//     A pointer to the Difference object related to this file
//   file:
//     The file being displayed
//   fileName:
//     The relative pathname of the file being displayed
//   offset:
//     The position in the file of the first byte in the buffer
//   win:
//     The handle of the window used for display
//   yPos:
//     The vertical position of the display window
//   buffer/line:
//     The currently displayed portion of the file
//
//--------------------------------------------------------------------
// Constructor:

FileDisplay::FileDisplay()
: bufContents(0),
  diffs(NULL),
  offset(0),
  win(NULL),
  yPos(0)
{
  fileName[0] = '\0';
} // end FileDisplay::FileDisplay

//--------------------------------------------------------------------
// Initialize:
//
// Creates the display window and opens the file.
//
// Input:
//   y:          The vertical position of the display window
//   aDiff:      The Difference object related to this buffer
//   aFileName:  The name of the file to display

void FileDisplay::init(int y, const Difference* aDiff,
                       const char* aFileName)
{
  diffs = aDiff;
  yPos  = y;

  win = wm_create (0,y+1, 79,y+numLines, // Window corners
                         0,              // No border
                         cFileWin,       // Border attributes
                         cFileWin);      // Default attributes
  wm_update(win, 0);
  wm_open(win);

  if (aFileName)
    setFile(aFileName);
} // end FileDisplay::init

//--------------------------------------------------------------------
// Destructor:

FileDisplay::~FileDisplay()
{
  shutDown();
  file.close();
} // end FileDisplay::~FileDisplay

//--------------------------------------------------------------------
// Shut down the file display:
//
// Deletes the display window.

void FileDisplay::shutDown()
{
  if (win) {
    wm_close(win);
    wm_delete(win);
    win = NULL;
  }
} // end FileDisplay::shutDown

//--------------------------------------------------------------------
// Display the file contents:

void FileDisplay::display() const
{
  const int  leftMar  = 11;     // Starting column of hex display
  const int  leftMar2 = 61;     // Starting column of ASCII display

  streampos  lineOffset = offset;

  wm_clear(win);

  int i,j,index,lineLength;
  char  buf[lineWidth + lineWidth/8 + 1];

  memset(buf, ' ', sizeof(buf));

  for (i = 0; i < numLines; i++) {
    wm_gotoxy(win, 0,i);
    wm_printf(win, "%04X %04X:",Word(lineOffset>>16),Word(lineOffset&0xFFFF));

    lineLength  = min(lineWidth, bufContents - i*lineWidth);

    for (j = index = 0; j < lineLength; j++) {
      if (j % 8 == 0) {
        wm_putc(win, ' ');
        ++index;
      }
      wm_printf(win, "%02X ", line[i][j]);
      buf[index++] = (line[i][j] >= ' ') ? line[i][j] : '.';
    }
    buf[index] = '\0';

    wm_puts_at(win, leftMar2-1,i, buf);

    if (diffs)
      for (j = 0; j < lineWidth; j++)
        if (diffs->line[i][j]) {
          wm_puta_at(win, j*3 + leftMar  + (j>7),i, cFileDiff,2);
          wm_puta_at(win, j   + leftMar2 + (j>7),i, cFileDiff,1);
        }
    lineOffset += lineWidth;
  }
  wm_update(win, 0);
} // end FileDisplay::display

//--------------------------------------------------------------------
// Change the file position:
//
// Changes the file offset and updates the buffer.
// Does not update the display.
//
// Input:
//   step:
//     The number of bytes to move
//     A negative value means to move backward

void FileDisplay::move(int step)
{
  offset += step;

  if (offset < 0)
    offset = 0;

  if (file.fail())
    file.clear();

  file.seekg(offset);
  file.read(buffer, bufSize);
  bufContents = file.gcount();
} // end FileDisplay::move

//--------------------------------------------------------------------
// Open a file for display:
//
// Opens the file, updates the filename display, and reads the start
// of the file into the buffer.
//
// Input:
//   aFileName:  The name of the file to open

void FileDisplay::setFile(const char* aFileName)
{
  strncpy(fileName, aFileName, maxPath);
  fileName[maxPath-1] = '\0';

  wm_puts_at(bgWin, 0,yPos, fileName);
  wm_puta_at(bgWin, 0,yPos, cFileName, 80);

  file.close();
  file.open(fileName, ios::in|ios::bin|ios::nocreate);

  offset = 0;
  file.read(buffer, bufSize);
  bufContents = file.gcount();
} // end FileDisplay::setFile

//====================================================================
// Main Program:
//--------------------------------------------------------------------
// Create & display prompt window:
//
// Returns:
//   True:   Prompt window created & displayed
//   False:  Unable to display prompt

Boolean initPrompt()
{
  promptWin = wm_create (1,22, 78,23, // Window corners
                         1,           // Single line border
                         cPromptBdr,  // Border attributes
                         cPromptWin); // Default attributes
  if (promptWin == NULL)
    return False;

  wm_puts(promptWin, "\x1A forward 1 char   \x19 forward 1 line"
          "   RET next difference  ALT  freeze top\n"
          "\x1B backward 1 char  \x18 backward 1 line"
          "  ESC quit             CTRL freeze bottom");
  wm_puta_at(promptWin,  0,0, cPromptKey, 1);
  wm_puta_at(promptWin, 19,0, cPromptKey, 1);
  wm_puta_at(promptWin, 38,0, cPromptKey, 3);
  wm_puta_at(promptWin, 59,0, cPromptKey, 4);
  wm_puta_at(promptWin,  0,1, cPromptKey, 1);
  wm_puta_at(promptWin, 19,1, cPromptKey, 1);
  wm_puta_at(promptWin, 38,1, cPromptKey, 3);
  wm_puta_at(promptWin, 59,1, cPromptKey, 4);
  wm_open (promptWin);          /* Open the window    */

  return True;
} // end initPrompt

//--------------------------------------------------------------------
// Initialize program:
//
// Returns:
//   True:   Initialization complete
//   False:  Error

Boolean initialize()
{
  if (!wm_init(7))
    return False;

  // Create & display background window:
  bgWin = wm_create (0,0, 79,24,   // Window corners
                     0,            // No border
                     cBackground,  // Border attributes
                     cBackground); // Default attributes
  if (bgWin == NULL)
    return False;
  wm_open(bgWin);

  if (!initPrompt())
    return False;

  file1.init(0,&diffs);
  file2.init(11,&diffs);

  wm_top(promptWin);

  return True;
} // end initialize

//--------------------------------------------------------------------
// Get a command from the keyboard:
//
// Returns:
//   Command code

Command getCommand()
{
  Command  cmd = cmNothing;

  while (_read_kbd(0,0,0) != -1) // Clear keyboard buffer
    ;

  int  key = _read_kbd(0,1,0);

  key = toupper(key);

  switch (key) {
   case 0:                      // Extended code
    key = _read_kbd(0,1,0);
    switch (key) {
     case K_DOWN:
      cmd = cmmMove|cmmMoveBoth|cmmMoveLine|cmmMoveForward;
      break;
     case K_RIGHT:
      cmd = cmmMove|cmmMoveBoth|cmmMoveByte|cmmMoveForward;
      break;
     case K_PAGEDOWN:
      cmd = cmmMove|cmmMoveBoth|cmmMovePage|cmmMoveForward;
      break;
     case K_LEFT:
      cmd = cmmMove|cmmMoveBoth|cmmMoveByte;
      break;
     case K_UP:
      cmd = cmmMove|cmmMoveBoth|cmmMoveLine;
      break;
     case K_PAGEUP:
      cmd = cmmMove|cmmMoveBoth|cmmMovePage;
      break;

     case K_ALT_DOWN:
      cmd = cmmMove|cmmMoveBottom|cmmMoveLine|cmmMoveForward;
      break;
     case K_ALT_RIGHT:
      cmd = cmmMove|cmmMoveBottom|cmmMoveByte|cmmMoveForward;
      break;
     case K_ALT_PAGEDOWN:
      cmd = cmmMove|cmmMoveBottom|cmmMovePage|cmmMoveForward;
      break;
     case K_ALT_LEFT:
      cmd = cmmMove|cmmMoveBottom|cmmMoveByte;
      break;
     case K_ALT_UP:
      cmd = cmmMove|cmmMoveBottom|cmmMoveLine;
      break;
     case K_ALT_PAGEUP:
      cmd = cmmMove|cmmMoveBottom|cmmMovePage;
      break;

     case K_CTRL_DOWN:
      cmd = cmmMove|cmmMoveTop|cmmMoveLine|cmmMoveForward;
      break;
     case K_CTRL_RIGHT:
      cmd = cmmMove|cmmMoveTop|cmmMoveByte|cmmMoveForward;
      break;
     case K_CTRL_PAGEDOWN:
      cmd = cmmMove|cmmMoveTop|cmmMovePage|cmmMoveForward;
      break;
     case K_CTRL_LEFT:
      cmd = cmmMove|cmmMoveTop|cmmMoveByte;
      break;
     case K_CTRL_UP:
      cmd = cmmMove|cmmMoveTop|cmmMoveLine;
      break;
     case K_CTRL_PAGEUP:
      cmd = cmmMove|cmmMoveTop|cmmMovePage;
      break;
    } // end inner switch
    break;

   case 0x0D:                   // Enter
    cmd = cmNextDiff;
    break;

   case 0x1B:                   // Esc
   case 0x03:                   // Ctrl+C
   case 'Q':
    cmd = cmQuit;
    break;
  } // end switch key

  return cmd;
} // end getCommand

//--------------------------------------------------------------------
// Handle a command:
//
// Input:
//   cmd:  The command to be handled

void handleCmd(Command cmd)
{
  if (cmd & cmmMove) {
    int  step = steps[cmd & cmmMoveSize];

    if ((cmd & cmmMoveForward) == 0)
      step *= -1;               // We're moving backward

    if (cmd & cmmMoveTop)
      file1.move(step);

    if (cmd & cmmMoveBottom)
      file2.move(step);
  } // end if move
  else if (cmd == cmNextDiff) {
    do {
      file1.move(bufSize);
      file2.move(bufSize);
    } while (!diffs.compute());
  } // end else if cmNextDiff

  while (diffs.compute() < 0) {
    file1.move(-steps[cmmMovePage]);
    file2.move(-steps[cmmMovePage]);
  }

  file1.display();
  file2.display();
} // end handleCmd

//--------------------------------------------------------------------
int main(int argc, char* argv[])
{
  if (argc != 3) {
    fputs("Usage: vbindiff file1 file2\n",stderr);
    return 1;
  }

  if (!initialize()) {
    fputs("Unable to initialize windows\n",stderr);
    return 1;
  }

  file1.setFile(argv[1]);
  file2.setFile(argv[2]);

  diffs.compute();

  file1.display();
  file2.display();

  Command  cmd;
  while ((cmd = getCommand()) != cmQuit)
    handleCmd(cmd);

  file1.shutDown();
  file2.shutDown();

  wm_close_all();               // Close all windows
  wm_exit();                    // End window manager

  return 0;
} // end main

// Local Variables:
// pathsearch-c-include-path: emx-c-include-path
// End: