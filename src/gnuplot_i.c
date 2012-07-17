#ifndef _GNUPLOT_PIPES_C_
#define _GNUPLOT_PIPES_C_
/*-------------------------------------------------------------------------*/
/**
  @file		gnuplot_i.c
  @author	N. Devillard (parts R. Bradley)
  @date	Sep 1998, Oct 2004, Sept 2005, Nov 2005, Apr 2006
  @version	$Revision: 2.10.3 $
  @brief	C interface to gnuplot.

  gnuplot is a freely available, command-driven graphical display tool for
  Unix. It compiles and works quite well on a number of Unix flavours as
  well as other operating systems. The following module enables sending
  display requests to gnuplot through simple C calls.

  3D and colour printing extensions added October 2004 by Robert Bradley
  (robert.bradley@merton.ox.ac.uk), based on existing code:

  gnuplot_splot (plot 3D graph), gnuplot_set_zlabel (set the z axis label for 3D plots),
  gnuplot_hardcopy_colour (for printing in colour)

  OS X and limited Windows support added 11-13th September 2005:

  On Windows, pgnuplot and wgnuplot.exe must be in the current directory.
  On OS X, if DISPLAY environment variable is not set, or USE_AQUA=1, aqua is
  used instead of x11.
  gnuplot_setterm also added to allow the terminal type to be set.

  gnuplot_plot_obj_xy added 15th September 2005 - plotting data from arbitrary
  structures using callback functions
  gnuplot_contour_plot added 23rd November 2005 (plots contours, use
  gnuplot_setstyle to set the contour style)

  gnuplot_splot_grid added 2nd April 2006 - plots 2D grids of data [x,y]
 */
/*--------------------------------------------------------------------------*/

/*
	$Id: gnuplot_i.c,v 2.10 2003/01/27 08:58:04 ndevilla Exp $
	$Author: ndevilla $
	$Date: 2003/01/27 08:58:04 $
	$Revision: 2.10.2 $
 */

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/

#include "gnuplot_i.h"

/*---------------------------------------------------------------------------
                                Defines
 ---------------------------------------------------------------------------*/

/** Maximal size of a gnuplot command */
#define GP_CMD_SIZE     	2048
/** Maximal size of a plot title */
#define GP_TITLE_SIZE   	80
/** Maximal size for an equation */
#define GP_EQ_SIZE      	512
/** Maximal size of a name in the PATH */
#define PATH_MAXNAMESZ       4096

#ifdef _WIN32
#undef P_tmpdir
#endif
/** Define P_tmpdir if not defined (this is normally a POSIX symbol) */
#ifndef P_tmpdir
#define P_tmpdir "."
#endif

//#undef P_tmpdir
//#define P_tmpdir "/home/anne/temp"

#define GNUPLOT_TEMPFILE "%s/gnuplot-i-XXXXXX"
#ifndef _WIN32
#define GNUPLOT_EXEC "gnuplot"
#else
#define GNUPLOT_EXEC "pgnuplot.exe"
#endif

/*---------------------------------------------------------------------------
                            Function codes
 ---------------------------------------------------------------------------*/

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>

int mkstemp(char* templ) {
	srand(time(NULL));
	int i;
	char *start=strstr(templ, "XXXXXX");
	for (i=0;i<6;i++) {
		start[i]=(char) (48+((int) rand()*10/32768.0));
	}
	//while ((start=strchr(templ, '/'))!=NULL) {start[0]='-';}
	//printf("Opening %s...\n",templ);
	i=open(templ,O_RDWR | O_CREAT);
	if (i!=-1) {
		DWORD dwFileAttr=GetFileAttributes(templ);
		SetFileAttributes(templ,dwFileAttr & !FILE_ATTRIBUTE_READONLY);
	}
	return i;
}
#endif

