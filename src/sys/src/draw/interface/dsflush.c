#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: dsflush.c,v 1.18 1999/01/12 23:16:28 bsmith Exp bsmith $";
#endif
/*
       Provides the calling sequences for all the basic Draw routines.
*/
#include "src/sys/src/draw/drawimpl.h"  /*I "draw.h" I*/

#undef __FUNC__  
#define __FUNC__ "DrawSynchronizedFlush" 
/*@
   DrawSynchronizedFlush - Flushes graphical output. This waits until all 
   processors have arrived and flushed, then does a global flush.
   This is usually done to change the frame for double buffered graphics.

   Collective on Draw

   Input Parameters:
.  draw - the drawing context

.keywords: draw, sync, flush

@*/
int DrawSynchronizedFlush(Draw draw)
{
  int ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,DRAW_COOKIE);
  if (draw->ops->synchronizedflush) {
    ierr = (*draw->ops->synchronizedflush)(draw);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
