
/*-------------------------------------------------------------------------*/
/**
  @file		gnuplot_i.h
  @author	N. Devillard
  @date		Sep 1998, Oct 2004, Sept 2005, Nov 2005, Apr 2006
  @version	$Revision: 1.11.3 $
  @brief	C interface to gnuplot.

  gnuplot is a freely available, command-driven graphical display tool for
  Unix. It compiles and works quite well on a number of Unix flavours as
  well as other operating systems. The following module enables sending
  display requests to gnuplot through simple C calls.
  
  set_zlabel, splot and hardcopy_colour functions are based on existing code,
  and added on 12 October 2004 by Robert Bradley (robert.bradley@merton.ox.ac.uk).
  OS X and limited Windows support added 11-13th September 2005 - on Windows,
  pgnuplot and wgnuplot.exe must be in the current directory.  
  
  gnuplot_splot_grid added 2nd April 2006 (R. Bradley).
*/
/*--------------------------------------------------------------------------*/

/*
	$Id: gnuplot_i.h,v 1.11 2003/01/27 08:58:04 ndevilla Exp $
	$Author: ndevilla $
	$Date: 2003/01/27 08:58:04 $
	$Revision: 1.11.2 $
 */

#ifndef _GNUPLOT_PIPES_H_
#define _GNUPLOT_PIPES_H_