/*-------------------------------------------------------------------------*/
/**
  @brief	Find out where a command lives in your PATH.
  @param	pname Name of the program to look for.
  @return	pointer to statically allocated character string.

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

char * gnuplot_get_program_path(char * pname)
		{
	int         i, j, lg;
	char    *   path;
	static char buf[PATH_MAXNAMESZ];

	/* Trivial case: try in CWD */
	sprintf(buf, "./%s", pname) ;
	if (access(buf, X_OK)==0) {
		sprintf(buf, ".");
		return buf ;
	}
	/* Try out in all paths given in the PATH variable */
	buf[0] = 0;
	path = getenv("PATH") ;
	if (path!=NULL) {
		for (i=0; path[i]; ) {
			for (j=i ; (path[j]) && (path[j]!=':') ; j++);
			lg = j - i;
			strncpy(buf, path + i, lg);
			if (lg == 0) buf[lg++] = '.';
			buf[lg++] = '/';
			strcpy(buf + lg, pname);
			if (access(buf, X_OK) == 0) {
				/* Found it! */
				break ;
			}
			buf[0] = 0;
			i = j;
			if (path[i] == ':') i++ ;
		}
	} else {
		fprintf(stderr, "PATH variable not set\n");
	}
	/* If the buffer is still empty, the command was not found */
	if (buf[0] == 0) return NULL ;
	/* Otherwise truncate the command name to yield path only */
	lg = strlen(buf) - 1 ;
	while (buf[lg]!='/') {
		buf[lg]=0 ;
		lg -- ;
	}
	buf[lg] = 0;
	return buf ;
		}

/*-------------------------------------------------------------------------*/
/**
  @brief	Opens up a gnuplot session, ready to receive commands.
  @return	Newly allocated gnuplot control structure.

  This opens up a new gnuplot session, ready for input. The struct
  controlling a gnuplot session should remain opaque and only be
  accessed through the provided functions.

  The session must be closed using gnuplot_close().
 */
/*--------------------------------------------------------------------------*/

gnuplot_ctrl * gnuplot_init(void)
		{
	gnuplot_ctrl *  handle ;
#ifndef _WIN32
#ifndef __APPLE__
	if (getenv("DISPLAY") == NULL) {
		fprintf(stderr, "cannot find DISPLAY variable: is it set?\n") ;
	}
#endif
#endif
#ifndef _WIN32
	if (gnuplot_get_program_path("gnuplot")==NULL) {
		fprintf(stderr, "cannot find gnuplot in your PATH");
		return NULL ;
	}
#endif
	if (gnuplot_get_program_path(GNUPLOT_EXEC)==NULL) {
		fprintf(stderr, "cannot find gnuplot in your PATH");
		return NULL ;
	}

	/* 
	 * Structure initialization:
	 */
	handle = (gnuplot_ctrl*)malloc(sizeof(gnuplot_ctrl)) ;
	handle->nplots = 0 ;
	gnuplot_setstyle(handle, "points") ;
	handle->ntmp = 0 ;
	handle->gnucmd = popen(GNUPLOT_EXEC, "w") ;
	if (handle->gnucmd == NULL) {
		fprintf(stderr, "error starting gnuplot\n") ;
		free(handle) ;
		return NULL ;
	}
	//	else {
	//		fprintf(stdout, "We can open gnuplot indeed\n");
	//	}
#ifdef _WIN32
	gnuplot_setterm(handle,"windows");
#elif __APPLE__
	// Try to determine whether we should use aqua or x11 as our default
	if (getenv("DISPLAY") == NULL || (getenv("USE_AQUA")!=NULL && strcmp(getenv("USE_AQUA"),"1")>=0))
		gnuplot_setterm(handle,"aqua");
	else
		gnuplot_setterm(handle,"x11");
#else
	gnuplot_setterm(handle,"x11");
#endif
	return handle;
		}

/*-------------------------------------------------------------------------*/
/**
  @brief	Closes a gnuplot session previously opened by gnuplot_init()
  @param	handle Gnuplot session control handle.
  @return	void

  Kills the child PID and deletes all opened temporary files.
  It is mandatory to call this function to close the handle, otherwise
  temporary files are not cleaned and child process might survive.

 */
/*--------------------------------------------------------------------------*/

