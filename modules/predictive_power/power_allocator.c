#include "power_allocator.h"
#include <math.h>
#include <stddef.h>
#include <float.h>

static float clip(float x,float lo,float hi) {return fminf(hi,fmaxf(lo,x));}
static bool valid(const PPAllocationProblem *p)
{
    if(!p || !isfinite(p->scale) || p->scale<=0 || !isfinite(p->budget_w) ||
        p->budget_w<0 || !isfinite(p->power_c)) return false;
    float l[4][4]={{0}};
    for(unsigned i=0;i<4;++i) {
        if(!isfinite(p->low[i]) || !isfinite(p->high[i]) || p->low[i]>p->high[i] ||
            p->low[i]<-32768 || p->high[i]>32767 || ceilf(p->low[i])>floorf(p->high[i]) ||
            !isfinite(p->linear[i]) || !isfinite(p->power_h[i]) || p->power_h[i]<0 ||
            !isfinite(p->power_g[i])) return false;
        for(unsigned j=0;j<4;++j)
            if(!isfinite(p->q[i][j]) || fabsf(p->q[i][j]-p->q[j][i])>1e-6f) return false;
        for(unsigned j=0;j<=i;++j) {
            float t=p->q[i][j];
            for(unsigned k=0;k<j;++k) t-=l[i][k]*l[j][k];
            if(i==j) {if(!isfinite(t) || t<=1e-7f) return false;l[i][j]=sqrtf(t);}
            else l[i][j]=t/l[j][j];
        }
    }
    return true;
}
static float power(const PPAllocationProblem *p,const float x[4])
{
    float v=p->power_c;for(unsigned i=0;i<4;++i) v+=p->power_h[i]*x[i]*x[i]+p->power_g[i]*x[i];return v;
}
static float cost(const PPAllocationProblem *p,const float x[4])
{
    float v=0;
    for(unsigned i=0;i<4;++i) {
        v+=p->linear[i]*x[i];for(unsigned j=0;j<4;++j) v+=x[i]*p->q[i][j]*x[j];
    }
    return v;
}
/* Four variables have only 3^4 faces. Use bounded exact face enumeration when
 * coordinate descent is slow on a coupled/ill-conditioned objective. */
static void polish_box(const PPAllocationProblem *p,float lambda,float x[4])
{
    float best=FLT_MAX;
    for(unsigned face=0;face<81;++face) {
        unsigned code=face,n=0,free_index[4];bool free_axis[4]={false};
        float candidate[4]={0},a[4][5]={{0}};
        for(unsigned i=0;i<4;++i) {
            const unsigned state=code%3;code/=3;
            if(state==0) {free_axis[i]=true;free_index[n++]=i;}
            else candidate[i]=(state==1 ? p->low[i] : p->high[i])/p->scale;
        }
        for(unsigned row=0;row<n;++row) {
            const unsigned i=free_index[row];float rhs=-.5f*(p->linear[i]+lambda*p->power_g[i]);
            for(unsigned j=0;j<4;++j) if(!free_axis[j]) rhs-=p->q[i][j]*candidate[j];
            for(unsigned col=0;col<n;++col)
                a[row][col]=p->q[i][free_index[col]]+(row==col ? lambda*p->power_h[i] : 0);
            a[row][n]=rhs;
        }
        bool solved=true;
        for(unsigned k=0;k<n;++k) {
            if(!isfinite(a[k][k]) || a[k][k]<=0) {solved=false;break;}
            for(unsigned row=k+1;row<n;++row) {
                const float ratio=a[row][k]/a[k][k];
                for(unsigned col=k+1;col<=n;++col) a[row][col]-=ratio*a[k][col];
            }
        }
        if(!solved) continue;
        for(unsigned reverse=n;reverse>0;--reverse) {
            const unsigned row=reverse-1;float value=a[row][n];
            for(unsigned col=row+1;col<n;++col) value-=a[row][col]*candidate[free_index[col]];
            candidate[free_index[row]]=value/a[row][row];
        }
        for(unsigned i=0;i<4;++i)
            if(!isfinite(candidate[i]) || candidate[i]<p->low[i]/p->scale ||
                candidate[i]>p->high[i]/p->scale) solved=false;
        if(!solved) continue;
        const float objective=cost(p,candidate)+lambda*(power(p,candidate)-p->power_c);
        if(isfinite(objective) && objective<best) {
            best=objective;for(unsigned i=0;i<4;++i) x[i]=candidate[i];
        }
    }
}
bool PPAllocationCheck(const PPAllocationProblem *p,const int16_t u[4],float *watts)
{
    if(!u || !valid(p)) return false;
    float x[4];
    for(unsigned i=0;i<4;++i) {
        if(u[i]<p->low[i] || u[i]>p->high[i]) return false;
        x[i]=(float)u[i]/p->scale;
    }
    const float v=power(p,x);if(watts) *watts=v;
    return isfinite(v) && v<=p->budget_w;
}
/* Fixed work bound. Every accepted point is checked independently even when
 * coordinate descent hits its iteration cap. Residual reports stationarity. */
