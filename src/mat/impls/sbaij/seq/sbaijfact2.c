#define PETSCMAT_DLL

/*
    Factorization code for SBAIJ format. 
*/

#include "src/mat/impls/sbaij/seq/sbaij.h"
#include "src/mat/impls/baij/seq/baij.h"
#include "src/inline/ilu.h"
#include "src/inline/dot.h"

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_N"
PetscErrorCode MatSolve_SeqSBAIJ_N(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ    *a=(Mat_SeqSBAIJ*)A->data;
  IS              isrow=a->row;
  PetscInt        mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode  ierr;
  PetscInt        nz,*vj,k,*r,idx,k1;
  PetscInt        bs=A->rmap.bs,bs2 = a->bs2;
  MatScalar       *aa=a->a,*v,*diag;
  PetscScalar     *x,*xk,*xj,*b,*xk_tmp,*t;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);
  ierr = PetscMalloc(bs*sizeof(PetscScalar),&xk_tmp);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  xk = t; 
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = bs*r[k];
    for (k1=0; k1<bs; k1++) *xk++ = b[idx+k1];
  }
  for (k=0; k<mbs; k++){
    v  = aa + bs2*ai[k]; 
    xk = t + k*bs;      /* Dk*xk = k-th block of x */
    ierr = PetscMemcpy(xk_tmp,xk,bs*sizeof(PetscScalar));CHKERRQ(ierr); /* xk_tmp <- xk */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xj = t + (*vj)*bs;  /* *vj-th block of x, *vj>k */
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      Kernel_v_gets_v_plus_Atranspose_times_w(bs,xj,v,xk_tmp); /* xj <- xj + v^t * xk */
      vj++; xj = t + (*vj)*bs;
      v += bs2;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*bs2;                            /* ptr to inv(Dk) */
    Kernel_w_gets_A_times_v(bs,xk_tmp,diag,xk); /* xk <- diag * xk */
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + bs2*ai[k]; 
    xk = t + k*bs;        /* xk */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xj = t + (*vj)*bs;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      Kernel_v_gets_v_plus_A_times_w(bs,xk,v,xj); /* xk <- xk + v*xj */
      vj++; 
      v += bs2; xj = t + (*vj)*bs;
    }
    idx   = bs*r[k];
    for (k1=0; k1<bs; k1++) x[idx+k1] = *xk++;
  }

  ierr = PetscFree(xk_tmp);CHKERRQ(ierr);
  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(bs2*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}     

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_N_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_N_NaturalOrdering(Mat A,Vec bb,Vec xx) 
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscInt       nz,*vj,k;
  PetscInt       bs=A->rmap.bs,bs2 = a->bs2;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*xk,*xj,*b,*xk_tmp;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);

  ierr = PetscMalloc(bs*sizeof(PetscScalar),&xk_tmp);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,bs*mbs*sizeof(PetscScalar));CHKERRQ(ierr); /* x <- b */
  for (k=0; k<mbs; k++){
    v  = aa + bs2*ai[k]; 
    xk = x + k*bs;      /* Dk*xk = k-th block of x */
    ierr = PetscMemcpy(xk_tmp,xk,bs*sizeof(PetscScalar));CHKERRQ(ierr); /* xk_tmp <- xk */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xj = x + (*vj)*bs;  /* *vj-th block of x, *vj>k */
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      Kernel_v_gets_v_plus_Atranspose_times_w(bs,xj,v,xk_tmp); /* xj <- xj + v^t * xk */
      vj++; xj = x + (*vj)*bs;
      v += bs2;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*bs2;                            /* ptr to inv(Dk) */
    Kernel_w_gets_A_times_v(bs,xk_tmp,diag,xk); /* xk <- diag * xk */
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + bs2*ai[k]; 
    xk = x + k*bs;        /* xk */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xj = x + (*vj)*bs;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      Kernel_v_gets_v_plus_A_times_w(bs,xk,v,xj); /* xk <- xk + v*xj */
      vj++; 
      v += bs2; xj = x + (*vj)*bs;
    }
  }

  ierr = PetscFree(xk_tmp);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(bs2*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}     

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_7"
PetscErrorCode MatSolve_SeqSBAIJ_7(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,*r,idx;  
  MatScalar      *aa=a->a,*v,*d;
  PetscScalar    *x,*b,x0,x1,x2,x3,x4,x5,x6,*t,*tp;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  tp = t; 
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = 7*r[k];
    tp[0] = b[idx];
    tp[1] = b[idx+1];
    tp[2] = b[idx+2];
    tp[3] = b[idx+3];
    tp[4] = b[idx+4];
    tp[5] = b[idx+5]; 
    tp[6] = b[idx+6];
    tp += 7; 
  }
  
  for (k=0; k<mbs; k++){
    v  = aa + 49*ai[k]; 
    vj = aj + ai[k]; 
    tp = t + k*7;
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4]; x5=tp[5]; x6=tp[6];
    nz = ai[k+1] - ai[k];  
    tp = t + (*vj)*7;
    while (nz--) {  
      tp[0]+=  v[0]*x0 +  v[1]*x1 +  v[2]*x2 + v[3]*x3 + v[4]*x4 + v[5]*x5 + v[6]*x6;
      tp[1]+=  v[7]*x0 +  v[8]*x1 +  v[9]*x2+ v[10]*x3+ v[11]*x4+ v[12]*x5+ v[13]*x6;
      tp[2]+= v[14]*x0 + v[15]*x1 + v[16]*x2+ v[17]*x3+ v[18]*x4+ v[19]*x5+ v[20]*x6;
      tp[3]+= v[21]*x0 + v[22]*x1 + v[23]*x2+ v[24]*x3+ v[25]*x4+ v[26]*x5+ v[27]*x6;
      tp[4]+= v[28]*x0 + v[29]*x1 + v[30]*x2+ v[31]*x3+ v[32]*x4+ v[33]*x5+ v[34]*x6;
      tp[5]+= v[35]*x0 + v[36]*x1 + v[37]*x2+ v[38]*x3+ v[39]*x4+ v[40]*x5+ v[41]*x6;
      tp[6]+= v[42]*x0 + v[43]*x1 + v[44]*x2+ v[45]*x3+ v[46]*x4+ v[47]*x5+ v[48]*x6;
      vj++; tp = t + (*vj)*7;
      v += 49;      
    }

    /* xk = inv(Dk)*(Dk*xk) */
    d  = aa+k*49;          /* ptr to inv(Dk) */
    tp    = t + k*7;
    tp[0] = d[0]*x0 + d[7]*x1 + d[14]*x2 + d[21]*x3 + d[28]*x4 + d[35]*x5 + d[42]*x6;
    tp[1] = d[1]*x0 + d[8]*x1 + d[15]*x2 + d[22]*x3 + d[29]*x4 + d[36]*x5 + d[43]*x6;
    tp[2] = d[2]*x0 + d[9]*x1 + d[16]*x2 + d[23]*x3 + d[30]*x4 + d[37]*x5 + d[44]*x6;
    tp[3] = d[3]*x0+ d[10]*x1 + d[17]*x2 + d[24]*x3 + d[31]*x4 + d[38]*x5 + d[45]*x6;
    tp[4] = d[4]*x0+ d[11]*x1 + d[18]*x2 + d[25]*x3 + d[32]*x4 + d[39]*x5 + d[46]*x6;
    tp[5] = d[5]*x0+ d[12]*x1 + d[19]*x2 + d[26]*x3 + d[33]*x4 + d[40]*x5 + d[47]*x6;
    tp[6] = d[6]*x0+ d[13]*x1 + d[20]*x2 + d[27]*x3 + d[34]*x4 + d[41]*x5 + d[48]*x6;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 49*ai[k]; 
    vj = aj + ai[k]; 
    tp    = t + k*7;    
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4]; x5=tp[5];  x6=tp[6]; /* xk */ 
    nz = ai[k+1] - ai[k]; 
  
    tp = t + (*vj)*7;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*tp[0] + v[7]*tp[1] + v[14]*tp[2] + v[21]*tp[3] + v[28]*tp[4] + v[35]*tp[5] + v[42]*tp[6];
      x1 += v[1]*tp[0] + v[8]*tp[1] + v[15]*tp[2] + v[22]*tp[3] + v[29]*tp[4] + v[36]*tp[5] + v[43]*tp[6];
      x2 += v[2]*tp[0] + v[9]*tp[1] + v[16]*tp[2] + v[23]*tp[3] + v[30]*tp[4] + v[37]*tp[5] + v[44]*tp[6];
      x3 += v[3]*tp[0]+ v[10]*tp[1] + v[17]*tp[2] + v[24]*tp[3] + v[31]*tp[4] + v[38]*tp[5] + v[45]*tp[6];
      x4 += v[4]*tp[0]+ v[11]*tp[1] + v[18]*tp[2] + v[25]*tp[3] + v[32]*tp[4] + v[39]*tp[5] + v[46]*tp[6];
      x5 += v[5]*tp[0]+ v[12]*tp[1] + v[19]*tp[2] + v[26]*tp[3] + v[33]*tp[4] + v[40]*tp[5] + v[47]*tp[6];
      x6 += v[6]*tp[0]+ v[13]*tp[1] + v[20]*tp[2] + v[27]*tp[3] + v[34]*tp[4] + v[41]*tp[5] + v[48]*tp[6];
      vj++; tp = t + (*vj)*7;
      v += 49;
    }
    tp    = t + k*7;    
    tp[0]=x0; tp[1]=x1; tp[2]=x2; tp[3]=x3; tp[4]=x4; tp[5]=x5; tp[6]=x6;
    idx   = 7*r[k];
    x[idx]     = x0;
    x[idx+1]   = x1;
    x[idx+2]   = x2;
    x[idx+3]   = x3;
    x[idx+4]   = x4;
    x[idx+5]   = x5;
    x[idx+6]   = x6;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(49*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_7_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_7_NaturalOrdering(Mat A,Vec bb,Vec xx) 
{ 
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*d;
  PetscScalar    *x,*xp,*b,x0,x1,x2,x3,x4,x5,x6;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  
  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,7*mbs*sizeof(PetscScalar));CHKERRQ(ierr); /* x <- b */
  for (k=0; k<mbs; k++){
    v  = aa + 49*ai[k]; 
    xp = x + k*7;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4]; x5=xp[5]; x6=xp[6]; /* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*7;
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      xp[0]+=  v[0]*x0 +  v[1]*x1 +  v[2]*x2 + v[3]*x3 + v[4]*x4 + v[5]*x5 + v[6]*x6;
      xp[1]+=  v[7]*x0 +  v[8]*x1 +  v[9]*x2+ v[10]*x3+ v[11]*x4+ v[12]*x5+ v[13]*x6;
      xp[2]+= v[14]*x0 + v[15]*x1 + v[16]*x2+ v[17]*x3+ v[18]*x4+ v[19]*x5+ v[20]*x6;
      xp[3]+= v[21]*x0 + v[22]*x1 + v[23]*x2+ v[24]*x3+ v[25]*x4+ v[26]*x5+ v[27]*x6;
      xp[4]+= v[28]*x0 + v[29]*x1 + v[30]*x2+ v[31]*x3+ v[32]*x4+ v[33]*x5+ v[34]*x6;
      xp[5]+= v[35]*x0 + v[36]*x1 + v[37]*x2+ v[38]*x3+ v[39]*x4+ v[40]*x5+ v[41]*x6;
      xp[6]+= v[42]*x0 + v[43]*x1 + v[44]*x2+ v[45]*x3+ v[46]*x4+ v[47]*x5+ v[48]*x6;
      vj++; xp = x + (*vj)*7;
      v += 49;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    d  = aa+k*49;          /* ptr to inv(Dk) */
    xp = x + k*7;
    xp[0] = d[0]*x0 + d[7]*x1 + d[14]*x2 + d[21]*x3 + d[28]*x4 + d[35]*x5 + d[42]*x6;
    xp[1] = d[1]*x0 + d[8]*x1 + d[15]*x2 + d[22]*x3 + d[29]*x4 + d[36]*x5 + d[43]*x6;
    xp[2] = d[2]*x0 + d[9]*x1 + d[16]*x2 + d[23]*x3 + d[30]*x4 + d[37]*x5 + d[44]*x6;
    xp[3] = d[3]*x0+ d[10]*x1 + d[17]*x2 + d[24]*x3 + d[31]*x4 + d[38]*x5 + d[45]*x6;
    xp[4] = d[4]*x0+ d[11]*x1 + d[18]*x2 + d[25]*x3 + d[32]*x4 + d[39]*x5 + d[46]*x6;
    xp[5] = d[5]*x0+ d[12]*x1 + d[19]*x2 + d[26]*x3 + d[33]*x4 + d[40]*x5 + d[47]*x6;
    xp[6] = d[6]*x0+ d[13]*x1 + d[20]*x2 + d[27]*x3 + d[34]*x4 + d[41]*x5 + d[48]*x6;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 49*ai[k]; 
    xp = x + k*7;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4]; x5=xp[5]; x6=xp[6]; /* xk */ 
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*7;
    while (nz--) {
      /* xk += U(k,:)*x(:) */   
      x0 += v[0]*xp[0] + v[7]*xp[1] + v[14]*xp[2] + v[21]*xp[3] + v[28]*xp[4] + v[35]*xp[5] + v[42]*xp[6];
      x1 += v[1]*xp[0] + v[8]*xp[1] + v[15]*xp[2] + v[22]*xp[3] + v[29]*xp[4] + v[36]*xp[5] + v[43]*xp[6];
      x2 += v[2]*xp[0] + v[9]*xp[1] + v[16]*xp[2] + v[23]*xp[3] + v[30]*xp[4] + v[37]*xp[5] + v[44]*xp[6];
      x3 += v[3]*xp[0]+ v[10]*xp[1] + v[17]*xp[2] + v[24]*xp[3] + v[31]*xp[4] + v[38]*xp[5] + v[45]*xp[6];
      x4 += v[4]*xp[0]+ v[11]*xp[1] + v[18]*xp[2] + v[25]*xp[3] + v[32]*xp[4] + v[39]*xp[5] + v[46]*xp[6];
      x5 += v[5]*xp[0]+ v[12]*xp[1] + v[19]*xp[2] + v[26]*xp[3] + v[33]*xp[4] + v[40]*xp[5] + v[47]*xp[6];
      x6 += v[6]*xp[0]+ v[13]*xp[1] + v[20]*xp[2] + v[27]*xp[3] + v[34]*xp[4] + v[41]*xp[5] + v[48]*xp[6];
      vj++; 
      v += 49; xp = x + (*vj)*7;
    }
    xp = x + k*7;
    xp[0]=x0; xp[1]=x1; xp[2]=x2; xp[3]=x3; xp[4]=x4; xp[5]=x5; xp[6]=x6;
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(49*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_6"
PetscErrorCode MatSolve_SeqSBAIJ_6(Mat A,Vec bb,Vec xx)
{  
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,*r,idx;  
  MatScalar      *aa=a->a,*v,*d;
  PetscScalar    *x,*b,x0,x1,x2,x3,x4,x5,*t,*tp;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  tp = t; 
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = 6*r[k];
    tp[0] = b[idx];
    tp[1] = b[idx+1];
    tp[2] = b[idx+2];
    tp[3] = b[idx+3];
    tp[4] = b[idx+4];
    tp[5] = b[idx+5];   
    tp += 6; 
  }
  
  for (k=0; k<mbs; k++){
    v  = aa + 36*ai[k]; 
    vj = aj + ai[k]; 
    tp = t + k*6;
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4]; x5=tp[5];
    nz = ai[k+1] - ai[k];  
    tp = t + (*vj)*6;
    while (nz--) {  
      tp[0] +=  v[0]*x0 +  v[1]*x1 +  v[2]*x2 + v[3]*x3 + v[4]*x4 + v[5]*x5;
      tp[1] +=  v[6]*x0 +  v[7]*x1 +  v[8]*x2 + v[9]*x3+ v[10]*x4+ v[11]*x5;
      tp[2] += v[12]*x0 + v[13]*x1 + v[14]*x2+ v[15]*x3+ v[16]*x4+ v[17]*x5;
      tp[3] += v[18]*x0 + v[19]*x1 + v[20]*x2+ v[21]*x3+ v[22]*x4+ v[23]*x5;
      tp[4] += v[24]*x0 + v[25]*x1 + v[26]*x2+ v[27]*x3+ v[28]*x4+ v[29]*x5;
      tp[5] += v[30]*x0 + v[31]*x1 + v[32]*x2+ v[33]*x3+ v[34]*x4+ v[35]*x5;
      vj++; tp = t + (*vj)*6;
      v += 36;      
    }

    /* xk = inv(Dk)*(Dk*xk) */
    d  = aa+k*36;          /* ptr to inv(Dk) */
    tp    = t + k*6;
    tp[0] = d[0]*x0 + d[6]*x1 + d[12]*x2 + d[18]*x3 + d[24]*x4 + d[30]*x5;
    tp[1] = d[1]*x0 + d[7]*x1 + d[13]*x2 + d[19]*x3 + d[25]*x4 + d[31]*x5;
    tp[2] = d[2]*x0 + d[8]*x1 + d[14]*x2 + d[20]*x3 + d[26]*x4 + d[32]*x5;
    tp[3] = d[3]*x0 + d[9]*x1 + d[15]*x2 + d[21]*x3 + d[27]*x4 + d[33]*x5;
    tp[4] = d[4]*x0+ d[10]*x1 + d[16]*x2 + d[22]*x3 + d[28]*x4 + d[34]*x5;
    tp[5] = d[5]*x0+ d[11]*x1 + d[17]*x2 + d[23]*x3 + d[29]*x4 + d[35]*x5;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 36*ai[k]; 
    vj = aj + ai[k]; 
    tp    = t + k*6;    
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4]; x5=tp[5];  /* xk */ 
    nz = ai[k+1] - ai[k]; 
  
    tp = t + (*vj)*6;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*tp[0] + v[6]*tp[1] + v[12]*tp[2] + v[18]*tp[3] + v[24]*tp[4] + v[30]*tp[5];
      x1 += v[1]*tp[0] + v[7]*tp[1] + v[13]*tp[2] + v[19]*tp[3] + v[25]*tp[4] + v[31]*tp[5];
      x2 += v[2]*tp[0] + v[8]*tp[1] + v[14]*tp[2] + v[20]*tp[3] + v[26]*tp[4] + v[32]*tp[5];
      x3 += v[3]*tp[0] + v[9]*tp[1] + v[15]*tp[2] + v[21]*tp[3] + v[27]*tp[4] + v[33]*tp[5];
      x4 += v[4]*tp[0]+ v[10]*tp[1] + v[16]*tp[2] + v[22]*tp[3] + v[28]*tp[4] + v[34]*tp[5];
      x5 += v[5]*tp[0]+ v[11]*tp[1] + v[17]*tp[2] + v[23]*tp[3] + v[29]*tp[4] + v[35]*tp[5];
      vj++; tp = t + (*vj)*6;
      v += 36;
    }
    tp    = t + k*6;    
    tp[0]=x0; tp[1]=x1; tp[2]=x2; tp[3]=x3; tp[4]=x4; tp[5]=x5;
    idx   = 6*r[k];
    x[idx]     = x0;
    x[idx+1]   = x1;
    x[idx+2]   = x2;
    x[idx+3]   = x3;
    x[idx+4]   = x4;
    x[idx+5]   = x5;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(36*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_6_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_6_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*d;
  PetscScalar    *x,*xp,*b,x0,x1,x2,x3,x4,x5;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  
  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,6*mbs*sizeof(PetscScalar));CHKERRQ(ierr); /* x <- b */
  for (k=0; k<mbs; k++){
    v  = aa + 36*ai[k]; 
    xp = x + k*6;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4]; x5=xp[5]; /* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*6;
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      xp[0] +=  v[0]*x0 +  v[1]*x1 +  v[2]*x2 + v[3]*x3 + v[4]*x4 + v[5]*x5;
      xp[1] +=  v[6]*x0 +  v[7]*x1 +  v[8]*x2 + v[9]*x3+ v[10]*x4+ v[11]*x5;
      xp[2] += v[12]*x0 + v[13]*x1 + v[14]*x2+ v[15]*x3+ v[16]*x4+ v[17]*x5;
      xp[3] += v[18]*x0 + v[19]*x1 + v[20]*x2+ v[21]*x3+ v[22]*x4+ v[23]*x5;
      xp[4] += v[24]*x0 + v[25]*x1 + v[26]*x2+ v[27]*x3+ v[28]*x4+ v[29]*x5;
      xp[5] += v[30]*x0 + v[31]*x1 + v[32]*x2+ v[33]*x3+ v[34]*x4+ v[35]*x5;
      vj++; xp = x + (*vj)*6;
      v += 36;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    d  = aa+k*36;          /* ptr to inv(Dk) */
    xp = x + k*6;
    xp[0] = d[0]*x0 + d[6]*x1 + d[12]*x2 + d[18]*x3 + d[24]*x4 + d[30]*x5;
    xp[1] = d[1]*x0 + d[7]*x1 + d[13]*x2 + d[19]*x3 + d[25]*x4 + d[31]*x5;
    xp[2] = d[2]*x0 + d[8]*x1 + d[14]*x2 + d[20]*x3 + d[26]*x4 + d[32]*x5;
    xp[3] = d[3]*x0 + d[9]*x1 + d[15]*x2 + d[21]*x3 + d[27]*x4 + d[33]*x5;
    xp[4] = d[4]*x0+ d[10]*x1 + d[16]*x2 + d[22]*x3 + d[28]*x4 + d[34]*x5;
    xp[5] = d[5]*x0+ d[11]*x1 + d[17]*x2 + d[23]*x3 + d[29]*x4 + d[35]*x5;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 36*ai[k]; 
    xp = x + k*6;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4]; x5=xp[5]; /* xk */ 
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*6;
    while (nz--) {
      /* xk += U(k,:)*x(:) */    
      x0 += v[0]*xp[0] + v[6]*xp[1] + v[12]*xp[2] + v[18]*xp[3] + v[24]*xp[4] + v[30]*xp[5];
      x1 += v[1]*xp[0] + v[7]*xp[1] + v[13]*xp[2] + v[19]*xp[3] + v[25]*xp[4] + v[31]*xp[5];
      x2 += v[2]*xp[0] + v[8]*xp[1] + v[14]*xp[2] + v[20]*xp[3] + v[26]*xp[4] + v[32]*xp[5];
      x3 += v[3]*xp[0] + v[9]*xp[1] + v[15]*xp[2] + v[21]*xp[3] + v[27]*xp[4] + v[33]*xp[5];
      x4 += v[4]*xp[0]+ v[10]*xp[1] + v[16]*xp[2] + v[22]*xp[3] + v[28]*xp[4] + v[34]*xp[5];
      x5 += v[5]*xp[0]+ v[11]*xp[1] + v[17]*xp[2] + v[23]*xp[3] + v[29]*xp[4] + v[35]*xp[5];
      vj++; 
      v += 36; xp = x + (*vj)*6;
    }
    xp = x + k*6;
    xp[0]=x0; xp[1]=x1; xp[2]=x2; xp[3]=x3; xp[4]=x4; xp[5]=x5; 
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(36*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_5"
PetscErrorCode MatSolve_SeqSBAIJ_5(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,*r,idx;  
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*b,x0,x1,x2,x3,x4,*t,*tp;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  tp = t; 
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = 5*r[k];
    tp[0] = b[idx];
    tp[1] = b[idx+1];
    tp[2] = b[idx+2];
    tp[3] = b[idx+3];
    tp[4] = b[idx+4];
    tp += 5; 
  }
  
  for (k=0; k<mbs; k++){
    v  = aa + 25*ai[k]; 
    vj = aj + ai[k]; 
    tp = t + k*5;
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4];
    nz = ai[k+1] - ai[k];  

    tp = t + (*vj)*5;
    while (nz--) {  
      tp[0] +=  v[0]*x0 + v[1]*x1 + v[2]*x2 + v[3]*x3 + v[4]*x4;
      tp[1] +=  v[5]*x0 + v[6]*x1 + v[7]*x2 + v[8]*x3 + v[9]*x4;
      tp[2] += v[10]*x0+ v[11]*x1+ v[12]*x2+ v[13]*x3+ v[14]*x4;
      tp[3] += v[15]*x0+ v[16]*x1+ v[17]*x2+ v[18]*x3+ v[19]*x4;
      tp[4] += v[20]*x0+ v[21]*x1+ v[22]*x2+ v[23]*x3+ v[24]*x4;
      vj++; tp = t + (*vj)*5;
      v += 25;      
    }

    /* xk = inv(Dk)*(Dk*xk) */
    diag  = aa+k*25;          /* ptr to inv(Dk) */
    tp    = t + k*5;
      tp[0] = diag[0]*x0 + diag[5]*x1 + diag[10]*x2 + diag[15]*x3 + diag[20]*x4;
      tp[1] = diag[1]*x0 + diag[6]*x1 + diag[11]*x2 + diag[16]*x3 + diag[21]*x4;
      tp[2] = diag[2]*x0 + diag[7]*x1 + diag[12]*x2 + diag[17]*x3 + diag[22]*x4;
      tp[3] = diag[3]*x0 + diag[8]*x1 + diag[13]*x2 + diag[18]*x3 + diag[23]*x4;
      tp[4] = diag[4]*x0 + diag[9]*x1 + diag[14]*x2 + diag[19]*x3 + diag[24]*x4;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 25*ai[k]; 
    vj = aj + ai[k]; 
    tp    = t + k*5;    
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; x4=tp[4];/* xk */ 
    nz = ai[k+1] - ai[k]; 
  
    tp = t + (*vj)*5;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*tp[0] + v[5]*tp[1] + v[10]*tp[2] + v[15]*tp[3] + v[20]*tp[4];
      x1 += v[1]*tp[0] + v[6]*tp[1] + v[11]*tp[2] + v[16]*tp[3] + v[21]*tp[4];
      x2 += v[2]*tp[0] + v[7]*tp[1] + v[12]*tp[2] + v[17]*tp[3] + v[22]*tp[4];
      x3 += v[3]*tp[0] + v[8]*tp[1] + v[13]*tp[2] + v[18]*tp[3] + v[23]*tp[4];
      x4 += v[4]*tp[0] + v[9]*tp[1] + v[14]*tp[2] + v[19]*tp[3] + v[24]*tp[4];
      vj++; tp = t + (*vj)*5;
      v += 25;
    }
    tp    = t + k*5;    
    tp[0]=x0; tp[1]=x1; tp[2]=x2; tp[3]=x3; tp[4]=x4;
    idx   = 5*r[k];
    x[idx]     = x0;
    x[idx+1]   = x1;
    x[idx+2]   = x2;
    x[idx+3]   = x3;
    x[idx+4]   = x4;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(25*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_5_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_5_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*xp,*b,x0,x1,x2,x3,x4;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,5*mbs*sizeof(PetscScalar));CHKERRQ(ierr); /* x <- b */
  for (k=0; k<mbs; k++){
    v  = aa + 25*ai[k]; 
    xp = x + k*5;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4];/* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*5;
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      xp[0] +=  v[0]*x0 +  v[1]*x1 +  v[2]*x2 + v[3]*x3 + v[4]*x4;
      xp[1] +=  v[5]*x0 +  v[6]*x1 +  v[7]*x2 + v[8]*x3 + v[9]*x4;
      xp[2] += v[10]*x0 + v[11]*x1 + v[12]*x2+ v[13]*x3+ v[14]*x4;
      xp[3] += v[15]*x0 + v[16]*x1 + v[17]*x2+ v[18]*x3+ v[19]*x4;
      xp[4] += v[20]*x0 + v[21]*x1 + v[22]*x2+ v[23]*x3+ v[24]*x4;
      vj++; xp = x + (*vj)*5;
      v += 25;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*25;          /* ptr to inv(Dk) */
    xp   = x + k*5;
    xp[0] = diag[0]*x0 + diag[5]*x1 + diag[10]*x2 + diag[15]*x3 + diag[20]*x4;
    xp[1] = diag[1]*x0 + diag[6]*x1 + diag[11]*x2 + diag[16]*x3 + diag[21]*x4;
    xp[2] = diag[2]*x0 + diag[7]*x1 + diag[12]*x2 + diag[17]*x3 + diag[22]*x4;
    xp[3] = diag[3]*x0 + diag[8]*x1 + diag[13]*x2 + diag[18]*x3 + diag[23]*x4;
    xp[4] = diag[4]*x0 + diag[9]*x1 + diag[14]*x2 + diag[19]*x3 + diag[24]*x4;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 25*ai[k]; 
    xp = x + k*5;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; x4=xp[4];/* xk */ 
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*5;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*xp[0] + v[5]*xp[1] + v[10]*xp[2] + v[15]*xp[3] + v[20]*xp[4];
      x1 += v[1]*xp[0] + v[6]*xp[1] + v[11]*xp[2] + v[16]*xp[3] + v[21]*xp[4];
      x2 += v[2]*xp[0] + v[7]*xp[1] + v[12]*xp[2] + v[17]*xp[3] + v[22]*xp[4];
      x3 += v[3]*xp[0] + v[8]*xp[1] + v[13]*xp[2] + v[18]*xp[3] + v[23]*xp[4];      
      x4 += v[4]*xp[0] + v[9]*xp[1] + v[14]*xp[2] + v[19]*xp[3] + v[24]*xp[4];
      vj++; 
      v += 25; xp = x + (*vj)*5;
    }
    xp = x + k*5;
    xp[0]=x0; xp[1]=x1; xp[2]=x2; xp[3]=x3; xp[4]=x4;
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(25*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_4"
PetscErrorCode MatSolve_SeqSBAIJ_4(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,*r,idx;  
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*b,x0,x1,x2,x3,*t,*tp;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  tp = t;
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = 4*r[k];
    tp[0] = b[idx];
    tp[1] = b[idx+1];
    tp[2] = b[idx+2];
    tp[3] = b[idx+3];
    tp += 4;
  }
  
  for (k=0; k<mbs; k++){
    v  = aa + 16*ai[k]; 
    vj = aj + ai[k]; 
    tp = t + k*4;
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3];
    nz = ai[k+1] - ai[k];  

    tp = t + (*vj)*4;
    while (nz--) {  
      tp[0] += v[0]*x0 + v[1]*x1 + v[2]*x2 + v[3]*x3;
      tp[1] += v[4]*x0 + v[5]*x1 + v[6]*x2 + v[7]*x3;
      tp[2] += v[8]*x0 + v[9]*x1 + v[10]*x2+ v[11]*x3;
      tp[3] += v[12]*x0+ v[13]*x1+ v[14]*x2+ v[15]*x3;
      vj++; tp = t + (*vj)*4;
      v += 16;      
    }

    /* xk = inv(Dk)*(Dk*xk) */
    diag  = aa+k*16;          /* ptr to inv(Dk) */
    tp    = t + k*4;
    tp[0] = diag[0]*x0 + diag[4]*x1 + diag[8]*x2 + diag[12]*x3;
    tp[1] = diag[1]*x0 + diag[5]*x1 + diag[9]*x2 + diag[13]*x3;
    tp[2] = diag[2]*x0 + diag[6]*x1 + diag[10]*x2+ diag[14]*x3;
    tp[3] = diag[3]*x0 + diag[7]*x1 + diag[11]*x2+ diag[15]*x3;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 16*ai[k]; 
    vj = aj + ai[k]; 
    tp    = t + k*4;    
    x0=tp[0]; x1=tp[1]; x2=tp[2]; x3=tp[3]; /* xk */ 
    nz = ai[k+1] - ai[k]; 
  
    tp = t + (*vj)*4;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*tp[0] + v[4]*tp[1] + v[8]*tp[2] + v[12]*tp[3];
      x1 += v[1]*tp[0] + v[5]*tp[1] + v[9]*tp[2] + v[13]*tp[3];
      x2 += v[2]*tp[0] + v[6]*tp[1]+ v[10]*tp[2] + v[14]*tp[3];
      x3 += v[3]*tp[0] + v[7]*tp[1]+ v[11]*tp[2] + v[15]*tp[3];
      vj++; tp = t + (*vj)*4;
      v += 16;
    }
    tp    = t + k*4;    
    tp[0]=x0; tp[1]=x1; tp[2]=x2; tp[3]=x3;
    idx        = 4*r[k];
    x[idx]     = x0;
    x[idx+1]   = x1;
    x[idx+2]   = x2;
    x[idx+3]   = x3;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(16*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   Special case where the matrix was factored in the natural ordering. 
   This eliminates the need for the column and row permutation.
*/
#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_4_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_4_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*xp,*b,x0,x1,x2,x3;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,4*mbs*sizeof(PetscScalar));CHKERRQ(ierr); /* x <- b */
  for (k=0; k<mbs; k++){
    v  = aa + 16*ai[k]; 
    xp = x + k*4;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; /* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*4;
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      xp[0] += v[0]*x0 + v[1]*x1 + v[2]*x2 + v[3]*x3;
      xp[1] += v[4]*x0 + v[5]*x1 + v[6]*x2 + v[7]*x3;
      xp[2] += v[8]*x0 + v[9]*x1 + v[10]*x2+ v[11]*x3;
      xp[3] += v[12]*x0+ v[13]*x1+ v[14]*x2+ v[15]*x3;
      vj++; xp = x + (*vj)*4;
      v += 16;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*16;          /* ptr to inv(Dk) */
    xp   = x + k*4;
    xp[0] = diag[0]*x0 + diag[4]*x1 + diag[8]*x2 + diag[12]*x3;
    xp[1] = diag[1]*x0 + diag[5]*x1 + diag[9]*x2 + diag[13]*x3;
    xp[2] = diag[2]*x0 + diag[6]*x1 + diag[10]*x2+ diag[14]*x3;
    xp[3] = diag[3]*x0 + diag[7]*x1 + diag[11]*x2+ diag[15]*x3;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 16*ai[k]; 
    xp = x + k*4;
    x0=xp[0]; x1=xp[1]; x2=xp[2]; x3=xp[3]; /* xk */ 
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*4;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*xp[0] + v[4]*xp[1] + v[8]*xp[2] + v[12]*xp[3];
      x1 += v[1]*xp[0] + v[5]*xp[1] + v[9]*xp[2] + v[13]*xp[3];
      x2 += v[2]*xp[0] + v[6]*xp[1]+ v[10]*xp[2] + v[14]*xp[3];
      x3 += v[3]*xp[0] + v[7]*xp[1]+ v[11]*xp[2] + v[15]*xp[3];
      vj++; 
      v += 16; xp = x + (*vj)*4;
    }
    xp = x + k*4;
    xp[0] = x0; xp[1] = x1; xp[2] = x2; xp[3] = x3;
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);  
  ierr = PetscLogFlops(16*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_3"
PetscErrorCode MatSolve_SeqSBAIJ_3(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,*r,idx;  
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*b,x0,x1,x2,*t,*tp;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work;
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  tp = t;
  for (k=0; k<mbs; k++) { /* t <- perm(b) */
    idx   = 3*r[k];
    tp[0] = b[idx];
    tp[1] = b[idx+1];
    tp[2] = b[idx+2];
    tp += 3;
  }
  
  for (k=0; k<mbs; k++){
    v  = aa + 9*ai[k]; 
    vj = aj + ai[k]; 
    tp = t + k*3;
    x0 = tp[0]; x1 = tp[1]; x2 = tp[2]; 
    nz = ai[k+1] - ai[k];  

    tp = t + (*vj)*3;
    while (nz--) {  
      tp[0] += v[0]*x0 + v[1]*x1 + v[2]*x2; 
      tp[1] += v[3]*x0 + v[4]*x1 + v[5]*x2; 
      tp[2] += v[6]*x0 + v[7]*x1 + v[8]*x2; 
      vj++; tp = t + (*vj)*3;
      v += 9;      
    }

    /* xk = inv(Dk)*(Dk*xk) */
    diag  = aa+k*9;          /* ptr to inv(Dk) */
    tp    = t + k*3;
    tp[0] = diag[0]*x0 + diag[3]*x1 + diag[6]*x2;
    tp[1] = diag[1]*x0 + diag[4]*x1 + diag[7]*x2;
    tp[2] = diag[2]*x0 + diag[5]*x1 + diag[8]*x2;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 9*ai[k]; 
    vj = aj + ai[k]; 
    tp    = t + k*3;    
    x0 = tp[0]; x1 = tp[1]; x2 = tp[2];  /* xk */ 
    nz = ai[k+1] - ai[k]; 
  
    tp = t + (*vj)*3;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*tp[0] + v[3]*tp[1] + v[6]*tp[2];
      x1 += v[1]*tp[0] + v[4]*tp[1] + v[7]*tp[2];
      x2 += v[2]*tp[0] + v[5]*tp[1] + v[8]*tp[2];
      vj++; tp = t + (*vj)*3;
      v += 9;
    }
    tp    = t + k*3;    
    tp[0] = x0; tp[1] = x1; tp[2] = x2;
    idx      = 3*r[k];
    x[idx]   = x0;
    x[idx+1] = x1;
    x[idx+2] = x2;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(9*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   Special case where the matrix was factored in the natural ordering. 
   This eliminates the need for the column and row permutation.
*/
#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_3_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_3_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*xp,*b,x0,x1,x2;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,3*mbs*sizeof(PetscScalar));CHKERRQ(ierr);
  for (k=0; k<mbs; k++){
    v  = aa + 9*ai[k]; 
    xp = x + k*3;
    x0 = xp[0]; x1 = xp[1]; x2 = xp[2]; /* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*3;
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      xp[0] += v[0]*x0 + v[1]*x1 + v[2]*x2;
      xp[1] += v[3]*x0 + v[4]*x1 + v[5]*x2;
      xp[2] += v[6]*x0 + v[7]*x1 + v[8]*x2;
      vj++; xp = x + (*vj)*3;
      v += 9;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*9;          /* ptr to inv(Dk) */
    xp   = x + k*3;
    xp[0] = diag[0]*x0 + diag[3]*x1 + diag[6]*x2;
    xp[1] = diag[1]*x0 + diag[4]*x1 + diag[7]*x2;
    xp[2] = diag[2]*x0 + diag[5]*x1 + diag[8]*x2;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 9*ai[k]; 
    xp = x + k*3;
    x0 = xp[0]; x1 = xp[1]; x2 = xp[2];  /* xk */ 
    nz = ai[k+1] - ai[k];  
    vj = aj + ai[k];
    xp = x + (*vj)*3;
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*xp[0] + v[3]*xp[1] + v[6]*xp[2];
      x1 += v[1]*xp[0] + v[4]*xp[1] + v[7]*xp[2];
      x2 += v[2]*xp[0] + v[5]*xp[1] + v[8]*xp[2];
      vj++; 
      v += 9; xp = x + (*vj)*3;
    }
    xp = x + k*3;
    xp[0] = x0; xp[1] = x1; xp[2] = x2;
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(9*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_2"
PetscErrorCode MatSolve_SeqSBAIJ_2(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ *)A->data;
  IS             isrow=a->row;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,k2,*r,idx;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*b,x0,x1,*t;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  t  = a->solve_work; 
  /* printf("called MatSolve_SeqSBAIJ_2\n"); */
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr); 

  /* solve U^T * D * y = perm(b) by forward substitution */
  for (k=0; k<mbs; k++) {  /* t <- perm(b) */
    idx = 2*r[k];
    t[k*2]   = b[idx];
    t[k*2+1] = b[idx+1];
  }
  for (k=0; k<mbs; k++){
    v  = aa + 4*ai[k]; 
    vj = aj + ai[k]; 
    k2 = k*2;   
    x0 = t[k2]; x1 = t[k2+1];
    nz = ai[k+1] - ai[k];     
    while (nz--) {
      t[(*vj)*2]   += v[0]*x0 + v[1]*x1;
      t[(*vj)*2+1] += v[2]*x0 + v[3]*x1;
      vj++; v += 4;
    }
    diag = aa+k*4;  /* ptr to inv(Dk) */
    t[k2]   = diag[0]*x0 + diag[2]*x1;
    t[k2+1] = diag[1]*x0 + diag[3]*x1;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 4*ai[k]; 
    vj = aj + ai[k]; 
    k2 = k*2;
    x0 = t[k2]; x1 = t[k2+1];   
    nz = ai[k+1] - ai[k];    
    while (nz--) {
      x0 += v[0]*t[(*vj)*2] + v[2]*t[(*vj)*2+1];
      x1 += v[1]*t[(*vj)*2] + v[3]*t[(*vj)*2+1];
      vj++; v += 4;
    }
    t[k2]    = x0;
    t[k2+1]  = x1;
    idx      = 2*r[k];
    x[idx]   = x0; 
    x[idx+1] = x1;
  }

  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);  
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(4*(2*a->nz + mbs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   Special case where the matrix was factored in the natural ordering. 
   This eliminates the need for the column and row permutation.
*/
#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_2_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_2_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a=(Mat_SeqSBAIJ*)A->data;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v,*diag;
  PetscScalar    *x,*b,x0,x1;
  PetscErrorCode ierr;
  PetscInt       nz,*vj,k,k2;

  PetscFunctionBegin;
  
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);

  /* solve U^T * D * y = b by forward substitution */
  ierr = PetscMemcpy(x,b,2*mbs*sizeof(PetscScalar));CHKERRQ(ierr);
  for (k=0; k<mbs; k++){
    v  = aa + 4*ai[k]; 
    vj = aj + ai[k]; 
    k2 = k*2;
    x0 = x[k2]; x1 = x[k2+1];  /* Dk*xk = k-th block of x */
    nz = ai[k+1] - ai[k];  
    
    while (nz--) {
      /* x(:) += U(k,:)^T*(Dk*xk) */      
      x[(*vj)*2]   += v[0]*x0 + v[1]*x1;
      x[(*vj)*2+1] += v[2]*x0 + v[3]*x1;
      vj++; v += 4;      
    }
    /* xk = inv(Dk)*(Dk*xk) */
    diag = aa+k*4;          /* ptr to inv(Dk) */
    x[k2]   = diag[0]*x0 + diag[2]*x1;
    x[k2+1] = diag[1]*x0 + diag[3]*x1;
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + 4*ai[k]; 
    vj = aj + ai[k]; 
    k2 = k*2;
    x0 = x[k2]; x1 = x[k2+1];  /* xk */ 
    nz = ai[k+1] - ai[k];    
    while (nz--) {
      /* xk += U(k,:)*x(:) */
      x0 += v[0]*x[(*vj)*2] + v[2]*x[(*vj)*2+1];
      x1 += v[1]*x[(*vj)*2] + v[3]*x[(*vj)*2+1];
      vj++; v += 4;
    }
    x[k2]     = x0;
    x[k2+1]   = x1;
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(4*(2*a->nz + mbs));CHKERRQ(ierr); /* bs2*(2*a->nz + mbs) */
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_1"
PetscErrorCode MatSolve_SeqSBAIJ_1(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ *)A->data;
  IS             isrow=a->row;
  PetscErrorCode ierr;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j,*rip;
  MatScalar      *aa=a->a,*v;
  PetscScalar    *x,*b,xk,*t;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  if (!mbs) PetscFunctionReturn(0);
  /* printf(" MatSolve_SeqSBAIJ_1 is called\n"); */

  ierr = VecGetArray(bb,&b);CHKERRQ(ierr); 
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr); 
  t    = a->solve_work;

  ierr = ISGetIndices(isrow,&rip);CHKERRQ(ierr); 
  
  /* solve U^T*D*y = perm(b) by forward substitution */
  for (k=0; k<mbs; k++) t[k] = b[rip[k]];   
  for (k=0; k<mbs; k++){
    v  = aa + ai[k] + 1; 
    vj = aj + ai[k] + 1;    
    xk = t[k];
    nz = ai[k+1] - ai[k] - 1; 
    while (nz--) t[*vj++] += (*v++) * xk;
    t[k] = xk*aa[ai[k]];  /* aa[k] = 1/D(k) */
  }

  /* solve U*x = y by back substitution */   
  for (k=mbs-1; k>=0; k--){ 
    v  = aa + ai[k] + 1; 
    vj = aj + ai[k] + 1; 
    xk = t[k];   
    nz = ai[k+1] - ai[k] - 1;    
    while (nz--) xk += (*v++) * t[*vj++]; 
    t[k]      = xk;
    x[rip[k]] = xk; 
  }

  ierr = ISRestoreIndices(isrow,&rip);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr); 
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(4*a->nz + A->rmap.N);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolves_SeqSBAIJ_1"
PetscErrorCode MatSolves_SeqSBAIJ_1(Mat A,Vecs bb,Vecs xx)
{
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ *)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (A->rmap.bs == 1) {
    ierr = MatSolve_SeqSBAIJ_1(A,bb->v,xx->v);CHKERRQ(ierr);
  } else {
    IS              isrow=a->row;
    PetscInt             mbs=a->mbs,*ai=a->i,*aj=a->j,*rip,i;
    MatScalar       *aa=a->a,*v;
    PetscScalar     *x,*b,*t;
    PetscInt             nz,*vj,k,n;
    if (bb->n > a->solves_work_n) {
      ierr = PetscFree(a->solves_work);CHKERRQ(ierr);
      ierr = PetscMalloc(bb->n*A->rmap.N*sizeof(PetscScalar),&a->solves_work);CHKERRQ(ierr);
      a->solves_work_n = bb->n;
    }
    n    = bb->n;
    ierr = VecGetArray(bb->v,&b);CHKERRQ(ierr); 
    ierr = VecGetArray(xx->v,&x);CHKERRQ(ierr); 
    t    = a->solves_work;

    ierr = ISGetIndices(isrow,&rip);CHKERRQ(ierr); 
  
    /* solve U^T*D*y = perm(b) by forward substitution */
    for (k=0; k<mbs; k++) {for (i=0; i<n; i++) t[n*k+i] = b[rip[k]+i*mbs];} /* values are stored interlaced in t */
    for (k=0; k<mbs; k++){
      v  = aa + ai[k]; 
      vj = aj + ai[k];    
      nz = ai[k+1] - ai[k];     
      while (nz--) {
        for (i=0; i<n; i++) t[n*(*vj)+i] += (*v) * t[n*k+i];
        v++;vj++;
      }
      for (i=0; i<n; i++) t[n*k+i] *= aa[k];  /* note: aa[k] = 1/D(k) */
    }
    
    /* solve U*x = y by back substitution */   
    for (k=mbs-1; k>=0; k--){ 
      v  = aa + ai[k]; 
      vj = aj + ai[k]; 
      nz = ai[k+1] - ai[k];    
      while (nz--) {
        for (i=0; i<n; i++) t[n*k+i] += (*v) * t[n*(*vj)+i]; 
        v++;vj++;
      }
      for (i=0; i<n; i++) x[rip[k]+i*mbs] = t[n*k+i];
    }

    ierr = ISRestoreIndices(isrow,&rip);CHKERRQ(ierr);
    ierr = VecRestoreArray(bb->v,&b);CHKERRQ(ierr); 
    ierr = VecRestoreArray(xx->v,&x);CHKERRQ(ierr); 
    ierr = PetscLogFlops(bb->n*(4*a->nz + A->rmap.N));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*
      Special case where the matrix was ILU(0) factored in the natural
   ordering. This eliminates the need for the column and row permutation.
*/
#undef __FUNCT__  
#define __FUNCT__ "MatSolve_SeqSBAIJ_1_NaturalOrdering"
PetscErrorCode MatSolve_SeqSBAIJ_1_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ *)A->data;
  PetscErrorCode ierr;
  PetscInt       mbs=a->mbs,*ai=a->i,*aj=a->j;
  MatScalar      *aa=a->a,*v;
  PetscScalar    *x,*b,xk;
  PetscInt       nz,*vj,k;

  PetscFunctionBegin;
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr); 
  ierr = VecGetArray(xx,&x);CHKERRQ(ierr); 
  
  /* solve U^T*D*y = b by forward substitution */
  ierr = PetscMemcpy(x,b,mbs*sizeof(PetscScalar));CHKERRQ(ierr);
  for (k=0; k<mbs; k++){
    v  = aa + ai[k] + 1; 
    vj = aj + ai[k] + 1;    
    xk = x[k];
    nz = ai[k+1] - ai[k] - 1;     /* exclude diag[k] */
    while (nz--) x[*vj++] += (*v++) * xk;
    x[k] = xk*aa[ai[k]];  /* note: aa[diag[k]] = 1/D(k) */
  }

  /* solve U*x = y by back substitution */ 
  for (k=mbs-2; k>=0; k--){ 
    v  = aa + ai[k] + 1; 
    vj = aj + ai[k] + 1; 
    xk = x[k];   
    nz = ai[k+1] - ai[k] - 1;    
    while (nz--) xk += (*v++) * x[*vj++];    
    x[k] = xk;      
  }

  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr); 
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr); 
  ierr = PetscLogFlops(4*a->nz + A->rmap.N);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* Use Modified Sparse Row storage for u and ju, see Saad pp.85 */
