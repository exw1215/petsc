
#ifndef lint
static char vcid[] = "$Id: zmat.c,v 1.2 1995/09/04 17:18:58 bsmith Exp bsmith $";
#endif

#include "zpetsc.h"
#include "mat.h"
#include "pinclude/petscfix.h"

#ifdef FORTRANCAPS
#define matgetreordering_             MATGETREORDERING
#define matreorderingregisterall_     MATREORDERINGREGISTERALL
#define matdestroy_                   MATDESTROY
#define matcreatempiaij_              MATCREATEMPIAIJ
#define matcreateseqaij_              MATCREATESEQAIJ
#define matgetname_                   MATGETNAME
#define matcreate_                    MATCREATE
#define matshellcreate_               MATSHELLCREATE
#define matshellsetmulttransadd_      MATSHELLSETMULTTRANSADD
#define matshellsetdestroy_           MATSHELLSETDESTROY
#define matshellsetmult_              MATSHELLSETMULT
#define matreorderingregisterdestroy_ MATREORDERINGREGISTERDESTROY
#define matcreatempirow_              MATCREATEMPIROW
#define matcreateseqrow_              MATCREATESEQROW
#define matcreatempirowbs_            MATCREATEMPIROWBS
#define matcreateseqbdiag_            MATCREATESEQBDIAG
#define matcreatempibdiag_            MATCREATEMPIBDIAG
#define matcreateseqdense_            MATCREATESEQDENSE
#define matconvert_                   MATCONVERT
#define matreorderingregister_        MATREORDERINGREGISTER
#elif !defined(FORTRANUNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matreorderingregisterall_     matreorderingregisterall
#define matdestroy_                   matdestroy
#define matcreatempiaij_              matcreatempiaij
#define matcreateseqaij_              matcreateseqaij
#define matgetname_                   matgetname
#define matcreate_                    matcreate
#define matshellsetmult_              matshellsetmult
#define matshellcreate_               matshellcreate
#define matshellsetmulttransadd_      matshellsetmulttransadd
#define matshellsetdestroy_           matshellsetdestroy
#define matreorderingregisterdestroy_ matreorderingregisterdestroy
#define matgetreordering_             matgetreordering
#define matcreatempirow_              matcreatempirow
#define matcreateseqrow_              matcreateseqrow
#define matcreatempirowbs_            matcreatempirowbs
#define matcreateseqbdiag_            matcreateseqbdiag
#define matcreatempibdiag_            matcreatempibdiag
#define matcreateseqdense_            matcreateseqdense
#define matconvert_                   matconvert
#define matreorderingregister_        matreorderingregister
#endif

static void (*f5)(int*,int*,int*,int*,int*,int*);
static int ourorder(int* a,int* b,int* c,int *d, int* e)
{
  int ierr;
  (*f5)(a,b,c,d,e,&ierr); CHKERRQ(ierr);
  return 0;
}

void matreorderingregister(MatOrdering *name, char* sname,
            void (*order)(int*,int*,int*,int*,int*,int*),int *__ierr,int len)
{
  char *t;
  if (sname[len] != 0) {
    t = (char *) PETSCMALLOC( (len+1)*sizeof(char) ); 
    PetscStrncpy(t,sname,len);
    t[len] = 0;
  }
  else t = sname;
  f5 = order;

  *__ierr = MatReorderingRegister(*name,t,ourorder);
  if (t != sname) PETSCFREE(t);
}			       


void matconvert_(Mat mat,MatType *newtype,Mat *M, int *__ierr ){
  Mat mm;
  *__ierr = MatConvert(
	(Mat)MPIR_ToPointer( *(int*)(mat) ),*newtype,&mm);
  *(int*) M = MPIR_FromPointer(mm);
}

