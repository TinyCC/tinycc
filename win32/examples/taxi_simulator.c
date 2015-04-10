/* A Windows console program: taxi and passengers simulator */

#include <windows.h>
#include <wincon.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>

void init_scr();
void restore_scr();
void clrscr();
void write_console(const char *msg, DWORD len, COORD coords, DWORD attr);

int  add_br (int n, int addon);

DWORD WINAPI Si1  (LPVOID parm);	// a passengers thread
DWORD WINAPI Si2  (LPVOID parm);
DWORD WINAPI Si3  (LPVOID parm);
DWORD WINAPI Si4  (LPVOID parm);
DWORD WINAPI Taxi (LPVOID parm);	// a taxi thread

HANDLE hstdout;
HANDLE hproc[5], htaxi, hproc2[5], hproc3[5], hproc4[5], ht2;

HANDLE hs_br;			// a semaphor for br[]
HANDLE hs_wc;			// a semaphor for write_console()
HANDLE hs_ds;			// a semaphor for draw_statistics()
HANDLE hs_st;			// a semaphor for draw_station()

int br[5];			// a number of the passengers on the taxi stations
int w;				// a number of the passengers on the taxi, used in taxi()

DWORD si_attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
DWORD st_attr = FOREGROUND_GREEN;
DWORD tx_attr = FOREGROUND_RED;

static
void draw_statistics()
{
  char msg[100];
  COORD coords;

  WaitForSingleObject (hs_ds, INFINITE);

  coords.X = 0;
  coords.Y = 0;

  sprintf (msg, "station1=%d \n", br[1]);
  write_console (msg, strlen(msg), coords, si_attr);

  coords.Y++;
  sprintf (msg, "station2=%d \n", br[2]);
  write_console (msg, strlen(msg), coords, si_attr);

  coords.Y++;
  sprintf (msg, "station3=%d \n", br[3]);
  write_console (msg, strlen(msg), coords, si_attr);

  coords.Y++;
  sprintf (msg, "station4=%d \n", br[4]);
  write_console (msg, strlen(msg), coords, si_attr);

  coords.Y++;
  sprintf (msg, "taxi=%d \n", w);
  write_console (msg, strlen(msg), coords, si_attr);

  ReleaseSemaphore (hs_ds, 1, NULL);
}

static
void draw_station(int station)
{
  char msg[100];
  COORD coords;
  int j;

  WaitForSingleObject (hs_st, INFINITE);

  if (station == 1)
  {
    int br1, n, c;
 
    coords.X = 34;
    coords.Y = 3;
    write_console (" st1 ", 5, coords, st_attr);
    coords.Y++;
    write_console ("-----", 5, coords, st_attr);

    br1 = add_br (1, 0);
    coords.X = 40;
    c = 3;
    while (c-- > 0)
    {
      n  = 5;
      if (br1 < 5)
        n = br1;

      if (n != 0)
      for (j = 5; j > 5-n; j--)
      {
        coords.Y = j - 1;
        write_console ("*", 1, coords, si_attr);
      }

      if (n != 5)
      for (j = 5-n; j > 0; j--)
      {
        coords.Y = j - 1;
        write_console (" ", 1, coords, si_attr);
      }

      coords.X++;
      br1 -= n;
    }
  }
  else
  if (station == 3)
  {
    int br3, n, c;

    coords.X = 34;
    coords.Y = 20;
    write_console (" st3 ", 5, coords, st_attr);
    coords.Y--;
    write_console ("-----", 5, coords, st_attr);

    br3 = add_br (3, 0);
    coords.X = 33;
    c = 3;
    while (c-- > 0)
    {
      n = 5;
      if (br3 < 5)
        n = br3;

      if (n != 0)
      for (j = 0; j < n; j++)
      {
        coords.Y = 20 + j;
        write_console ("*", 1, coords, si_attr);
      }

      if (n != 5)
      for (j = n; j < 5; j++)
      {
        coords.Y = 20 + j;
        write_console (" ", 1, coords, si_attr);
      }

      coords.X--;
      br3 -= n;
    }
  }
  else
  if (station == 2)
  {
    int br2, n, c;

    coords.X = 69;
    coords.Y = 11;
    write_console ("st2", 3, coords, st_attr);
    coords.X -= 2;
    for (j = 14; j >= 10; j--)
    {
      coords.Y = j;
      write_console ("|", 1, coords, st_attr);
    }

    br2 = add_br (2, 0);
    coords.Y = 13;
    c = 3;
    while (c-- > 0)
    {
      n = 5;
      if (br2 < 5)
        n = br2;

      coords.X = 79;
      write_console (" ", 1, coords, si_attr);

      coords.Y++;
      coords.X = 68;
      write_console ("*****", br2, coords, si_attr);

      coords.X = 68+n;
      write_console ("            ", 11-n, coords, si_attr);

      coords.Y++;
      coords.X = 79;
      write_console (" ", 1, coords, si_attr);

      coords.Y--;
      br2 -= n;
    }
  }
  else
  if (station == 4)
  {
    int br4, n, c;

    coords.X = 7;
    coords.Y = 13;
    write_console ("st4", 3, coords, st_attr);
    coords.X += 4;
    for (j = 10; j <= 14; j++)
    {
      coords.Y = j;
      write_console ("|", 1, coords, st_attr);
    }

    coords.Y = 9;
    br4 = add_br (4, 0);
    c = 3;
    while (c-- > 0)
    {
      n = 5;
      if (br4 < 5)
        n = br4;

      coords.X = 0;
      write_console (" ", 1, coords, si_attr);
     
      coords.Y++;
      write_console ("          ", 10, coords, si_attr);

      coords.X = 10 - n;
      write_console ("**********", n, coords, si_attr);

      coords.Y++;
      coords.X = 0;
      write_console (" ", 1, coords, si_attr);

      coords.Y--;
      br4 -= n;
    }
  }

  ReleaseSemaphore (hs_st, 1, NULL);
}

