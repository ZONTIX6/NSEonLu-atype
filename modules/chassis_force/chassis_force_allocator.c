#include "chassis_force_allocator.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static bool input_valid(const ChassisForceAllocationInput_s *i)
{
    if(!i || !isfinite(i->budget_w) || i->budget_w<0 || i->linear_count>2) return false;
    for(unsigned w=0;w<4;++w) {
        const MotorPowerModelConfig_s *m=&i->models[w];
        const float v[]={i->high[w],i->low[w],i->min_current[w],i->max_current[w],i->rpm_bound[w],
            m->k0,m->k1,m->k2,m->k3,m->k4,m->k5,m->current_conversion};
        for(unsigned k=0;k<sizeof v/sizeof v[0];++k) if(!isfinite(v[k])) return false;
        if(i->min_current[w]>i->max_current[w] || m->current_conversion<=0 || i->rpm_bound[w]<0) return false;
    }
    for(unsigned j=0;j<i->linear_count;++j) {
        if(!isfinite(i->bound[j])) return false;
        for(unsigned w=0;w<4;++w) if(!isfinite(i->linear[j][w])) return false;
    }
    return true;
}
float ChassisForcePredictWheel(const MotorPowerModelConfig_s *m,float current,float n)
{
    const float x=fabsf(current/m->current_conversion);
    const float p=m->k0+m->k1*x+m->k2*n+m->k3*x*n+m->k4*x*x+m->k5*n*n;
    return isfinite(p) ? fmaxf(0,p) : NAN;
}
float ChassisForcePredict(const ChassisForceAllocationInput_s *i,const float current[4])
{
    float p=0;
    for(unsigned w=0;w<4;++w) {
        const float v=ChassisForcePredictWheel(&i->models[w],current[w],i->rpm_bound[w]);
        if(!isfinite(v)) return NAN;
        p+=fmaxf(0,v);
    }
    return p;
}
bool ChassisForceCandidateValid(const ChassisForceAllocationInput_s *i,const float c[4])
{
    if(!input_valid(i) || !c) return false;
    for(unsigned w=0;w<4;++w)
        if(!isfinite(c[w]) || c[w]<i->min_current[w]-.0001f || c[w]>i->max_current[w]+.0001f) return false;
    for(unsigned j=0;j<i->linear_count;++j) {
        float dot=0;for(unsigned w=0;w<4;++w) dot+=i->linear[j][w]*c[w];
        if(!isfinite(dot) || dot>i->bound[j]+.000001f) return false;
    }
    const float p=ChassisForcePredict(i,c);
    return isfinite(p) && p<=i->budget_w+.00002f;
}
/* Stable real roots, including constant and linear degeneracy. */
static unsigned roots(float a,float b,float c,float r[2])
{
    const float scale=fmaxf(fabsf(a),fmaxf(fabsf(b),fabsf(c)));
    if(scale==0) return 0;
    a/=scale;b/=scale;c/=scale;
    if(fabsf(a)<1e-7f) {
        if(fabsf(b)<1e-7f) return 0;
        r[0]=-c/b;return 1;
    }
    float d=b*b-4*a*c;
    if(d<0) {if(d<-.000001f) return 0;d=0;}
    const float q=-.5f*(b+copysignf(sqrtf(d),b));
    if(q==0) {r[0]=-b/(2*a);return 1;}
    r[0]=q/a;r[1]=c/q;return 2;
}
static bool clip(float a,float b,float bound,float *lo,float *hi)
{
    if(b==0) return a<=bound+.000001f;
    const float t=(bound-a)/b;
    if(b>0) *hi=fminf(*hi,t);else *lo=fmaxf(*lo,t);
    return *lo<=*hi;
}
static void polynomial(const ChassisForceAllocationInput_s *i,unsigned w,float a,float b,float sign,float p[3])
{
    const MotorPowerModelConfig_s *m=&i->models[w];
    const float x=sign*a/m->current_conversion,y=sign*b/m->current_conversion,n=i->rpm_bound[w];
    p[0]=m->k4*y*y;
    p[1]=(m->k1+m->k3*n)*y+2*m->k4*x*y;
    p[2]=m->k0+m->k2*n+m->k5*n*n+(m->k1+m->k3*n)*x+m->k4*x*x;
}
static void point(float points[22],unsigned *count,float x,float lo,float hi)
{
    if(isfinite(x) && x>lo && x<hi && *count<22) points[(*count)++]=x;
}
static bool line(const ChassisForceAllocationInput_s *i,const float a[4],const float b[4],float *result)
{
    float lo=0,hi=1;
    for(unsigned w=0;w<4;++w)
        if(!clip(a[w],b[w],i->max_current[w],&lo,&hi) || !clip(-a[w],-b[w],-i->min_current[w],&lo,&hi)) return false;
    for(unsigned j=0;j<i->linear_count;++j) {
        float x=0,y=0;for(unsigned w=0;w<4;++w) {x+=i->linear[j][w]*a[w];y+=i->linear[j][w]*b[w];}
        if(!clip(x,y,i->bound[j],&lo,&hi)) return false;
    }
    float points[22]={lo,hi};unsigned count=2;
    for(unsigned w=0;w<4;++w) {
        if(b[w]!=0) point(points,&count,-a[w]/b[w],lo,hi);
        for(int sign=-1;sign<=1;sign+=2) {
            float p[3],r[2];polynomial(i,w,a[w],b[w],(float)sign,p);
            const unsigned nr=roots(p[0],p[1],p[2],r);
            for(unsigned k=0;k<nr;++k)
                if((a[w]+b[w]*r[k])*sign>=-.00001f) point(points,&count,r[k],lo,hi);
        }
    }
    for(unsigned k=1;k<count;++k) {
        const float v=points[k];unsigned j=k;
        while(j>0 && points[j-1]>v) {points[j]=points[j-1];--j;}points[j]=v;
    }
    float best=-1;
    for(unsigned k=1;k<count;++k) {
        const float left=points[k-1],right=points[k],mid=(left+right)*.5f;
        float sum[3]={0,0,-i->budget_w};
        for(unsigned w=0;w<4;++w) {
            float p[3];polynomial(i,w,a[w],b[w],a[w]+b[w]*mid>=0 ? 1 : -1,p);
            if((p[0]*mid+p[1])*mid+p[2]>0) for(unsigned j=0;j<3;++j) sum[j]+=p[j];
        }
        float candidates[4]={left,right,0,0};
        const unsigned nr=roots(sum[0],sum[1],sum[2],&candidates[2]);
        for(unsigned j=0;j<2+nr;++j) {
            float t=candidates[j],c[4];
            if(t<left-.0000002f || t>right+.0000002f) continue;
            t=fmaxf(left,fminf(right,t));
            for(unsigned w=0;w<4;++w) c[w]=a[w]+b[w]*t;
            /* Either side of a root can be feasible. Check a bounded ULP
             * neighborhood on both sides, still inside this segment. */
            float below=t,above=t;
            for(unsigned retry=0;retry<5;++retry) {
                const float trial[2]={above,below};
                for(unsigned side=0;side<2;++side) {
                    if(trial[side]<left || trial[side]>right) continue;
                    for(unsigned w=0;w<4;++w) c[w]=a[w]+b[w]*trial[side];
                    if(ChassisForceCandidateValid(i,c)) best=fmaxf(best,trial[side]);
                }
                below=nextafterf(below,-INFINITY);above=nextafterf(above,INFINITY);
            }
        }
    }
    if(best<0) return false;
    *result=best;return true;
}
bool ChassisForceAllocate(const ChassisForceAllocationInput_s *i,ChassisForceAllocationOutput_s *o)
{
    if(!o) return false;
    memset(o,0,sizeof *o);
    if(!input_valid(i)) return false;
    const float zero[4]={0};float base[4];
    if(!line(i,zero,i->high,&o->alpha)) {o->infeasible=true;return false;}
    for(unsigned w=0;w<4;++w) base[w]=o->alpha*i->high[w];
    if(!line(i,base,i->low,&o->beta)) {o->infeasible=true;return false;}
    for(unsigned w=0;w<4;++w) o->current[w]=base[w]+o->beta*i->low[w];
    o->predicted_w=ChassisForcePredict(i,o->current);
    o->valid=ChassisForceCandidateValid(i,o->current);
    return o->valid;
}
