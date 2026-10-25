#include "chassis_force_geometry.h"
#include <math.h>
#include <string.h>
bool ChassisForceGeometryInit(const ChassisForceConfig_s *c,ChassisForceGeometry_s *g)
{
    if(!g) return false;
    memset(g,0,sizeof *g);
    if(!ChassisForceConfigValid(c)) return false;
    float m[3][3]={{0}}, inv[3][3];
    for(unsigned w=0;w<4;++w) {
        const ChassisForceWheel_s *p=&c->wheel[w];
        g->h[w][0]=p->dx;g->h[w][1]=p->dy;
        g->h[w][2]=-p->dx*p->y_m+p->dy*p->x_m;
        for(unsigned j=0;j<3;++j) for(unsigned k=0;k<3;++k) m[j][k]+=g->h[w][j]*g->h[w][k];
    }
    const float det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-
        m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
    if(!isfinite(det) || det<=1e-8f) return false;
    for(unsigned j=0;j<3;++j) for(unsigned k=0;k<3;++k)
        inv[k][j]=(m[(j+1)%3][(k+1)%3]*m[(j+2)%3][(k+2)%3]-
                   m[(j+1)%3][(k+2)%3]*m[(j+2)%3][(k+1)%3])/det;
    const float condition=(m[0][0]+m[1][1]+m[2][2])*(inv[0][0]+inv[1][1]+inv[2][2]);
    if(!isfinite(condition) || condition>10000) return false;
    for(unsigned w=0;w<4;++w) for(unsigned j=0;j<3;++j) {
        for(unsigned k=0;k<3;++k) g->plus[j][w]+=inv[j][k]*g->h[w][k];
        g->current_map[w][j]=g->plus[j][w]*c->wheel[w].radius_m/c->wheel[w].k_out_nm_a*(16384.0f/20);
        g->wrench_map[j][w]=g->h[w][j]*c->wheel[w].k_out_nm_a/c->wheel[w].radius_m*(20.0f/16384);
        if(!isfinite(g->current_map[w][j]) || !isfinite(g->wrench_map[j][w])) return false;
    }
    return true;
}
void ChassisForceToCurrent(const ChassisForceGeometry_s *g,const float wrench[3],float current[4])
{
    for(unsigned w=0;w<4;++w) {current[w]=0;for(unsigned k=0;k<3;++k) current[w]+=g->current_map[w][k]*wrench[k];}
}
void ChassisForceFromCurrent(const ChassisForceGeometry_s *g,const float current[4],float wrench[3])
{
    for(unsigned k=0;k<3;++k) {wrench[k]=0;for(unsigned w=0;w<4;++w) wrench[k]+=g->wrench_map[k][w]*current[w];}
}
void ChassisForceEstimate(const ChassisForceGeometry_s *g,const ChassisForceConfig_s *c,const float rpm[4],float velocity[3])
{
    for(unsigned k=0;k<3;++k) {
        velocity[k]=0;
        for(unsigned w=0;w<4;++w) velocity[k]+=g->plus[k][w]*rpm[w]*(6.28318530718f/60)*c->wheel[w].radius_m/c->wheel[w].reduction;
    }
}