static void box(const PPAllocationProblem *p,float lambda,float x[4],uint32_t *iterations)
{
    for(unsigned sweep=0;sweep<32;++sweep) {
        float change=0;
        for(unsigned i=0;i<4;++i) {
            float g=p->linear[i]+lambda*p->power_g[i];
            for(unsigned j=0;j<4;++j) if(j!=i) g+=2*p->q[i][j]*x[j];
            const float next=clip(-g/(2*(p->q[i][i]+lambda*p->power_h[i])),
                p->low[i]/p->scale,p->high[i]/p->scale);
            change=fmaxf(change,fabsf(next-x[i]));x[i]=next;
        }
        ++*iterations;if(change<1e-6f) break;
    }
    float residual=0;
    for(unsigned i=0;i<4;++i) {
        float g=p->linear[i]+lambda*p->power_g[i]+2*lambda*p->power_h[i]*x[i];
        for(unsigned j=0;j<4;++j) g+=2*p->q[i][j]*x[j];
        if(x[i]<=p->low[i]/p->scale) g=fminf(g,0);
        if(x[i]>=p->high[i]/p->scale) g=fmaxf(g,0);
        residual=fmaxf(residual,fabsf(g));
    }
    if(residual>1e-4f) {polish_box(p,lambda,x);*iterations+=81;}
}
static void integer_candidates(const PPAllocationProblem *p,const float x[4],PPAllocationResult *r)
{
    for(unsigned bits=0;bits<16;++bits) {
        int16_t u[4];float q[4];
        for(unsigned i=0;i<4;++i) {
            const float raw=x[i]*p->scale;
            if(!isfinite(raw)) return;
            const float rounded=(bits&(1U<<i)) ? ceilf(raw) : floorf(raw);
            u[i]=(int16_t)clip(rounded,ceilf(p->low[i]),floorf(p->high[i]));q[i]=(float)u[i]/p->scale;
        }
        float w;
        if(PPAllocationCheck(p,u,&w)) {
            const float v=cost(p,q);
            if(isfinite(v) && (!r->feasible || v<r->objective)) {
                for(unsigned i=0;i<4;++i) r->command[i]=u[i];
                r->power_w=w;r->objective=v;r->feasible=true;
            }
        }
    }
}
bool PPAllocate(const PPAllocationProblem *p,PPAllocationResult *r)
{
    if(!r) return false;
    *r=(PPAllocationResult){.residual=FLT_MAX};if(!valid(p)) return false;
    float minimum[4],x[4];
    for(unsigned i=0;i<4;++i) {
        minimum[i]=p->power_h[i]>0 ? clip(-p->power_g[i]/(2*p->power_h[i]),p->low[i]/p->scale,p->high[i]/p->scale) :
            (p->power_g[i]>=0 ? p->low[i] : p->high[i])/p->scale;
        x[i]=clip(0,p->low[i]/p->scale,p->high[i]/p->scale);
    }
    if(!isfinite(power(p,minimum)) || power(p,minimum)>p->budget_w) return false;
    integer_candidates(p,minimum,r);
    box(p,0,x,&r->iterations);
    float lambda=0;
    if(power(p,x)>p->budget_w) {
        float lo=0,hi=1;
        for(unsigned k=0;k<24;++k) {
            box(p,hi,x,&r->iterations);
            if(power(p,x)<=p->budget_w) break;
            lo=hi;hi*=2;
        }
        for(unsigned k=0;k<24;++k) {
            const float mid=(lo+hi)*.5f;box(p,mid,x,&r->iterations);
            if(power(p,x)>p->budget_w) lo=mid;else hi=mid;
        }
        lambda=hi;box(p,lambda,x,&r->iterations);
    }
    integer_candidates(p,x,r);
    if(!r->feasible) return false;
    r->residual=0;
    for(unsigned i=0;i<4;++i) {
        float g=p->linear[i]+lambda*p->power_g[i]+2*lambda*p->power_h[i]*r->command[i]/p->scale;
        for(unsigned j=0;j<4;++j) g+=2*p->q[i][j]*r->command[j]/p->scale;
        if(r->command[i]<=ceilf(p->low[i])) g=fminf(g,0);
        if(r->command[i]>=floorf(p->high[i])) g=fmaxf(g,0);
        r->residual=fmaxf(r->residual,fabsf(g));
    }
    return PPAllocationCheck(p,r->command,&r->power_w);
}