int
main (void)
{
  int N;			// a passengers on the all stations
  int i, j;
  COORD coords;			// a cursor coords
  DWORD len;
  int   br1, br2, br3, br4;	// a passengers on the station X

  init_scr();

  printf ("Taxi and passengers simulator:\n");
  while (1)
    {
      printf ("  enter a max passengers number on the stations (0..20) = ");
      scanf ("%d", &N);
      if ((N >= 0) && (N<=20))
        break;
    }
  while (1)
    {
      printf ("  enter a passengers on the taxi (0..10) = ");
      scanf ("%d", &w);
      if ((w >= 0) && (w<=10))
        break;
    }
  init_scr();

  srand (time (0));
  br1 = rand () % 6;	 		// passngers on the station1, 0..5
  if (br1 > N)
    br1 = N;

  br2 = rand () % 6;
  if (br2 > N - br1)
    br2 = N - br1;

  br3 = rand () % 6;
  if (br3 > N - br1 - br2)
    br3 = N - br1 - br2;

  br4 = N - br1 - br2 - br3;
  while (br4 > 5)
  {
    br4--;
    if (br1 < 5) { br1++; continue; }
    if (br2 < 5) { br2++; continue; }
    if (br3 < 5) { br3++; continue; }
  }

  hs_br = CreateSemaphore (NULL, 1,  1, NULL);
  hs_wc = CreateSemaphore (NULL, 1,  1, NULL);
  hs_ds = CreateSemaphore (NULL, 1,  1, NULL);
  hs_st = CreateSemaphore (NULL, 1,  1, NULL);

  draw_statistics();
  for (i=1; i<=4; i++)
    draw_station(i);

  htaxi = CreateThread (NULL, 4096, Taxi, NULL, 0, NULL);

  for (i = 0; i < br1; i++) {
      srand(time(0));
      Sleep(600*(4+rand()%10));
      hproc[i] = CreateThread (NULL, 4096, Si1, NULL, 0, NULL);
  }

  for (i = 0; i < br2; i++) {
      srand(time(0));
      Sleep(400*(4+rand()%10));
      hproc2[i] = CreateThread (NULL, 4096, Si2, NULL, 0, NULL);
  }

  for (i = 0; i < br3; i++) {
      srand(time(0));
      Sleep(600*(4+rand()%10));
      hproc3[i] = CreateThread (NULL, 4096, Si3, NULL, 0, NULL);
  }

  for (i = 0; i < br4; i++) {
      srand(time(0));
      Sleep(600*(4+rand()%10));
      hproc4[i] = CreateThread (NULL, 4096, Si4, NULL, 0, NULL);
  }

  getch ();
  restore_scr();
  return 0;
}

