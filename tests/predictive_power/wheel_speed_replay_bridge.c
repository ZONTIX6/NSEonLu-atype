/* Host-only ABI checks; all estimator and power arithmetic remains production C. */
#include "wheel_speed_kf.h"
#include <stddef.h>
#define O(T,F) offsetof(T,F)
size_t PPReplayAbi(unsigned type,unsigned field)
{
    static const size_t config[]={sizeof(PPWheelKFConfig),O(PPWheelKFConfig,version),O(PPWheelKFConfig,max_gap_us),
        O(PPWheelKFConfig,max_age_us),O(PPWheelKFConfig,max_horizon_us),O(PPWheelKFConfig,q),O(PPWheelKFConfig,r),
        O(PPWheelKFConfig,initial_p00),O(PPWheelKFConfig,initial_p11),O(PPWheelKFConfig,gate_squared),O(PPWheelKFConfig,max_rejections)};
    static const size_t sample[]={sizeof(PPWheelSample),O(PPWheelSample,rx_time_us),O(PPWheelSample,sequence),
        O(PPWheelSample,source_epoch),O(PPWheelSample,omega_rad_s),O(PPWheelSample,online)};
    static const size_t estimate[]={sizeof(PPWheelEstimate),O(PPWheelEstimate,omega),O(PPWheelEstimate,alpha),
        O(PPWheelEstimate,p00),O(PPWheelEstimate,p01),O(PPWheelEstimate,p11),O(PPWheelEstimate,innovation),
        O(PPWheelEstimate,innovation_variance),O(PPWheelEstimate,corrected_time_us),O(PPWheelEstimate,output_time_us),
        O(PPWheelEstimate,sequence),O(PPWheelEstimate,source_epoch),O(PPWheelEstimate,config_version),
        O(PPWheelEstimate,valid),O(PPWheelEstimate,reason)};
    static const size_t filter[]={sizeof(PPWheelKF),O(PPWheelKF,corrected),O(PPWheelKF,seen_time_us),
        O(PPWheelKF,seen_sequence),O(PPWheelKF,seen_source_epoch),O(PPWheelKF,rejected_consecutive),
        O(PPWheelKF,seen),O(PPWheelKF,initialized)};
    const size_t *sets[]={config,sample,estimate,filter};
    const unsigned sizes[]={sizeof config/sizeof *config,sizeof sample/sizeof *sample,
        sizeof estimate/sizeof *estimate,sizeof filter/sizeof *filter};
    return type<4 && field<sizes[type] ? sets[type][field] : (size_t)-1;
}