void gnuplot_close(gnuplot_ctrl * handle)
{
	int     i ;

	if (pclose(handle->gnucmd) == -1) {
		fprintf(stderr, "problem closing communication to gnuplot\n") ;
		return ;
	}
	if (handle->ntmp) {
		for (i=0 ; i<handle->ntmp ; i++) {
#ifdef _WIN32
			int x=remove(handle->to_delete[i]) ;
			if (x)
				printf("Failure to delate %s: error number %d\n",handle->to_delete[i],errno);
#else
			remove(handle->to_delete[i]);
#endif
		}
	}
	free(handle) ;
	return ;
}


/*-------------------------------------------------------------------------*/
/**
  @brief	Sends a command to an active gnuplot session.
  @param	handle Gnuplot session control handle
  @param	cmd    Command to send, same as a printf statement.

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

void gnuplot_cmd(gnuplot_ctrl *  handle, char *  cmd, ...)
{
	va_list ap ;
	char    local_cmd[GP_CMD_SIZE];

	va_start(ap, cmd);
	vsprintf(local_cmd, cmd, ap);
	va_end(ap);

	strcat(local_cmd, "\n");

	fputs(local_cmd, handle->gnucmd) ;
	fflush(handle->gnucmd);
	return ;
}


/*-------------------------------------------------------------------------*/
/**
  @brief	Change the plotting style of a gnuplot session.
  @param	h Gnuplot session control handle
  @param	plot_style Plotting-style to use (character string)
  @return	void

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

/**
 * ANNE: add gnuplot_setwithstyle, have "lines with pt" accepted.
 */
void gnuplot_setstyle(gnuplot_ctrl * h, char * plot_style) 
{
	if (strcmp(plot_style, "lines") &&
			strcmp(plot_style, "points") &&
			strcmp(plot_style, "linespoints") &&
			strcmp(plot_style, "impulses") &&
			strcmp(plot_style, "dots") &&
			strcmp(plot_style, "steps") &&
			strcmp(plot_style, "errorbars") &&
			strcmp(plot_style, "boxes") &&
			strcmp(plot_style, "boxerrorbars")) {
		fprintf(stderr, "warning: unknown requested style: using points\n") ;
		strcpy(h->pstyle, "points") ;
	} else {
		strcpy(h->pstyle, plot_style) ;
	}
	return ;
}

// no checking, so plot_style can be "points pt 7 ps 4 lc rgb" e.g.
void gnuplot_setstyleraw(gnuplot_ctrl *h, char *plot_style) {
	strcpy(h->pstyle, plot_style);
}

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

void gnuplot_setterm(gnuplot_ctrl * h, char * terminal) 
{
	char cmd[64];
	strncpy(h->term, terminal,32);
	h->term[31]=0;
	sprintf(cmd,"set terminal %s",h->term);
	gnuplot_cmd(h,cmd);
}


/*-------------------------------------------------------------------------*/
/**
  @brief	Sets the x label of a gnuplot session.
  @param	h Gnuplot session control handle.
  @param	label Character string to use for X label.
  @return	void

  Sets the x label for a gnuplot session.
 */
/*--------------------------------------------------------------------------*/

void gnuplot_set_xlabel(gnuplot_ctrl * h, char * label)
{
	char    cmd[GP_CMD_SIZE] ;

	sprintf(cmd, "set xlabel \"%s\"", label) ;
	gnuplot_cmd(h, cmd) ;
	return ;
}


/*-------------------------------------------------------------------------*/
/**
  @brief	Sets the y label of a gnuplot session.
  @param	h Gnuplot session control handle.
  @param	label Character string to use for Y label.
  @return	void

  Sets the y label for a gnuplot session.
 */
/*--------------------------------------------------------------------------*/