static
void draw_taxi (int orientation, COORD coords)
{
  static int   old_orientation;
  static COORD old_coords;

  if (orientation != 0) {
    old_orientation = orientation;
    old_coords = coords;
  }
  else {
    orientation = old_orientation;
    coords = old_coords;
  }

  if (orientation == 1)
  {
      int f, b, ii;
      f = 5;
      if (w < 5)
        f = w;
      b = w - f;

      write_console ("  ----- ", 8, coords, tx_attr);

      coords.Y++;
      write_console (" |     |", 8, coords, tx_attr);
      coords.X += 2 + 5 - b;
      write_console (  "ooooo", b, coords, tx_attr);
      coords.X -= 2 + 5 - b;

      coords.Y++;
      write_console (" |     |", 8, coords, tx_attr);
      coords.X += 2 + 5 - f;
      write_console (  "ooooo", f, coords, tx_attr);
      coords.X -= 2 + 5 - f;

      coords.Y++;
      write_console ("  ----- ", 8, coords, tx_attr);

      for (ii=0; ii<5; ii++)
      {
      coords.Y++;
      write_console ("        ", 8, coords, tx_attr);
      }
  }
  else
  if (orientation == 2)
  {
      COORD coords2;
      int f, b, ii;

      f = 5;
      if (f > w)
        f = w;
      b = w - f;

      coords2.X = coords.X;
      coords.Y--;
      write_console ("       ", 7, coords, tx_attr);
      coords.Y++;
      write_console ("   --  ", 7, coords, tx_attr);

      for (ii=0; ii < 5; ii++)
      {
        coords2.Y = coords.Y + 5 - ii;
        if (f && b) {
          write_console ("  |oo| ", 7, coords2, tx_attr);
          f--; b--;
        }
        else
        if (f) {
          write_console ("  |o | ", 7, coords2, tx_attr);
          f--;
        }
        else {
          write_console ("  |  | ", 7, coords2, tx_attr);
        }
      }

      coords.Y += 6;
      write_console ("   --  ", 7, coords, tx_attr);
  }
  else
  if (orientation == 3)
  {
      int ii;
      for (ii=0; ii < 4; ii++)
      {
      write_console ("        ", 8, coords, tx_attr);
      coords.Y++;
      }

      write_console (" -----  ", 8, coords, tx_attr);

      coords.Y++;
      write_console ("|     | ", 8, coords, tx_attr);
      coords.X++;
      ii = 5;
      if (w < 5)
        ii = w;
      write_console ( "ooooo",   ii, coords, tx_attr);
      coords.X--;

      coords.Y++;
      write_console ("|     | ", 8, coords, tx_attr);
      coords.X++;
      ii = w - ii;
      write_console ( "ooooo",  ii, coords, tx_attr);
      coords.X--;

      coords.Y++;
      write_console (" -----  ", 8, coords, tx_attr);
  }
  else
  if (orientation == 4)
  {
      int f, b, ii;

      f = 5;
      if (f > w)
        f = w;
      b = w - f;

      write_console ("  --   ", 7, coords, tx_attr);

      for (ii=0; ii < 5; ii++)
      {
        coords.Y++;
        if (f && b) {
          write_console (" |oo|  ", 7, coords, tx_attr);
          f--; b--;
        }
        else
        if (f) {
          write_console (" | o|  ", 7, coords, tx_attr);
          f--;
        }
        else
          write_console (" |  |  ", 7, coords, tx_attr);
      }

      coords.Y++;
      write_console ("  --   ", 7, coords, tx_attr);

      coords.Y++;
      write_console ("       ", 7, coords, tx_attr);
  }
}