#ifdef __cplusplus
extern "C" {
#endif 

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>

/** Maximal number of simultaneous temporary files */
#define GP_MAX_TMP_FILES    64
/** Maximal size of a temporary file name */
#define GP_TMP_NAME_SIZE    512


/*---------------------------------------------------------------------------
                                New Types
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  @typedef	gnuplot_ctrl
  @brief	gnuplot session handle (opaque type).

  This structure holds all necessary information to talk to a gnuplot
  session. It is built and returned by gnuplot_init() and later used
  by all functions in this module to communicate with the session, then
  meant to be closed by gnuplot_close().

  This structure is meant to remain opaque, you normally do not need
  to know what is contained in there.
 */
/*-------------------------------------------------------------------------*/

typedef struct _GNUPLOT_CTRL_ {
    /** Pipe to gnuplot process */
    FILE    * gnucmd ;
    
    /** Number of currently active plots */
    int       nplots ;
	/** Current plotting style */
    char      pstyle[32] ;
    /* Save terminal name (used by hardcopy functions) */
    char      term[32] ;

    /** Name of temporary files */
    char      to_delete[GP_MAX_TMP_FILES][GP_TMP_NAME_SIZE] ;
	/** Number of temporary files */
    int       ntmp ;
} gnuplot_ctrl ;

/*
gnuplot_point: Simple point struct to allow return of points to the
gnuplot_plot_obj_xy function by callback functions.
*/

typedef struct _GNUPLOT_POINT_ {
    double x;
    double y;
    double z;
} gnuplot_point;

/**
 * Anne: extend gnuplot with possibility to draw lines at once.
 */
typedef struct _GNUPLOT_DATA_ {
	double * 	x;
	double * 	y;
	int 		n;
	char *		title;
	char  		pstyle[32];
} gnuplot_data;

/*---------------------------------------------------------------------------
                        Function ANSI C prototypes
 ---------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/**
  @brief    Find out where a command lives in your PATH.
  @param    pname Name of the program to look for.
  @return   pointer to statically allocated character string.

  This is the C equivalent to the 'which' command in Unix. It parses
  out your PATH environment variable to find out where a command
  lives. The returned character string is statically allocated within
  this function, i.e. there is no need to free it. Beware that the
  contents of this string will change from one call to the next,
  though (as all static variables in a function).

  The input character string must be the name of a command without
  prefixing path of any kind, i.e. only the command name. The returned
  string is the path in which a command matching the same name was
  found.

  Examples (assuming there is a prog named 'hello' in the cwd):

  @verbatim
  gnuplot_get_program_path("hello") returns "."
  gnuplot_get_program_path("ls") returns "/bin"
  gnuplot_get_program_path("csh") returns "/usr/bin"
  gnuplot_get_program_path("/bin/ls") returns NULL
  @endverbatim
  
 */
/*-------------------------------------------------------------------------*/
char * gnuplot_get_program_path(char * pname);

/*-------------------------------------------------------------------------*/
/**
  @brief    Opens up a gnuplot session, ready to receive commands.
  @return   Newly allocated gnuplot control structure.

  This opens up a new gnuplot session, ready for input. The struct
  controlling a gnuplot session should remain opaque and only be
  accessed through the provided functions.

  The session must be closed using gnuplot_close().
 */
/*--------------------------------------------------------------------------*/
gnuplot_ctrl * gnuplot_init(void);

/*-------------------------------------------------------------------------*/
/**
  @brief    Closes a gnuplot session previously opened by gnuplot_init()
  @param    handle Gnuplot session control handle.
  @return   void

  Kills the child PID and deletes all opened temporary files.
  It is mandatory to call this function to close the handle, otherwise
  temporary files are not cleaned and child process might survive.

 */
/*--------------------------------------------------------------------------*/
void gnuplot_close(gnuplot_ctrl * handle);

/*-------------------------------------------------------------------------*/
/**
  @brief    Sends a command to an active gnuplot session.
  @param    handle Gnuplot session control handle
  @param    cmd    Command to send, same as a printf statement.

  This sends a string to an active gnuplot session, to be executed.
  There is strictly no way to know if the command has been
  successfully executed or not.
  The command syntax is the same as printf.

  Examples:

  @code
  gnuplot_cmd(g, "plot %d*x", 23.0);
  gnuplot_cmd(g, "plot %g * cos(%g * x)", 32.0, -3.0);
  @endcode

  Since the communication to the gnuplot process is run through
  a standard Unix pipe, it is only unidirectional. This means that
  it is not possible for this interface to query an error status
  back from gnuplot.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_cmd(gnuplot_ctrl *  handle, char *  cmd, ...);

/*-------------------------------------------------------------------------*/
/**
  @brief    Change the plotting style of a gnuplot session.
  @param    h Gnuplot session control handle
  @param    plot_style Plotting-style to use (character string)
  @return   void

  The provided plotting style is a character string. It must be one of
  the following:

  - lines
  - points
  - linespoints
  - impulses
  - dots
  - steps
  - errorbars
  - boxes
  - boxeserrorbars
 */
/*--------------------------------------------------------------------------*/
void gnuplot_setstyle(gnuplot_ctrl * h, char * plot_style);


void gnuplot_setstyleraw(gnuplot_ctrl *h, char *plot_style);

/*-------------------------------------------------------------------------*/
/**
  @brief	Change the terminal of a gnuplot session.
  @param	h Gnuplot session control handle
  @param	terminal Terminal name (character string)
  @return	void

  No attempt is made to check the validity of the terminal name.  This function
  simply makes a note of it and calls gnuplot_cmd to change the name
 */
/*--------------------------------------------------------------------------*/

void gnuplot_setterm(gnuplot_ctrl * h, char * terminal);

/*-------------------------------------------------------------------------*/
/**
  @brief    Sets the x label of a gnuplot session.
  @param    h Gnuplot session control handle.
  @param    label Character string to use for X label.
  @return   void

  Sets the x label for a gnuplot session.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_set_xlabel(gnuplot_ctrl * h, char * label);


/*-------------------------------------------------------------------------*/
/**
  @brief    Sets the y label of a gnuplot session.
  @param    h Gnuplot session control handle.
  @param    label Character string to use for Y label.
  @return   void

  Sets the y label for a gnuplot session.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_set_ylabel(gnuplot_ctrl * h, char * label);

/*-------------------------------------------------------------------------*/
/**
  @brief    Sets the z label of a gnuplot session.
  @param    h Gnuplot session control handle.
  @param    label Character string to use for Z label.
  @return   void

  Sets the z label for a gnuplot session.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_set_zlabel(gnuplot_ctrl * h, char * label);

/*-------------------------------------------------------------------------*/
/**
  @brief    Resets a gnuplot session (next plot will erase previous ones).
  @param    h Gnuplot session control handle.
  @return   void

  Resets a gnuplot session, i.e. the next plot will erase all previous
  ones.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_resetplot(gnuplot_ctrl * h);


/*-------------------------------------------------------------------------*/
/**
  @brief    Plots a 2d graph from a list of doubles.
  @param    handle  Gnuplot session control handle.
  @param    d       Array of doubles.
  @param    n       Number of values in the passed array.
  @param    title   Title of the plot.
  @return   void

  Plots out a 2d graph from a list of doubles. The x-coordinate is the
  index of the double in the list, the y coordinate is the double in
  the list.

  Example:

  @code
    gnuplot_ctrl    *h ;
    double          d[50] ;
    int             i ;

    h = gnuplot_init() ;
    for (i=0 ; i<50 ; i++) {
        d[i] = (double)(i*i) ;
    }
    gnuplot_plot_x(h, d, 50, "parabola") ;
    sleep(2) ;
    gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/
void gnuplot_plot_x(gnuplot_ctrl * handle, double * d, int n, char * title);

/*-------------------------------------------------------------------------*/
/**
  @brief    Plot a 2d graph from a list of points.
  @param    handle      Gnuplot session control handle.
  @param    x           Pointer to a list of x coordinates.
  @param    y           Pointer to a list of y coordinates.
  @param    n           Number of doubles in x (assumed the same as in y).
  @param    title       Title of the plot.
  @return   void

  Plots out a 2d graph from a list of points. Provide points through a list
  of x and a list of y coordinates. Both provided arrays are assumed to
  contain the same number of values.

  @code
    gnuplot_ctrl    *h ;
    double          x[50] ;
    double          y[50] ;
    int             i ;

    h = gnuplot_init() ;
    for (i=0 ; i<50 ; i++) {
        x[i] = (double)(i)/10.0 ;
        y[i] = x[i] * x[i] ;
    }
    gnuplot_plot_xy(h, x, y, 50, "parabola") ;
    sleep(2) ;
    gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/
void gnuplot_plot_xy(
    gnuplot_ctrl    *   handle,
    double          *   x,
    double          *   y,
    int                 n,
    char            *   title
) ;

void gnuplot_plot_xy_N(
		gnuplot_ctrl    *   handle,
		gnuplot_data	*   data,
		int					plots
);

/*-------------------------------------------------------------------------*/
/**
  @brief    Plot a 3d graph from a list of points.
  @param    handle      Gnuplot session control handle.
  @param    x           Pointer to a list of x coordinates.
  @param    y           Pointer to a list of y coordinates.
  @param    z           Pointer to a list of z coordinates.
  @param    n           Number of doubles in x (assumed the same as in y and z).
  @param    title       Title of the plot.
  @return   void

  Plots out a 3d graph from a list of points. Provide points through lists
  of x, y and z coordinates. Both provided arrays are assumed to contain the
  same number of values.
*/
int gnuplot_splot(
    gnuplot_ctrl    *   handle,
    double          *   x,
    double          *   y,
    double          *   z,
    int                 n,
    char            *   title
) ;

/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a 3d graph from a grid of points.
  @param	handle		Gnuplot session control handle.
  @param	points		Pointer to a grid of points (rows,cols).
  @param	rows       	Number of rows (y points).
  @param	cols       	Number of columns (x points).
  @param	title		Title of the plot.
  @return	void

  gnuplot_splot_grid(handle,(double *) points,rows,cols,title);
  Based on gnuplot_splot, modifications by Robert Bradley 2/4/2006

  Plots a 3d graph from a grid of points, passed in the form of a 2D array [x,y].
 */
/*--------------------------------------------------------------------------*/

int gnuplot_splot_grid(gnuplot_ctrl *handle, double *points, int rows, int cols, char *title);

/*-------------------------------------------------------------------------*/
/**
  @brief	Plot contours from a list of points.
  @param	handle		Gnuplot session control handle.
  @param	x			Pointer to a list of x coordinates. (length=nx*ny)
  @param	y			Pointer to a list of y coordinates. (length=nx*ny)
  @param	z			Pointer to a list of z coordinates. (length=nx*ny)
  @param	nx			Number of doubles in x-direction
  @param	ny			Number of doubles in y-direction
  @param	title		Title of the plot.
  @return	void

  gnuplot_contour_plot(handle,x,y,z,n,title);
  Based on gnuplot_splot, modifications by Robert Bradley 23/11/2005

  Plots a contour plot from a list of points, passed in the form of three arrays x, y and z.
  
    @code
    gnuplot_ctrl    *h ;
	double x[50] ; y[50] ; z[50];
    int i ;

    h = gnuplot_init() ;
    int count=100;
    double x[count*count],y[count*count],z[count*count];
    int i,j;
    for (i=0;i<count;i++) {
        for (j=0;j<count;j++) {
            x[count*i+j]=i;
            y[count*i+j]=j;
            z[count*i+j]=1000*sqrt(square(i-count/2)+square(j-count/2));
        }
    }
    gnuplot_setstyle(h,"lines");
    gnuplot_contour_plot(h,x,y,z,count,count,"Points");
    sleep(2) ;
    gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/

int gnuplot_contour_plot(gnuplot_ctrl *handle, double *x, double *y, double *z, int nx, int ny, char *title);

/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a 3d graph using callback functions to return the points
  @param	handle		Gnuplot session control handle.
  @param	obj			Pointer to an arbitrary object.
  @param	getPoint	Pointer to a callback function.
  @param	n			Number of doubles in x (y and z must be the the same).
  @param	title		Title of the plot.
  @return	void

Calback:

void getPoint(void *object,gnuplot_point *point,int index,int pointCount);
  @param	obj			Pointer to an arbitrary object
  @param	point		Pointer to the returned point struct (double x,y,z)
  @param	i			Index of the current point (0 to n-1)
  @param	n			Number of points
  @return	void
 */
/*--------------------------------------------------------------------------*/

int gnuplot_splot_obj(gnuplot_ctrl *handle,
                      void *obj,
                      void (*getPoint)(void*,gnuplot_point*,int,int),
                      int n,
                      char *title);

/*
  @brief	Plot a 2d graph using a callback function to return points.
  @param	handle		Gnuplot session control handle.
  @param	obj			Pointer to an arbitrary object.
  @param	getPoint	Pointer to a callback function.
  @param	n			Number of points.
  @param	title		Title of the plot.
  @return	void

The callback function is of the following form, and is called once for each
point plotted:
    
void getPoint(void *object,gnuplot_point *point,int index,int pointCount);
  @param	obj			Pointer to an arbitrary object
  @param	point		Pointer to the returned point struct (double x,y,z)
  @param	i			Index of the current point (0 to n-1)
  @param	n			Number of points
  @return	void
*/

int gnuplot_plot_obj_xy(
    gnuplot_ctrl *handle,
    void *obj,
    void (*getPoint)(void*,gnuplot_point*,int,int),
    int n,
    char *title
);

/*-------------------------------------------------------------------------*/
/** 
  @brief    Open a new session, plot a signal, close the session.
  @param    title   Plot title
  @param    style   Plot style
  @param    label_x Label for X
  @param    label_y Label for Y
  @param    x       Array of X coordinates
  @param    y       Array of Y coordinates (can be NULL)
  @param    n       Number of values in x and y.
  @return

  This function opens a new gnuplot session, plots the provided
  signal as an X or XY signal depending on a provided y, waits for
  a carriage return on stdin and closes the session.

  It is Ok to provide an empty title, empty style, or empty labels for
  X and Y. Defaults are provided in this case.
 */
/*--------------------------------------------------------------------------*/
void gnuplot_plot_once(
    char    *   title,
    char    *   style,
    char    *   label_x,
    char    *   label_y,
    double  *   x,
    double  *   y,
    int         n
);

/*-------------------------------------------------------------------------*/
/**
  @brief    Plot a slope on a gnuplot session.
  @param    handle      Gnuplot session control handle.
  @param    a           Slope.
  @param    b           Intercept.
  @param    title       Title of the plot.
  @return   void
  @doc

  Plot a slope on a gnuplot session. The provided slope has an
  equation of the form y=ax+b

  Example:

  @code
    gnuplot_ctrl    *   h ;
    double              a, b ;

    h = gnuplot_init() ;
    gnuplot_plot_slope(h, 1.0, 0.0, "unity slope") ;
    sleep(2) ;
    gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/
void gnuplot_plot_slope(
    gnuplot_ctrl    *   handle,
    double              a,
    double              b,
    char            *   title
) ;

/*-------------------------------------------------------------------------*/
/**
  @brief    Plot a curve of given equation y=f(x).
  @param    h           Gnuplot session control handle.
  @param    equation    Equation to plot.
  @param    title       Title of the plot.
  @return   void

  Plots out a curve of given equation. The general form of the
  equation is y=f(x), you only provide the f(x) side of the equation.

  Example:

  @code
        gnuplot_ctrl    *h ;
        char            eq[80] ;

        h = gnuplot_init() ;
        strcpy(eq, "sin(x) * cos(2*x)") ;
        gnuplot_plot_equation(h, eq, "sine wave", normal) ;
        gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/
void gnuplot_plot_equation(gnuplot_ctrl * h, char * equation, char * title) ;



/*-------------------------------------------------------------------------*/
/**
  @brief    Save a graph as a postscript file on the hard disk
  @param    h           Gnuplot session control handle.
  @param    equation    Equation to plot.
  @param    title       Title of the plot.
  @return   void

  A wrapper for the gnuplot_cmd function that sets the terminal 
  the terminal to postscript, replots the raph and then resets 
  the terminal to x11.
  
  Example:

  @code
        gnuplot_ctrl    *h ;
        char            eq[80] ;

        h = gnuplot_init() ;
        strcpy(eq, "sin(x) * cos(2*x)") ;
        gnuplot_plot_equation(h, eq, "sine wave", normal) ;
        gnuplot_hardcopy(h, "sinewave.ps");
        gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/
void gnuplot_hardcopy(gnuplot_ctrl * h, char * filename);
// gnuplot_hardcopy_colour added Oct 2004 by Robert Bradley
void gnuplot_hardcopy_colour(gnuplot_ctrl * h, char * filename);

#ifdef __cplusplus
}
#endif 

#endif