void gnuplot_set_ylabel(gnuplot_ctrl * h, char * label)
{
	char    cmd[GP_CMD_SIZE] ;

	sprintf(cmd, "set ylabel \"%s\"", label) ;
	gnuplot_cmd(h, cmd) ;
	return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief	Sets the z label of a gnuplot session.
  @param	h Gnuplot session control handle.
  @param	label Character string to use for Z label.
  @return	void

  Sets the z label for a gnuplot session.  (Added by Robert Bradley 12 October 2004)
 */
/*--------------------------------------------------------------------------*/

void gnuplot_set_zlabel(gnuplot_ctrl * h, char * label)
{
	char    cmd[GP_CMD_SIZE] ;

	sprintf(cmd, "set zlabel \"%s\"", label) ;
	gnuplot_cmd(h, cmd) ;
	return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief	Resets a gnuplot session (next plot will erase previous ones).
  @param	h Gnuplot session control handle.
  @return	void

  Resets a gnuplot session, i.e. the next plot will erase all previous
  ones.
 */
/*--------------------------------------------------------------------------*/

void gnuplot_resetplot(gnuplot_ctrl * h)
{
	int     i ;
	if (h->ntmp) {
		for (i=0 ; i<h->ntmp ; i++) {
			remove(h->to_delete[i]) ;
		}
	}
	h->ntmp = 0 ;
	h->nplots = 0 ;
	return ;
}



/*-------------------------------------------------------------------------*/
/**
  @brief	Plots a 2d graph from a list of doubles.
  @param	handle	Gnuplot session control handle.
  @param	d		Array of doubles.
  @param	n		Number of values in the passed array.
  @param	title	Title of the plot.
  @return	void

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

void gnuplot_plot_x(
		gnuplot_ctrl    *   handle,
		double          *   d,
		int                 n,
		char            *   title
)
{
	int     i ;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;


	if (handle==NULL || d==NULL || (n<1)) return ;

	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return ;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return ;
	}

	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;
	/* Write data to this file  */
	for (i=0 ; i<n ; i++) {
		sprintf(line, "%g\n", d[i]);
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	if (handle->nplots > 0) {
		strcpy(cmd, "replot") ;
	} else {
		strcpy(cmd, "plot") ;
	}

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return ;
}



/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a 2d graph from a list of points.
  @param	handle		Gnuplot session control handle.
  @param	x			Pointer to a list of x coordinates.
  @param	y			Pointer to a list of y coordinates.
  @param	n			Number of doubles in x (assumed the same as in y).
  @param	title		Title of the plot.
  @return	void

  Plots out a 2d graph from a list of points. Provide points through a list
  of x and a list of y coordinates. Both provided arrays are assumed to
  contain the same number of values.

  @code
    gnuplot_ctrl    *h ;
	double			x[50] ;
	double			y[50] ;
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
		double			*	x,
		double			*	y,
		int                 n,
		char            *   title
)
{
	int     i ;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || x==NULL || y==NULL || (n<1)) return ;

	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return ;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return ;
	}

	//	printf("Use handle %i with name %s\n", handle->ntmp, name);

	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */
	for (i=0 ; i<n; i++) {
		//		printf("Print %i\n", i);
		sprintf(line, "%g %g\n", x[i], y[i]) ;
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	if (handle->nplots > 0) {
		strcpy(cmd, "replot") ;
	} else {
		strcpy(cmd, "plot") ;
	}

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return ;
}