DWORD WINAPI
Taxi (LPVOID _unused_thread_parm)
{
  int i, ii, j, k, f, b;
  int empty_st;
  COORD coords;
  DWORD len;
  char msg[100];

  for (j = 0; j < 48; j++)                              // taxi left
    {
      coords.X = 59 - j;
      coords.Y = 11;
      draw_taxi (3, coords);

      if (j == 23)		// station 3 for a passengers exit
	{
	  int br3 = add_br (3, 0);
          empty_st = (br3 == 0);

          srand (time (0));
	  f = rand () % (16 - br3); // passengers can be on the station

	  if (w)
          {
            while(f)
	    {
              if (w==0)
		break;

              f--;
	      w--;
	      add_br (3, 1);
              draw_statistics();
	      draw_station(3);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}

      if (j == 28)		// station 3 for enter
      {
	  int br3 = add_br (3, 0);
	  f = rand () % (11 - w); // passengers into taxi


          if (br3 && !empty_st)
          {
	    while (br3 > 0 && f > 0)
	    {
              f--;
	      w++;
	      br3 = add_br (3, -1);

	      draw_statistics();
	      draw_station(3);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
      }
      Sleep (300);
    }

  for (i = 0; i < 10; i++)	// taxi up
    {
      coords.X = 12;
      coords.Y = 15 - i;
      draw_taxi(4, coords);

      if (i == 4)		// station 4 for exit
	{
	  int br4 = add_br (4, 0);
          empty_st = (br4 == 0);

          srand (time (0));
	  f = rand () % (16 - br4);

	  if (w)
          {
            while(f)
	    {
              if (w==0)
		break;

              f--;
	      w--;
	      add_br (4, 1);
              draw_statistics();
	      draw_station(4);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}

      if (i == 7)		  // station 4 for enter
	{
	  int br4 = add_br (4, 0);
	  f = rand () % (11 - w);


          if (br4 && !empty_st)
          {
	    while (br4 > 0 && f > 0)
	    {
	      f--;
	      w++;
	      br4 = add_br (4, -1);
 
	      draw_statistics();
	      draw_station(4);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}
       Sleep (300);
    }

  for (k = 0; k < 48; k++)	// taxi rigth
    {
      coords.X = 12 + k;
      coords.Y = 5;
      draw_taxi (1, coords);

      if (k == 19)		// station 1 for exit
	{
	  int br1 = add_br (1, 0);
          empty_st = (br1 == 0);

          srand (time (0));
	  f = rand () % (16 - br1);

	  if (w)
          {
            while(f)
	    {
              if (w==0)
		break;

              f--;
	      w--;
	      add_br (1, 1);
              draw_statistics();
	      draw_station(1);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}

      if (k == 23)		// station 1 for enter
	{
	  int br1 = add_br (1, 0);
	  f = rand () % (11 - w);

          if (br1 && !empty_st)
          {
	    while (br1 > 0 && f > 0)
	    {
	      f--;
	      w++;
	      br1 = add_br (1, -1);

	      draw_statistics();
	      draw_station(1);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}
      Sleep (300);
    }

  for (i = 0; i < 9; i++)	// taxi down
    {
      coords.X = 60;
      coords.Y = 3 + i;
      draw_taxi (2, coords);

      if (i == 4)			// station 2 for exit
	{
	  int br2 = add_br (2, 0);
          empty_st = (br2 == 0);

          srand (time (0));
	  f = rand () % (16 - br2);

	  if (w)
          {
            while(f)
	    {
              if (w==0)
		break;

              f--;
	      w--;
	      add_br (2, 1);
              draw_statistics();
	      draw_station(2);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}

      if (i == 6)			// station 2 for enter
	{
	  int br2 = add_br (2, 0);
	  f = rand () % (11 - w);

	  if (br2 && !empty_st)
          {
	    while (br2 > 0 && f > 0)
	    {			
              f--;
              w++;
              br2 = add_br (2, -1);

	      draw_statistics();
	      draw_station(2);
	      draw_taxi(0, coords);
	      Sleep (300);
	    }
	    Sleep (3000);
          }
	}

      Sleep (300);
    }

  ht2 = CreateThread (NULL, 4096, Taxi, NULL, 0, NULL);	// endless loop
  return 0;
}

DWORD WINAPI
Si1 (LPVOID _unused_thread_parm) // a passengers thread for the station 1
{
  int a, j, n;
  COORD coords;

  srand (time (0));
  a = 6 + rand () % 74;		// a new passengers number, 6..79

  while (a != 40)
    {
      if (a < 40)
	  a++;
      else
	  a--;

      coords.X = a;
      coords.Y = 0;

      if (a < 40)
        write_console (" *", 2, coords, si_attr);
      else
        write_console ("* ", 2, coords, si_attr);

      Sleep (1000);
    }

  add_br (1, 1);
  draw_station(1);
  draw_statistics();

  Sleep (1000);
  return 0;
}

DWORD WINAPI
Si3 (LPVOID _unused_thread_parm) // a passengers thread for the station 3
{
  int b, j, n;
  COORD coords;

  srand (time (0));
  b = 1 + rand () % 79;

  while (b != 33)
    {
      if (b < 33)
	  b++;
      else
	  b--;

      coords.X = b;
      coords.Y = 24;

      if (b < 33)
        write_console (" *", 2, coords, si_attr);
      else
        write_console ("* ", 2, coords, si_attr);

      Sleep (1000);
    }

  add_br (3, 1);
  draw_station(3);
  draw_statistics();

  Sleep (1000);
  return 0;
}

DWORD WINAPI
Si4 (LPVOID _unused_thread_parm) // a passengers thread for the station 4
{

  int c, j, n;
  COORD coords;

  srand (time (0));
  c = 6 + rand () % 19;		// a new passengers number, 6..24

  coords.X = 0;
  while (c != 10)
    {
      coords.Y = c;
      write_console ("*", 1, coords, si_attr);

      if (c < 10) {
        c++;
	coords.Y--;
      }
      else {
        c--;
	coords.Y++;
      }
      write_console (" ", 1, coords, si_attr);

      Sleep (1000);
    }

  add_br (4, 1);
  draw_station(4);
  draw_statistics();

  Sleep (1000);
  return 0;
}

DWORD WINAPI
Si2 (LPVOID _unused_thread_parm) // a passengers thread for the station 2
{
  int d, j, n;
  COORD coords;

  srand (time (0));
  d = 1 + rand () % 24;		 // a new passengers number, 1..24

  coords.X = 79;
  while (d != 14)
    {
      coords.Y = d;
      write_console ("*", 1, coords, si_attr);
      if (d < 14)
	{
	  d++;
	  coords.Y--;
        }
      else
      {
	  d--;
	  coords.Y++;
      }
      write_console (" ", 1, coords, si_attr);
      Sleep (1000);
    }

  add_br (2, 1);
  draw_station(2);
  draw_statistics();

  Sleep (1000);
  return 0;
}

UINT oldcp;				// CodePage on the program start
CONSOLE_CURSOR_INFO old_ci;

void init_scr()
{
  hstdout = GetStdHandle (STD_OUTPUT_HANDLE);
  if (hstdout == INVALID_HANDLE_VALUE)
    {
      printf ("Error, can't get console handle\n");
      exit (1);
    }

  if (oldcp == 0)
  {
    oldcp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    GetConsoleCursorInfo (hstdout, &old_ci);
  }
  else
  {
    CONSOLE_CURSOR_INFO new_ci;

    new_ci.bVisible = FALSE;
    new_ci.dwSize   = old_ci.dwSize;
    SetConsoleCursorInfo (hstdout, &new_ci);
  }

  clrscr ();
}

void restore_scr()
{
  SetConsoleOutputCP(oldcp);
  SetConsoleCursorInfo (hstdout, &old_ci);
  clrscr ();
}

void clrscr()
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD nocw;
  COORD coords;

  SetConsoleTextAttribute (hstdout, FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN);
  GetConsoleScreenBufferInfo (hstdout, &csbi);

  // printf ("csbi.dwSize.X=%u csbi.dwSize.Y=%u\n",
  //	csbi.dwSize.X, csbi.dwSize.Y);
  //  getch ();

  coords.X=0; coords.Y=0;
  SetConsoleCursorPosition (hstdout, coords);
  FillConsoleOutputCharacterA(hstdout, ' ', csbi.dwSize.X * csbi.dwSize.Y, coords, &nocw);
}

void write_console(const char *msg, DWORD len, COORD coords, DWORD attr)
{
  DWORD len2;

  WaitForSingleObject (hs_wc, INFINITE);

  SetConsoleTextAttribute (hstdout, attr);
  SetConsoleCursorPosition (hstdout, coords);
  WriteConsole (hstdout, msg, len, &len2, NULL);

  ReleaseSemaphore (hs_wc, 1, NULL);
}

int add_br (int n, int addon)
{
  int new;

  WaitForSingleObject (hs_br, INFINITE);
  br[n] += addon;
  new = br[n];
  ReleaseSemaphore (hs_br, 1, NULL);

  return new;
}