#undef __FUNCT__  
#define __FUNCT__ "MatICCFactorSymbolic_SeqSBAIJ_MSR"
PetscErrorCode MatICCFactorSymbolic_SeqSBAIJ_MSR(Mat A,IS perm,MatFactorInfo *info,Mat *B) 
{
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ*)A->data,*b;  
  PetscErrorCode ierr;
  PetscInt       *rip,i,mbs = a->mbs,*ai = a->i,*aj = a->j;
  PetscInt       *jutmp,bs = A->rmap.bs,bs2=a->bs2;
  PetscInt       m,reallocs = 0,*levtmp;
  PetscInt       *prowl,*q,jmin,jmax,juidx,nzk,qm,*iu,*ju,k,j,vj,umax,maxadd;
  PetscInt       incrlev,*lev,shift,prow,nz;
  PetscReal      f = info->fill,levels = info->levels; 
  PetscTruth     perm_identity;

  PetscFunctionBegin;
  /* check whether perm is the identity mapping */  
  ierr = ISIdentity(perm,&perm_identity);CHKERRQ(ierr);

  if (perm_identity){
    a->permute = PETSC_FALSE;
    ai = a->i; aj = a->j;
  } else { /*  non-trivial permutation */   
    a->permute = PETSC_TRUE; 
    ierr = MatReorderingSeqSBAIJ(A, perm);CHKERRQ(ierr);   
    ai = a->inew; aj = a->jnew;
  }
 
  /* initialization */  
  ierr  = ISGetIndices(perm,&rip);CHKERRQ(ierr);
  umax  = (PetscInt)(f*ai[mbs] + 1); 
  ierr  = PetscMalloc(umax*sizeof(PetscInt),&lev);CHKERRQ(ierr);
  umax += mbs + 1; 
  shift = mbs + 1;
  ierr  = PetscMalloc((mbs+1)*sizeof(PetscInt),&iu);CHKERRQ(ierr);
  ierr  = PetscMalloc(umax*sizeof(PetscInt),&ju);CHKERRQ(ierr);
  iu[0] = mbs + 1; 
  juidx = mbs + 1;
  /* prowl: linked list for pivot row */
  ierr    = PetscMalloc((3*mbs+1)*sizeof(PetscInt),&prowl);CHKERRQ(ierr); 
  /* q: linked list for col index */
  q       = prowl + mbs; 
  levtmp  = q     + mbs;
  
  for (i=0; i<mbs; i++){
    prowl[i] = mbs; 
    q[i] = 0;
  }

  /* for each row k */
  for (k=0; k<mbs; k++){   
    nzk  = 0; 
    q[k] = mbs;
    /* copy current row into linked list */
    nz = ai[rip[k]+1] - ai[rip[k]];
    j = ai[rip[k]];
    while (nz--){
      vj = rip[aj[j++]]; 
      if (vj > k){
        qm = k; 
        do {
          m  = qm; qm = q[m];
        } while(qm < vj);
        if (qm == vj) {
          SETERRQ(PETSC_ERR_PLIB,"Duplicate entry in A\n"); 
        }     
        nzk++;
        q[m]       = vj;
        q[vj]      = qm;  
        levtmp[vj] = 0;   /* initialize lev for nonzero element */ 
      } 
    } 

    /* modify nonzero structure of k-th row by computing fill-in
       for each row prow to be merged in */
    prow = k; 
    prow = prowl[prow]; /* next pivot row (== 0 for symbolic factorization) */
   
    while (prow < k){
      /* merge row prow into k-th row */
      jmin = iu[prow] + 1; 
      jmax = iu[prow+1];     
      qm = k;
      for (j=jmin; j<jmax; j++){      
        incrlev = lev[j-shift] + 1; 
	if (incrlev > levels) continue; 
        vj      = ju[j]; 
        do {
          m = qm; qm = q[m];
        } while (qm < vj);
        if (qm != vj){      /* a fill */
          nzk++; q[m] = vj; q[vj] = qm; qm = vj; 
          levtmp[vj] = incrlev;
        } else {
          if (levtmp[vj] > incrlev) levtmp[vj] = incrlev;
        }       
      } 
      prow = prowl[prow]; /* next pivot row */     
    }  
   
    /* add k to row list for first nonzero element in k-th row */
    if (nzk > 1){
      i = q[k]; /* col value of first nonzero element in k_th row of U */    
      prowl[k] = prowl[i]; prowl[i] = k;
    } 
    iu[k+1] = iu[k] + nzk;  

    /* allocate more space to ju and lev if needed */
    if (iu[k+1] > umax) { 
      /* estimate how much additional space we will need */
      /* use the strategy suggested by David Hysom <hysom@perch-t.icase.edu> */
      /* just double the memory each time */
      maxadd = umax;      
      if (maxadd < nzk) maxadd = (mbs-k)*(nzk+1)/2;
      umax += maxadd;

      /* allocate a longer ju */
      ierr     = PetscMalloc(umax*sizeof(PetscInt),&jutmp);CHKERRQ(ierr);
      ierr     = PetscMemcpy(jutmp,ju,iu[k]*sizeof(PetscInt));CHKERRQ(ierr);
      ierr     = PetscFree(ju);CHKERRQ(ierr);       
      ju       = jutmp;

      ierr     = PetscMalloc(umax*sizeof(PetscInt),&jutmp);CHKERRQ(ierr);
      ierr     = PetscMemcpy(jutmp,lev,(iu[k]-shift)*sizeof(PetscInt));CHKERRQ(ierr); 
      ierr     = PetscFree(lev);CHKERRQ(ierr);       
      lev      = jutmp;
      reallocs += 2; /* count how many times we realloc */
    }

    /* save nonzero structure of k-th row in ju */
    i=k;    
    while (nzk --) {
      i                = q[i];
      ju[juidx]        = i;
      lev[juidx-shift] = levtmp[i]; 
      juidx++;
    }
  } 
  