void gnuplot_plot_xy_N(
		gnuplot_ctrl    *   handle,
		gnuplot_data	*   data,
		int					plots
)
{
	int     i, f;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;
	int		f_fd;

	if (handle==NULL || data ==NULL || (plots<1)) return ;

	// Set first file handle
	f_fd = handle->ntmp;

	// Open as many temporary files as the number of lines we have to plot
	for (f = 0; f < plots; f++) {

		if ((data[f].n < 1) || data[f].x == NULL || data[f].y == NULL) continue;

		if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
			fprintf(stderr,
					"maximum # of temporary files reached (%d): cannot open more",
					GP_MAX_TMP_FILES) ;
			return ;
		}

		// Open temporary file for output
		sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
		if ((tmpfd=mkstemp(name))==-1) {
			fprintf(stderr,"cannot create temporary file: exiting plot") ;
			return ;
		}

		// Store file name in array for future deletion
		strcpy(handle->to_delete[handle->ntmp], name) ;
		handle->ntmp++ ;

		// Write data to this file
		for (i=0 ; i<data[f].n; i++) {
			sprintf(line, "%g %g\n", data[f].x[i], data[f].y[i]) ;
			if (write(tmpfd, line, strlen(line)) < 0) {
				fprintf(stderr, "Couldn't write to temporary file\n") ;
			}
		}
		close(tmpfd) ;
	}

	// Command to "plot"
	sprintf(line, "plot \"%s\" title \"%s\" with %s", handle->to_delete[f_fd],
			data[f_fd].title, data[f_fd].pstyle) ;

	for (f = 1; f < plots; f++) {
		sprintf(line, "%s, \"%s\" title \"%s\" with %s", line, handle->to_delete[f_fd+f],
				data[f_fd+f].title, data[f_fd+f].pstyle) ;
	}
	printf("Plot command: %s\n", line);

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots += plots;
	return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a 3d graph from a list of points.
  @param	handle		Gnuplot session control handle.
  @param	x			Pointer to a list of x coordinates.
  @param	y			Pointer to a list of y coordinates.
  @param	z			Pointer to a list of z coordinates.
  @param	n			Number of doubles in x (y and z must be the the same).
  @param	title		Title of the plot.
  @return	void

  gnuplot_splot(handle,x,y,z,n,title);
  Based on gnuplot_plot_xy, modifications by Robert Bradley 12/10/2004

  Plots a 3d graph from a list of points, passed in the form of three arrays x, y and z.  All arrays are assumed to
  contain the same number of values.

  @code
    gnuplot_ctrl    *h ;
	double x[50] ; y[50] ; z[50];
    int i ;

    h = gnuplot_init() ;
    for (i=0 ; i<50 ; i++) {
        x[i] = (double)(i)/10.0 ;
        y[i] = x[i] * x[i] ;
        z[i] = x[i] * x[i]/2.0;
    }
    gnuplot_splot(h, x, y, z, 50, "parabola") ;
    sleep(2) ;
    gnuplot_close(h) ;
  @endcode
 */
/*--------------------------------------------------------------------------*/

int gnuplot_splot(gnuplot_ctrl *handle, double *x, double *y, double *z, int n, char *title) {
	int i;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || x==NULL || y==NULL || (n<1)) return 1;
	//    if (handle->nplots > 0)
	//      return 1;
	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return 1;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return 1;
	}
	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */
	for (i=0 ; i<n; i++) {
		sprintf(line, "%g %g %g\n", x[i], y[i], z[i]) ;
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	strcpy(cmd, "splot") ;

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return 0;
}

/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a 3d graph from a grid of points.
  @param	handle		Gnuplot session control handle.
  @param	points		Pointer to a grid of points (rows,cols).
  @param	rows       	Number of rows (y points).
  @param	cols       	Number of columns (x points).
  @param	title		Title of the plot.
  @return	void

  gnuplot_splot_grid(handle,(double*) points,rows,cols,title);
  Based on gnuplot_splot, modifications by Robert Bradley 2/4/2006

  Plots a 3d graph from a grid of points, passed in the form of a 2D array [x,y].
 */
/*--------------------------------------------------------------------------*/

int gnuplot_splot_grid(gnuplot_ctrl *handle, double *points, int rows, int cols, char *title) {
	int i;
	int j;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || points==NULL || (rows<1) || (cols<1)) return 1;
	if (handle->nplots > 0)
		return 1;
	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return 1;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return 1;
	}
	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */
	for (i=0 ; i<rows; i++) {
		for (j=0;j<cols;j++) {
			sprintf(line, "%d %d %g\n", i,j,points[i*cols+j]) ;
			if (write(tmpfd, line, strlen(line)) < 0) {
				fprintf(stderr,"couldn't write") ;
			}
		}
		strcpy(line,"\n");
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	strcpy(cmd, "splot") ;

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return 0;
}

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