void matcreateseqdense_(MPI_Comm comm,int *m,int *n,Mat *newmat, 
                               int *__ierr ){
  Mat mm;
  *__ierr = MatCreateSeqDense(
	(MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

/* Fortran ignores diagv */
void matcreatempibdiag_(MPI_Comm comm,int *m,int *M,int *N,int *nd,int *nb,
                     int *diag,Scalar **diagv,Mat *newmat, int *__ierr ){
  Mat mm;
  *__ierr = MatCreateMPIBDiag((MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),
                              *m,*M,*N,*nd,*nb,diag,0,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

/* Fortran ignores diagv */
void matcreateseqbdiag_(MPI_Comm comm,int *m,int *n,int *nd,int *nb,
                        int *diag,Scalar **diagv,Mat *newmat, int *__ierr ){
  Mat mm;
  *__ierr = MatCreateSeqBDiag(
    (MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,*nd,*nb,diag,0,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

/*  Fortran cannot pass in procinfo, hence ignored */
void matcreatempirowbs_(MPI_Comm comm,int *m,int *M,int *nz,int *nnz,
                       void *procinfo,Mat *newmat, int *__ierr ){
  Mat mm;
/*
   The p1 is because Fortran cannot pass in a null pointer 
*/
  int *p1 = 0;
  if (*nnz) p1 = nnz; 
  *__ierr = MatCreateMPIRowbs((MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),
                               *m,*M,*nz,p1,0,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

void matcreateseqrow_(MPI_Comm comm,int *m,int *n,int *nz,
                           int *nnz,Mat *newmat, int *__ierr ){
  Mat mm;
/*
   The p1 is because Fortran cannot pass in a null pointer 
*/
  int *p1 = 0;
  if (*nnz) p1 = nnz; 
  *__ierr = MatCreateSeqRow(
	(MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,*nz,p1,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}
void matcreatempirow_(MPI_Comm comm,int *m,int *n,int *M,int *N,
       int *d_nz,int *d_nnz,int *o_nz,int *o_nnz,Mat *newmat, int *__ierr ){
  Mat mm;
/*
   The p1, p2 are because Fortran cannot pass in a null pointer 
*/
  int *p1 = 0, *p2 = 0;
  if (*d_nnz) p1 = d_nnz; 
  if (*o_nnz) p2 = o_nnz; 
  *__ierr = MatCreateMPIRow((MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),
                            *m,*n,*M,*N,*d_nz,p1,*o_nz,p2,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

void matgetreordering_(Mat mat,MatOrdering *type,IS *rperm,IS *cperm, 
                       int *__ierr ){
  IS i1,i2;
  *__ierr = MatGetReordering((Mat)MPIR_ToPointer(*(int*)(mat)),*type,&i1,&i2);
  *(int*) rperm = MPIR_FromPointer(i1);
  *(int*) cperm = MPIR_FromPointer(i2);
}

void matreorderingregisterdestroy_(int *__ierr){
  *__ierr = MatReorderingRegisterDestroy();
}


void matshellcreate_(MPI_Comm comm,int *m,int *n,void *ctx,Mat *mat, 
                     int *__ierr ){
  Mat mm;
  *__ierr = MatShellCreate((MPI_Comm)MPIR_ToPointer(*(int*)(comm)),
                           *m,*n,ctx,&mm);
  *(int*) mat = MPIR_FromPointer(mm);
}

static int (*f2)(void*,int*,int*,int*);
static int ourmatshellmult(void *ctx,Vec x,Vec f)
{
  int ierr = 0, s2, s3;
  s2 = MPIR_FromPointer(x);
  s3 = MPIR_FromPointer(f);
  (*f2)(ctx,&s2,&s3,&ierr); CHKERRQ(ierr);
  MPIR_RmPointer(s2);
  MPIR_RmPointer(s3);
  return 0;
}
void matshellsetmult_(Mat mat,int (*mult)(void*,int*,int*,int*), int *__ierr ){
  f2 = mult;
  *__ierr = MatShellSetMult(
	(Mat)MPIR_ToPointer( *(int*)(mat) ),ourmatshellmult);
}
static int (*f1)(void*,int*,int*,int*,int*);
static int ourmatshellmulttransadd(void *ctx,Vec x,Vec f,Vec y)
{
  int ierr = 0, s2, s3,s4;
  s2 = MPIR_FromPointer(x);
  s3 = MPIR_FromPointer(f);
  s4 = MPIR_FromPointer(y);
  (*f1)(ctx,&s2,&s3,&s4,&ierr); CHKERRQ(ierr);
  MPIR_RmPointer(s2);
  MPIR_RmPointer(s3);
  MPIR_RmPointer(s4);
  return 0;
}

void matshellsetmulttransadd_(Mat mat,int (*mult)(void*,int*,int*,int*,int*), 
                              int *__ierr ){
  f1 = mult;
  *__ierr = MatShellSetMultTransAdd(
	(Mat)MPIR_ToPointer( *(int*)(mat) ),ourmatshellmulttransadd);
}

static int (*f3)(void*,int*);
static int ourmatshelldestroy(void *ctx)
{
  int ierr = 0;
  (*f3)(ctx,&ierr); CHKERRQ(ierr);
  return 0;
}
void matshellsetdestroy_(Mat mat,int (*destroy)(void*,int*), int *__ierr ){
  f3 = destroy;
  *__ierr = MatShellSetDestroy(
	(Mat)MPIR_ToPointer( *(int*)(mat) ),ourmatshelldestroy);
}

void matgetname_(Mat mm,char *name,int *__ierr,int len)
{
  char *tname;
  *__ierr = MatGetName((Mat)MPIR_ToPointer(*(int*)mm),&tname);
  PetscStrncpy(name,tname,len);
}

void matcreate_(MPI_Comm comm,int *m,int *n,Mat *V, int *__ierr )
{
  Mat mm;
  *__ierr = MatCreate(
	(MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,&mm);
  *(int*) V = MPIR_FromPointer(mm);
}

void matcreateseqaij_(MPI_Comm comm,int *m,int *n,int *nz,
                           int *nnz,Mat *newmat, int *__ierr )
{
  Mat mm;
/*
   The p1 is because Fortran cannot pass in a null pointer 
*/
  int *p1 = 0;
  if (*nnz) p1 = nnz; 
  *__ierr = MatCreateSeqAIJ(
	(MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,*nz,p1,&mm);
  *(int*) newmat = MPIR_FromPointer(mm);
}

void matdestroy_(Mat mat, int *__ierr )
{
  *__ierr = MatDestroy((Mat)MPIR_ToPointer( *(int*)(mat) ));
   MPIR_RmPointer(*(int*)(mat)); 
}

void matreorderingregisterall_(int *__ierr)
{
  *__ierr = MatReorderingRegisterAll();
}

void matcreatempiaij_(MPI_Comm comm,int *m,int *n,int *M,int *N,
         int *d_nz,int *d_nnz,int *o_nz,int *o_nnz,Mat *newmat, int *__ierr ){
  Mat mm;
/*
   The p1 and p2 are because Fortran cannot pass in a null pointer 
*/
  int *p1 = 0,*p2 = 0;
  if (*d_nnz) p1 = d_nnz; 
  if (*o_nnz) p2 = o_nnz; 
*__ierr = MatCreateMPIAIJ(
	(MPI_Comm)MPIR_ToPointer( *(int*)(comm) ),*m,*n,*M,*N,*d_nz,p1,
         *o_nz,p2,&mm);
  *(int*)newmat = MPIR_FromPointer(mm);
}
