#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: drawopenx.c,v 1.2 1999/01/12 23:16:44 bsmith Exp bsmith $";
#endif
/*
    Defines the operations for the X Draw implementation.
*/

#include "src/sys/src/draw/impls/x/ximpl.h"         /*I  "petsc.h" I*/

#undef __FUNC__  
#define __FUNC__ "DrawOpenX" 
/*@C
   DrawOpenX - Opens an X-window for use with the Draw routines.

   Collective on MPI_Comm

   Input Parameters:
+  comm - the communicator that will share X-window
.  display - the X display on which to open, or null for the local machine
.  title - the title to put in the title bar, or null for no title
.  x, y - the screen coordinates of the upper left corner of window
          may use PETSC_DECIDE for these two arguments, then PETSc places the 
          window
-  w, h - the screen width and height in pixels

   Output Parameters:
.  ctx - the drawing context.

   Options Database Keys:
+  -nox - Disables all x-windows output
.  -display <name> - Sets name of machine for the X display
.  -draw_pause <pause> - Sets time (in seconds) that the
       program pauses after DrawPause() has been called
       (0 is default, -1 implies until user input).
.  -draw_x_shared_colormap - Causes PETSc to use a shared
       colormap. By default PETSc creates a seperate color 
       for its windows, you must put the mouse into the graphics 
       window to see  the correct colors. This options forces
       PETSc to use the default colormap which will usually result
       in bad contour plots.
.  -draw_double_buffer - Uses double buffering for smooth animation.
-  -geometry - Indicates location and size of window

   Note:
   When finished with the drawing context, it should be destroyed
   with DrawDestroy().

   Note for Fortran Programmers:
   Whenever indicating null character data in a Fortran code,
   PETSC_NULL_CHARACTER must be employed; using PETSC_NULL is not
   correct for character data!  Thus, PETSC_NULL_CHARACTER can be
   used for the display and title input parameters.

.keywords: draw, open, x

.seealso: DrawSynchronizedFlush(), DrawDestroy()
@*/
int DrawOpenX(MPI_Comm comm,const char display[],const char title[],int x,int y,int w,int h,Draw* inctx)
{
  int  flg,ierr;

  PetscFunctionBegin;
  ierr = DrawCreate(comm,display,title,x,y,w,h,inctx);CHKERRQ(ierr);
  ierr = OptionsHasName(PETSC_NULL,"-nox",&flg); CHKERRQ(ierr);
  if (flg) {
    ierr = DrawSetType(*inctx,DRAW_NULL);CHKERRQ(ierr);
  } else {
#if !defined(HAVE_X11)
    (*PetscErrorPrintf)("PETSc installed without X windows on this machine\nproceeding without graphics\n");
    ierr = DrawSetType(*inctx,DRAW_NULL);CHKERRQ(ierr);
#else
    ierr = DrawSetType(*inctx,DRAW_X);CHKERRQ(ierr);
#endif
  }
  PetscFunctionReturn(0);
}