int gnuplot_contour_plot(gnuplot_ctrl *handle, double *x, double *y, double *z, int nx, int ny, char *title) {
	int i,j;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || x==NULL || y==NULL || (nx<1) || (ny<1)) return 1;
	if (handle->nplots > 0)
		return 1;
	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return 1;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return 1;
	}
	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */

	for (i=0 ; i<nx; i++) {
		for (j=0 ; j<ny; j++) {
			sprintf(line, "%g %g %g\n", x[nx*i+j], y[nx*i+j], z[nx*i+j]) ;
			if (write(tmpfd, line, strlen(line)) < 0) {
				fprintf(stderr,"couldn't write") ;
			}
		}
		sprintf(line, "\n") ;
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	gnuplot_cmd(handle,"unset surface");
	gnuplot_cmd(handle,"set contour base");
	gnuplot_cmd(handle,"set view map");
	gnuplot_cmd(handle,"set view 0,0");
	strcpy(cmd, "splot") ;

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return 0;
}

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
		char *title) {
	int i;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || getPoint==NULL || (n<1)) return 1;
	if (handle->nplots > 0)
		return 1;
	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return 1;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return 1;
	}
	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */
	gnuplot_point point;
	for (i=0;i<n;i++) {
		getPoint(obj,&point,i,n);
		sprintf(line, "%g %g %g\n", point.x, point.y, point.z) ;
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	strcpy(cmd, "splot") ;

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return 0;
}

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

For example (where points is an array of points in this case):

void PlotPoint(void *obj,gnuplot_point *point,int i, int n) {
    Point *p=(Point*) obj;
    point->x=p[i].x;
    point->y=p[i].y;
}

int main(int argc, char *argv[]) {
...
gnuplot_plot_obj_xy(gp,points,PlotPoint,pCount,"Points");
...
}

Alternately, PlotPoint could return values based on a complex formula and many
sources of information.  For example, it could be used to perform a Discrete
Fourier Transform on an array of complex numbers, calculating one transformed
point per call.

Note: you should always explicitly set all the relevant parts of the struct.
However, you may ignore the z component for 2D plots.
 */

int gnuplot_plot_obj_xy(
		gnuplot_ctrl *handle,
		void *obj,
		void (*getPoint)(void*,gnuplot_point*,int,int),
		int n,
		char *title
)
{
	int     i ;
	int		tmpfd ;
	char    name[128] ;
	char    cmd[GP_CMD_SIZE] ;
	char    line[GP_CMD_SIZE] ;

	if (handle==NULL || getPoint==NULL || (n<1)) return 1;

	/* Open one more temporary file? */
	if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
		fprintf(stderr,
				"maximum # of temporary files reached (%d): cannot open more",
				GP_MAX_TMP_FILES) ;
		return 1;
	}

	/* Open temporary file for output   */
	sprintf(name, GNUPLOT_TEMPFILE, P_tmpdir);
	if ((tmpfd=mkstemp(name))==-1) {
		fprintf(stderr,"cannot create temporary file: exiting plot") ;
		return 1;
	}
	/* Store file name in array for future deletion */
	strcpy(handle->to_delete[handle->ntmp], name) ;
	handle->ntmp ++ ;

	/* Write data to this file  */
	gnuplot_point point;
	for (i=0 ; i<n; i++) {
		getPoint(obj,&point,i,n);
		sprintf(line, "%g %g\n", point.x, point.y) ;
		if (write(tmpfd, line, strlen(line)) < 0) {
			fprintf(stderr,"couldn't write") ;
		}
	}
	close(tmpfd) ;

	/* Command to be sent to gnuplot    */
	if (handle->nplots > 0) {
		strcpy(cmd, "replot") ;
	} else {
		strcpy(cmd, "plot") ;
	}

	if (title == NULL) {
		sprintf(line, "%s \"%s\" with %s", cmd, name, handle->pstyle) ;
	} else {
		sprintf(line, "%s \"%s\" title \"%s\" with %s", cmd, name,
				title, handle->pstyle) ;
	}

	/* send command to gnuplot  */
	gnuplot_cmd(handle, line) ;
	handle->nplots++ ;
	return 0;
}

/*-------------------------------------------------------------------------*/
/**
  @brief	Open a new session, plot a signal, close the session.
  @param	title	Plot title
  @param	style	Plot style
  @param	label_x	Label for X
  @param	label_y	Label for Y
  @param	x		Array of X coordinates
  @param	y		Array of Y coordinates (can be NULL)
  @param	n		Number of values in x and y.
  @return

  This function opens a new gnuplot session, plots the provided
  signal as an X or XY signal depending on a provided y, waits for
  a carriage return on stdin and closes the session.

  It is Ok to provide an empty title, empty style, or empty labels for
  X and Y. Defaults are provided in this case.
 */
/*--------------------------------------------------------------------------*/