#if defined(PETSC_USE_INFO)
  if (ai[mbs] != 0) {
    PetscReal af = ((PetscReal)iu[mbs])/((PetscReal)ai[mbs]);
    ierr = PetscInfo3(A,"Reallocs %D Fill ratio:given %G needed %G\n",reallocs,f,af);CHKERRQ(ierr);
    ierr = PetscInfo1(A,"Run with -pc_factor_fill %G or use \n",af);CHKERRQ(ierr);
    ierr = PetscInfo1(A,"PCFactorSetFill(pc,%G);\n",af);CHKERRQ(ierr);
    ierr = PetscInfo(A,"for best performance.\n");CHKERRQ(ierr);
  } else {
    ierr = PetscInfo(A,"Empty matrix.\n");CHKERRQ(ierr);
  }
#endif

  ierr = ISRestoreIndices(perm,&rip);CHKERRQ(ierr); 
  ierr = PetscFree(prowl);CHKERRQ(ierr);
  ierr = PetscFree(lev);CHKERRQ(ierr);

  /* put together the new matrix */
  ierr = MatCreate(A->comm,B);CHKERRQ(ierr);
  ierr = MatSetSizes(*B,bs*mbs,bs*mbs,bs*mbs,bs*mbs);CHKERRQ(ierr);
  ierr = MatSetType(*B,A->type_name);CHKERRQ(ierr);
  ierr = MatSeqSBAIJSetPreallocation_SeqSBAIJ(*B,bs,0,PETSC_NULL);CHKERRQ(ierr);

  /* ierr = PetscLogObjectParent(*B,iperm);CHKERRQ(ierr); */
  b    = (Mat_SeqSBAIJ*)(*B)->data;
  ierr = PetscFree2(b->imax,b->ilen);CHKERRQ(ierr);
  b->singlemalloc = PETSC_FALSE;
  b->free_a       = PETSC_TRUE;
  b->free_ij      = PETSC_TRUE;
  /* the next line frees the default space generated by the Create() */
  ierr    = PetscFree3(b->a,b->j,b->i);CHKERRQ(ierr);
  ierr    = PetscMalloc((iu[mbs]+1)*sizeof(MatScalar)*bs2,&b->a);CHKERRQ(ierr);
  b->j    = ju;
  b->i    = iu;
  b->diag = 0;
  b->ilen = 0;
  b->imax = 0;
 
  if (b->row) {
    ierr = ISDestroy(b->row);CHKERRQ(ierr);
  }
  if (b->icol) {
    ierr = ISDestroy(b->icol);CHKERRQ(ierr);
  }
  b->row  = perm;
  b->icol = perm;
  ierr    = PetscObjectReference((PetscObject)perm);CHKERRQ(ierr); 
  ierr    = PetscObjectReference((PetscObject)perm);CHKERRQ(ierr);
  ierr    = PetscMalloc((bs*mbs+bs)*sizeof(PetscScalar),&b->solve_work);CHKERRQ(ierr);
  /* In b structure:  Free imax, ilen, old a, old j.  
     Allocate idnew, solve_work, new a, new j */
  ierr    = PetscLogObjectMemory(*B,(iu[mbs]-mbs)*(sizeof(PetscInt)+sizeof(MatScalar)));CHKERRQ(ierr);
  b->maxnz = b->nz = iu[mbs];
  
  (*B)->factor                 = FACTOR_CHOLESKY;
  (*B)->info.factor_mallocs    = reallocs;
  (*B)->info.fill_ratio_given  = f;
  if (ai[mbs] != 0) {
    (*B)->info.fill_ratio_needed = ((PetscReal)iu[mbs])/((PetscReal)ai[mbs]);
  } else {
    (*B)->info.fill_ratio_needed = 0.0;
  }

  if (perm_identity){
    switch (bs) {
      case 1:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_1_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_1_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_1_NaturalOrdering;
        (*B)->ops->solves                = MatSolves_SeqSBAIJ_1;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=1\n");CHKERRQ(ierr);
        break;
      case 2:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_2_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_2_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_2_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=2\n");CHKERRQ(ierr);
        break;
      case 3:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_3_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_3_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_3_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=3\n");CHKERRQ(ierr);
        break; 
      case 4:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_4_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_4_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_4_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=4\n");CHKERRQ(ierr);
        break;
      case 5:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_5_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_5_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_5_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=5\n");CHKERRQ(ierr);
        break;
      case 6: 
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_6_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_6_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_6_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=6\n");CHKERRQ(ierr);
        break; 
      case 7:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_7_NaturalOrdering;
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_7_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_7_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=7\n");CHKERRQ(ierr);
      break; 
      default:
        (*B)->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_N_NaturalOrdering; 
        (*B)->ops->solve                 = MatSolve_SeqSBAIJ_N_NaturalOrdering;
        (*B)->ops->solvetranspose        = MatSolve_SeqSBAIJ_N_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS>7\n");CHKERRQ(ierr);
      break; 
    }
  }

  PetscFunctionReturn(0); 
}

