#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: drawregall.c,v 1.3 1999/01/13 22:34:25 bsmith Exp bsmith $";
#endif
/*
       Provides the calling sequences for all the basic Draw routines.
*/
#include "src/sys/src/draw/drawimpl.h"  /*I "draw.h" I*/

EXTERN_C_BEGIN
extern int DrawCreate_X(Draw);
extern int DrawCreate_Null(Draw);
EXTERN_C_END
  
/*
    This is used by DrawSetType() to make sure that at least one 
    DrawRegisterAll() is called. In general, if there is more than one
    DLL, then DrawRegisterAll() may be called several times.
*/
extern int DrawRegisterAllCalled;

#undef __FUNC__  
#define __FUNC__ "DrawRegisterAll"
/*@C
  DrawRegisterAll - Registers all of the graphics methods in the Draw package.

  Not Collective

.keywords: Draw, register, all

.seealso:  DrawRegisterDestroy()
@*/
int DrawRegisterAll(char *path)
{
  int ierr;

  PetscFunctionBegin;
  DrawRegisterAllCalled = 1;
  
#if defined(HAVE_X11)
  ierr = DrawRegister(DRAW_X,     path,"DrawCreate_X",     DrawCreate_X);CHKERRQ(ierr);
#endif
  ierr = DrawRegister(DRAW_NULL,  path,"DrawCreate_Null",  DrawCreate_Null);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