void gnuplot_plot_once(
		char	*	title,
		char	*	style,
		char	*	label_x,
		char	*	label_y,
		double	*	x,
		double	*	y,
		int			n
)
{
	gnuplot_ctrl	*	handle ;

	if (x==NULL || n<1) return ;

	if ((handle = gnuplot_init()) == NULL) return ;
	if (style!=NULL) {
		gnuplot_setstyle(handle, style);
	} else {
		gnuplot_setstyle(handle, "lines");
	}
	if (label_x!=NULL) {
		gnuplot_set_xlabel(handle, label_x);
	} else {
		gnuplot_set_xlabel(handle, "X");
	}
	if (label_y!=NULL) {
		gnuplot_set_ylabel(handle, label_y);
	} else {
		gnuplot_set_ylabel(handle, "Y");
	}
	if (y==NULL) {
		gnuplot_plot_x(handle, x, n, title);
	} else {
		gnuplot_plot_xy(handle, x, y, n, title);
	}
	printf("press ENTER to continue\n");
	while (getchar()!='\n') {}
	gnuplot_close(handle);
	return ;
}




/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a slope on a gnuplot session.
  @param	handle		Gnuplot session control handle.
  @param	a			Slope.
  @param	b			Intercept.
  @param	title		Title of the plot.
  @return	void
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
)
{
	char    stitle[GP_TITLE_SIZE] ;
	char    cmd[GP_CMD_SIZE] ;

	if (title == NULL) {
		strcpy(stitle, "no title") ;
	} else {
		strcpy(stitle, title) ;
	}

	if (handle->nplots > 0) {
		sprintf(cmd, "replot %g * x + %g title \"%s\" with %s",
				a, b, title, handle->pstyle) ;
	} else {
		sprintf(cmd, "plot %g * x + %g title \"%s\" with %s",
				a, b, title, handle->pstyle) ;
	}
	gnuplot_cmd(handle, cmd) ;
	handle->nplots++ ;
	return ;
}


/*-------------------------------------------------------------------------*/
/**
  @brief	Plot a curve of given equation y=f(x).
  @param	h			Gnuplot session control handle.
  @param	equation	Equation to plot.
  @param	title		Title of the plot.
  @return	void

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

void gnuplot_plot_equation(
		gnuplot_ctrl    *   h,
		char            *   equation,
		char            *   title
)
{
	char    cmd[GP_CMD_SIZE];
	char    plot_str[GP_EQ_SIZE] ;
	char    title_str[GP_TITLE_SIZE] ;

	if (title == NULL) {
		strcpy(title_str, "no title") ;
	} else {
		strcpy(title_str, title) ;
	}
	if (h->nplots > 0) {
		strcpy(plot_str, "replot") ;
	} else {
		strcpy(plot_str, "plot") ;
	}

	sprintf(cmd, "%s %s title \"%s\" with %s", 
			plot_str, equation, title_str, h->pstyle) ;
	gnuplot_cmd(h, cmd) ;
	h->nplots++ ;
	return ;
}

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
void gnuplot_hardcopy(
		gnuplot_ctrl * h, 
		char * filename
)
{
	gnuplot_cmd(h, "set terminal postscript");

	gnuplot_cmd(h, "set output \"%s\"", filename);
	gnuplot_cmd(h, "replot");

	gnuplot_cmd(h, "set terminal %s",h->term);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Save a graph in colour as a postscript file on the hard disk
  @param    h           Gnuplot session control handle.
  @param    equation    Equation to plot.
  @param    title       Title of the plot.
  @return   void

  Added by Robert Bradley on 12th october 2004

  This function is identical in syntax (and implementation) to gnuplot_hardcopy, but the resulting
  PostScript file retains colour information.

 */
/*--------------------------------------------------------------------------*/

void gnuplot_hardcopy_colour(
		gnuplot_ctrl * h,
		char * filename
)
{
	gnuplot_cmd(h, "set terminal postscript enhanced color");

	gnuplot_cmd(h, "set output \"%s\"", filename);
	gnuplot_cmd(h, "replot");

	gnuplot_cmd(h, "set terminal %s",h->term);
}

#endif