#include "petscbt.h"
#include "src/mat/utils/freespace.h"
#undef __FUNCT__  
#define __FUNCT__ "MatICCFactorSymbolic_SeqSBAIJ"
PetscErrorCode MatICCFactorSymbolic_SeqSBAIJ(Mat A,IS perm,MatFactorInfo *info,Mat *fact)
{
  Mat_SeqSBAIJ       *a = (Mat_SeqSBAIJ*)A->data;
  Mat_SeqSBAIJ       *b;
  Mat                B;
  PetscErrorCode     ierr;
  PetscTruth         perm_identity,free_ij = PETSC_TRUE;
  PetscInt           bs=A->rmap.bs,am=a->mbs;
  PetscInt           reallocs=0,*rip,i,*ai,*aj,*ui;
  PetscInt           jmin,jmax,nzk,k,j,*jl,prow,*il,nextprow;
  PetscInt           nlnk,*lnk,*lnk_lvl=PETSC_NULL,ncols,*cols,*cols_lvl,*uj,**uj_ptr,**uj_lvl_ptr;
  PetscReal          fill=info->fill,levels=info->levels,ratio_needed;
  PetscFreeSpaceList free_space=PETSC_NULL,current_space=PETSC_NULL;
  PetscFreeSpaceList free_space_lvl=PETSC_NULL,current_space_lvl=PETSC_NULL;
  PetscBT            lnkbt;
  PetscScalar        *ua;

  PetscFunctionBegin;
  /*  
   This code originally uses Modified Sparse Row (MSR) storage
   (see page 85, "Iterative Methods ..." by Saad) for the output matrix B - bad choice!
   Then it is rewritten so the factor B takes seqsbaij format. However the associated 
   MatCholeskyFactorNumeric_() have not been modified for the cases of bs>1, 
   thus the original code in MSR format is still used for these cases. 
   The code below should replace MatICCFactorSymbolic_SeqSBAIJ_MSR() whenever 
   MatCholeskyFactorNumeric_() is modified for using sbaij symbolic factor.
  */
  if (bs > 1){ 
    ierr = MatICCFactorSymbolic_SeqSBAIJ_MSR(A,perm,info,fact);CHKERRQ(ierr);
    PetscFunctionReturn(0); 
  }

  /* check whether perm is the identity mapping */  
  ierr = ISIdentity(perm,&perm_identity);CHKERRQ(ierr);
 
  /* special case that simply copies fill pattern */
  if (!levels && perm_identity) {  
    a->permute = PETSC_FALSE;
    /* reuse the column pointers and row offsets for memory savings */
    ui           = a->i; 
    uj           = a->j;
    free_ij      = PETSC_FALSE;
    ratio_needed = 1.0;
  } else { /* case: levels>0 || (levels=0 && !perm_identity) */
    if (perm_identity){
      a->permute = PETSC_FALSE;
      ai = a->i; aj = a->j;
    } else { /*  non-trivial permutation */   
      a->permute = PETSC_TRUE;
      ierr = MatReorderingSeqSBAIJ(A, perm);CHKERRQ(ierr);   
      ai   = a->inew; aj = a->jnew;
    }
    ierr = ISGetIndices(perm,&rip);CHKERRQ(ierr);  

    /* initialization */
    ierr  = PetscMalloc((am+1)*sizeof(PetscInt),&ui);CHKERRQ(ierr);
    ui[0] = 0; 

    /* jl: linked list for storing indices of the pivot rows 
       il: il[i] points to the 1st nonzero entry of U(i,k:am-1) */
    ierr = PetscMalloc((2*am+1)*sizeof(PetscInt)+2*am*sizeof(PetscInt*),&jl);CHKERRQ(ierr); 
    il         = jl + am;
    uj_ptr     = (PetscInt**)(il + am);
    uj_lvl_ptr = (PetscInt**)(uj_ptr + am);
    for (i=0; i<am; i++){
      jl[i] = am; il[i] = 0;
    }

    /* create and initialize a linked list for storing column indices of the active row k */
    nlnk = am + 1;
    ierr = PetscIncompleteLLCreate(am,am,nlnk,lnk,lnk_lvl,lnkbt);CHKERRQ(ierr);

    /* initial FreeSpace size is fill*(ai[am]+1) */
    ierr = PetscFreeSpaceGet((PetscInt)(fill*(ai[am]+1)),&free_space);CHKERRQ(ierr);
    current_space = free_space;
    ierr = PetscFreeSpaceGet((PetscInt)(fill*(ai[am]+1)),&free_space_lvl);CHKERRQ(ierr);
    current_space_lvl = free_space_lvl;

    for (k=0; k<am; k++){  /* for each active row k */
      /* initialize lnk by the column indices of row rip[k] */
      nzk   = 0;
      ncols = ai[rip[k]+1] - ai[rip[k]]; 
      cols  = aj+ai[rip[k]];
      ierr = PetscIncompleteLLInit(ncols,cols,am,rip,nlnk,lnk,lnk_lvl,lnkbt);CHKERRQ(ierr);
      nzk += nlnk;

      /* update lnk by computing fill-in for each pivot row to be merged in */
      prow = jl[k]; /* 1st pivot row */
   
      while (prow < k){
        nextprow = jl[prow];
      
        /* merge prow into k-th row */
        jmin = il[prow] + 1;  /* index of the 2nd nzero entry in U(prow,k:am-1) */
        jmax = ui[prow+1]; 
        ncols = jmax-jmin;
        i     = jmin - ui[prow];
        cols  = uj_ptr[prow] + i; /* points to the 2nd nzero entry in U(prow,k:am-1) */
        j     = *(uj_lvl_ptr[prow] + i - 1);
        cols_lvl = uj_lvl_ptr[prow]+i;
        ierr = PetscICCLLAddSorted(ncols,cols,levels,cols_lvl,am,nlnk,lnk,lnk_lvl,lnkbt,j);CHKERRQ(ierr);
        nzk += nlnk;

        /* update il and jl for prow */
        if (jmin < jmax){
          il[prow] = jmin;
          j = *cols; jl[prow] = jl[j]; jl[j] = prow;  
        } 
        prow = nextprow; 
      }  

      /* if free space is not available, make more free space */
      if (current_space->local_remaining<nzk) {
        i = am - k + 1; /* num of unfactored rows */
        i = PetscMin(i*nzk, i*(i-1)); /* i*nzk, i*(i-1): estimated and max additional space needed */
        ierr = PetscFreeSpaceGet(i,&current_space);CHKERRQ(ierr);
        ierr = PetscFreeSpaceGet(i,&current_space_lvl);CHKERRQ(ierr);
        reallocs++;
      }

      /* copy data into free_space and free_space_lvl, then initialize lnk */
      ierr = PetscIncompleteLLClean(am,am,nzk,lnk,lnk_lvl,current_space->array,current_space_lvl->array,lnkbt);CHKERRQ(ierr);

      /* add the k-th row into il and jl */
      if (nzk-1 > 0){
        i = current_space->array[1]; /* col value of the first nonzero element in U(k, k+1:am-1) */    
        jl[k] = jl[i]; jl[i] = k;
        il[k] = ui[k] + 1;
      } 
      uj_ptr[k]     = current_space->array;
      uj_lvl_ptr[k] = current_space_lvl->array; 

      current_space->array           += nzk;
      current_space->local_used      += nzk;
      current_space->local_remaining -= nzk;
      current_space_lvl->array           += nzk;
      current_space_lvl->local_used      += nzk;
      current_space_lvl->local_remaining -= nzk;

      ui[k+1] = ui[k] + nzk;  
    } 

#if defined(PETSC_USE_INFO)
    if (ai[am] != 0) {
      PetscReal af = ((PetscReal)ui[am])/((PetscReal)ai[am]);
      ierr = PetscInfo3(A,"Reallocs %D Fill ratio:given %G needed %G\n",reallocs,fill,af);CHKERRQ(ierr);
      ierr = PetscInfo1(A,"Run with -pc_factor_fill %G or use \n",af);CHKERRQ(ierr);
      ierr = PetscInfo1(A,"PCFactorSetFill(pc,%G) for best performance.\n",af);CHKERRQ(ierr);
    } else {
      ierr = PetscInfo(A,"Empty matrix.\n");CHKERRQ(ierr);
    }
#endif

    ierr = ISRestoreIndices(perm,&rip);CHKERRQ(ierr);
    ierr = PetscFree(jl);CHKERRQ(ierr);

    /* destroy list of free space and other temporary array(s) */
    ierr = PetscMalloc((ui[am]+1)*sizeof(PetscInt),&uj);CHKERRQ(ierr);
    ierr = PetscFreeSpaceContiguous(&free_space,uj);CHKERRQ(ierr);
    ierr = PetscIncompleteLLDestroy(lnk,lnkbt);CHKERRQ(ierr);
    ierr = PetscFreeSpaceDestroy(free_space_lvl);CHKERRQ(ierr);
    if (ai[am] != 0) {
      ratio_needed = ((PetscReal)ui[am])/((PetscReal)ai[am]);
    } else {
      ratio_needed = 0.0;
    }
  } /* end of case: levels>0 || (levels=0 && !perm_identity) */

  /* put together the new matrix in MATSEQSBAIJ format */
  ierr = PetscMalloc((ui[am]+1)*sizeof(MatScalar),&ua);CHKERRQ(ierr);
  ierr = MatCreateSeqSBAIJWithArrays(PETSC_COMM_SELF,1,am,am,ui,uj,ua,fact);CHKERRQ(ierr);
  B = *fact;
  b = (Mat_SeqSBAIJ*)B->data;
  ierr = PetscFree2(b->imax,b->ilen);CHKERRQ(ierr);
  b->singlemalloc = PETSC_FALSE;
  b->free_a  = PETSC_TRUE;
  b->free_ij = free_ij;
  b->diag    = 0;
  b->ilen    = 0;
  b->imax    = 0;
  b->row     = perm;
  b->pivotinblocks = PETSC_FALSE; /* need to get from MatFactorInfo */
  ierr    = PetscObjectReference((PetscObject)perm);CHKERRQ(ierr); 
  b->icol = perm;
  ierr    = PetscObjectReference((PetscObject)perm);CHKERRQ(ierr); 
  ierr    = PetscMalloc((am+1)*sizeof(PetscScalar),&b->solve_work);CHKERRQ(ierr);
  b->maxnz = b->nz = ui[am];
  
  B->factor                 = FACTOR_CHOLESKY;
  B->info.factor_mallocs    = reallocs;
  B->info.fill_ratio_given  = fill;
  B->info.fill_ratio_needed = ratio_needed;

  if (perm_identity){
    switch (bs) {
      case 1:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_1_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_1_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_1_NaturalOrdering; 
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=1\n");CHKERRQ(ierr);
        break;
      case 2:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_2_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_2_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_2_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=2\n");CHKERRQ(ierr);
        break;
      case 3:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_3_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_3_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_3_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=3\n");CHKERRQ(ierr);
        break; 
      case 4:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_4_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_4_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_4_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=4\n");CHKERRQ(ierr);
        break;
      case 5:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_5_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_5_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_5_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=5\n");CHKERRQ(ierr);
        break;
      case 6: 
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_6_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_6_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_6_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=6\n");CHKERRQ(ierr);
        break; 
      case 7:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_7_NaturalOrdering;
        B->ops->solve                 = MatSolve_SeqSBAIJ_7_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_7_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS=7\n");CHKERRQ(ierr);
      break; 
      default:
        B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqSBAIJ_N_NaturalOrdering; 
        B->ops->solve                 = MatSolve_SeqSBAIJ_N_NaturalOrdering;
        B->ops->solvetranspose        = MatSolve_SeqSBAIJ_N_NaturalOrdering;
        ierr = PetscInfo(A,"Using special in-place natural ordering factor and solve BS>7\n");CHKERRQ(ierr);
      break; 
    }
  }
  PetscFunctionReturn(0); 
} 

