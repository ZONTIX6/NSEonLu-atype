#include "board_chassis_force.h"

/*
 * 车辆力控参数填写入口：只在本函数填写车辆参数，不修改 modules/chassis_force 内部算法。
 *
 * 标记约定：
 *   [沿用] 原工程已有值，注明来源；装配、轮胎或重心改变后需要复核。
 *   [待测] 原工程没有的物理标定/保护参数，先留 0，测量并验收后填写。
 *   [初值] 软件整形/调节参数，可从现有初值开始，再通过台架整定。
 *
 * 单位约定：车体 x 向前、y 向左，正转矩为俯视逆时针；位置相对车辆质心。
 * 四轮顺序固定为 LF 左前、RF 右前、LB 左后、RB 右后，对应 CAN1 电调 ID 1、4、2、3。
 * CAN 电流命令不是 A：I_cmd_A = command * 20 / 16384；反馈电流字段的标度需另外核对。
 *
 * 标定顺序：核对轮序/方向/几何 -> 受限单轮电流台架测 K_out -> 填电流/轮力/保护上限
 *           -> 低力平移与固定转矩 -> 功率自适应 -> 可选退出制动。
 * 当前没有独立单轮标定入口；不要通过伪填参数来绕过未标定锁定。
 * 全部必填项验收后，再通过构建参数设置 CHASSIS_FORCE_CALIBRATION_VALID=1。
 * 只改有效标志仍不能通过参数检查；修改任意参数后，必须在函数末尾重新计算 checksum。
 */
ChassisForceConfig_s BoardChassisForceConfig(void)
{
    ChassisForceConfig_s c={
        .backend=CHASSIS_CONTROL_FORCE, /* 协议后端标识，无单位；固定为 FORCE，不是调参项。 */
        .schema_version=1U,             /* 参数结构版本，无单位；修改结构时单独升级。 */
        .calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0, /* 标定验收标志；默认宏值为 0。 */

        /* 四轮逐轮填写。共同标定方法：
         * k_out_nm_a：输出轴接已知力臂 L(m)，测力 F(N)，tau=F*L；多个低电流正/反向
         *             点拟合 tau-I 的斜率，单位 N*m/A。它是减速器输出端常数，不再乘减速比。
         *             电源输入电流不能代替电机转矩电流；检查摩擦、偏置及正反向拟合误差。
         * current_limit：根据电机、电调、线束能力及短时电流/温升记录确定；不得超过 16384。
         * force_limit_n：用测力装置和低力加载验证可重复传递的轮缘力，考虑接地滑移后留裕量；
         *                架空轮速或输出轴转矩不能证明真实接地力。
         * current_slew：低电流阶跃/斜坡，记录电流、负载功率和冲击，逐步确定允许变化率。
         */
        .wheel={
            [0]={ /* LF 左前，CAN1 ID 1。几何在下方从原车配置装配。 */
                .k_out_nm_a=0.0f,   /* [待测] N*m/A，输出轴转矩常数，按上述力臂法拟合。 */
                .current_limit=0.0f, /* [待测] CAN 命令幅值，逐轮电流/温升验证。 */
                .force_limit_n=0.0f, /* [待测] N，保守可传递轮缘力，低力加载验证。 */
                .current_slew=0.0f,  /* [待测] CAN 命令/s，低电流斜坡验证。 */
            },
            [1]={ /* RF 右前，CAN1 ID 4。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右前轮组。 */
                .current_limit=0.0f, /* [待测] CAN 命令幅值。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=0.0f,  /* [待测] CAN 命令/s。 */
            },
            [2]={ /* LB 左后，CAN1 ID 2。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量左后轮组。 */
                .current_limit=0.0f, /* [待测] CAN 命令幅值。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=0.0f,  /* [待测] CAN 命令/s。 */
            },
            [3]={ /* RB 右后，CAN1 ID 3。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右后轮组。 */
                .current_limit=0.0f, /* [待测] CAN 命令幅值。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=0.0f,  /* [待测] CAN 命令/s。 */
            },
        },

        /* 车体力/转矩：从已验证的低电流能力逐步增加，不由旧目标轮速换算。 */
        .force_max_n=0.0f,             /* [待测] N，满平移输入的合力；低力加载/加速试验确认。 */
        .spin_torque_nm=0.0f,          /* [待测] N*m，固定模式旋转转矩；从低值爬升并核对电流/功率。 */
        .spin_start_torque_nm=0.0f,    /* [待测] N*m，自适应起步转矩；验证起步与温升，不保证最小输出。 */
        .spin_torque_max_nm=0.0f,      /* [待测] N*m，旋转转矩上限；start、固定转矩均不得超过它。 */
        .force_rise_n_s=0.0f,          /* [待测] N/s，平移力上升斜率；操作阶跃时测电流/功率/冲击。 */
        .force_fall_n_s=0.0f,          /* [待测] N/s，平移力下降斜率；松杆滑行试验确认撤力速度。 */
        .torque_rise_nm_s=0.0f,        /* [待测] N*m/s，旋转转矩上升斜率；低转矩进入/反向试验确认。 */
        .torque_fall_nm_s=0.0f,        /* [待测] N*m/s，旋转转矩下降斜率；反向和失联降额试验确认。 */

        /* 速度只作保护，不是正常驱动力的目标值。轮级 rpm 一律指电机转子。 */
        .velocity_limit_mps=0.0f,      /* [待测] m/s，平移上限；外部测速核对估计后按机械能力留裕量。 */
        .omega_limit_rad_s=0.0f,       /* [待测] rad/s，车体角速度上限；独立测速核对峰值及撤力延迟。 */
        .wheel_limit_rpm=0.0f,         /* [待测] 转子 rpm，轮级上限；核对反馈标度与轮组机械能力。 */
        .resume_ratio=0.9f,            /* [初值] 无单位，(0,1)；降至上限乘此比例后恢复，检查有无抖动。 */
        .startup_current_limit=0.0f,   /* [待测] CAN 命令幅值，近零速每轮上限；短时低电流起步验证。 */
        .speed_prediction_rpm=0.0f,    /* [待测] 转子 rpm，功率预测增长裕量；取实测预测窗口内最大增长并留裕量。 */

        /* 堵转/温度：不以长时间硬堵转测试阈值；使用受限短脉冲或负载台架。 */
        .stall_current_command=0.0f,   /* [待测] CAN 命令幅值，堵转高电流门槛；结合温升与电流能力确定。 */
        .stall_rpm=0.0f,               /* [待测] 转子 rpm，堵转低速门槛；测静止噪声和低速转动区间。 */
        .stall_timeout_ms=0U,          /* [待测] ms，允许高电流低速持续时间；按短脉冲温升结果确定，1~5000。 */
        .temp_derate_c=0.0f,           /* [待测] 摄氏度，开始降额温度；复核传感器及电机允许温升后留裕量。 */
        .temp_stop_c=0.0f,             /* [待测] 摄氏度，停机温度；必须高于降额温度且不超过代码上限 100。 */

        /* 退出制动：仅 EXIT_MODE=1 使用，但字段集中列出；滑行模式可继续留 0。 */
        .brake_torque_nm=0.0f,         /* [待测] N*m，一次反向制动转矩；低转速测试距离/电流，不得超过旋转上限。 */
        .brake_stop_rad_s=0.0f,        /* [待测] rad/s，停止死区；按静止角速度噪声与撤销制动试验确定。 */
        .brake_timeout_ms=0U,          /* [待测] ms，单次制动超时；按实测制动时间留裕量，1~5000。 */

        /* 输入整形：摇杆径向死区和力曲线；键盘同样经过单位圆限幅与力斜率。 */
        .input_deadzone=0.05f,         /* [初值] 归一化幅值，[0,1)；记录松杆噪声，设置为噪声包络以上。 */
        .input_gamma=1.5f,             /* [初值] 无单位，>0；低输入驾驶试验调节细腻程度。 */

        /* 自适应功率环：软件初值，不是已完成的功率计/整车标定结果。
         * 即使当前使用固定转矩模式，这些字段也需要保持合法正值以通过统一参数校验。
         * 功率计必须位于受控底盘负载侧，先核对零点/方向/采样周期，再做低转矩功率阶跃。
         */
        .power_gain_nm_per_w_s=0.01f,  /* [初值] N*m/(W*s)，功率误差增益；低转矩阶跃整定，观察超调与恢复。 */
        .power_deadband_w=2.0f,        /* [初值] W，功率死区；记录零点和稳态功率噪声后复核。 */
        .power_lpf_tau_s=0.02f,        /* [初值] s，功率跟踪低通；按采样间隔/波动整定，不能代替快保护。 */
        .power_recovery_samples=3U,    /* [初值] 样本数，3~20；连续新鲜功率样本确认，重复样本不计数。 */
        .torque_probe_step_nm=0.05f,   /* [初值] N*m，单次候选转矩增量；低转矩申请试验确认，不能大于转矩上限。 */
        .power_request_step_w=5.0f,    /* [初值] W，申请功率步长；观察能源批准/拒绝，要求 >0 且 <=20。 */
        .grant_timeout_ms=50U,         /* [初值] ms，等待批准超时；测慢任务及输入确认延迟，允许范围 20~1000。 */
    };

    /* [沿用] robot_def.h 的 OMNI_X_*_DIRECTION；单轮短时低电流核对正命令、反馈正转及轮缘方向。
     * 下列 dx/dy 是 X 型轮布局的理论方向；符号必须与最终 CAN 电流一致，不能再由驱动反向一次。 */
    const float direction[4]={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION};
    const float dx[4]={-.7071067812f,.7071067812f,-.7071067812f,.7071067812f};
    const float dy[4]={.7071067812f,.7071067812f,-.7071067812f,-.7071067812f};
    for(unsigned w=0;w<4;++w) {
        /* [沿用] m；WHEEL_BASE/TRACK_WIDTH 原单位为 mm。测轮心位置并用称重/支撑法确认质心；
         * 当前假定质心与几何中心重合。若偏移，改为各轮相对实测质心的 x_m/y_m。 */
        c.wheel[w].x_m=(w<2 ? 1 : -1)*WHEEL_BASE*.0005f;
        c.wheel[w].y_m=(w%2==0 ? 1 : -1)*TRACK_WIDTH*.0005f;
        c.wheel[w].dx=dx[w]*direction[w]; /* [沿用] 无单位，单位向量 x 分量，按上述单轮方向试验核对。 */
        c.wheel[w].dy=dy[w]*direction[w]; /* [沿用] 无单位，单位向量 y 分量；dx^2+dy^2 应约等于 1。 */
        c.wheel[w].radius_m=RADIUS_WHEEL*.001f; /* [沿用] m；原半径为 mm，可用受载轮滚动一周距离/(2*pi)复核。 */
        c.wheel[w].reduction=REDUCTION_RATIO_WHEEL; /* [沿用] 无单位；核对规格或转子/输出轴转数之比。 */
    }
    c.checksum=ChassisForceConfigChecksum(&c); /* 自动校验摘要，必须放在全部参数赋值之后。 */
    return c;
}

/* 能源配置沿用原工程。下面数值/宏是既有策略配置，不表示新测得的物理能力；
 * 更换电机、功率计、能源硬件时，应重新核对对应参数。四轮 k_out 等待测项仍只在上方填写。
 * 能源策略台架复核：同步采集负载侧 P_load、输入侧 P_input、裁判缓冲 J 和电容能量比例；
 * 测预算阶跃、低缓冲恢复及 051/052 失联，确认增补不超过实物能力且下降立即生效。
 * 各 CHASSIS_BUFFER_* 宏沿用 chassis_energy_config.h，并可通过原 Make/CMake 入口覆盖。 */
ChassisPower_Init_Config_s BoardChassisForcePowerConfig(void)
{
    ChassisPower_Init_Config_s c={.stage=3,.force_control=true,.meter_enabled=POWER_METER_ENABLE!=0,
        .buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0,
        .use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP,
        .algorithm={
            .safety_factor=.98f,                 /* [沿用] 无单位，物理功率安全系数；按模型误差复核裕量。 */
            .small_error_threshold_rpm=500,      /* [沿用] rpm，旧速控接口参数，力控不以它生成速度误差。 */
            .reserved_power_threshold_w=54,      /* [沿用] W，旧分配器参数，力控不走旧轮速分配路径。 */
            .per_motor_reserved_power_w=8,       /* [沿用] W，同上，保留共用配置结构兼容性。 */
            .max_current_command=15000,          /* [沿用] CAN 命令幅值，共用功率配置上限；不能代替上方每轮待测限流。 */
        },
        .transient={
            /* [沿用] 与 ChassisPowerTransientDefaultConfig 相同；力控快路径实际读取以下四项。
             * 低功率阶跃验证：超限迅速收紧，恢复需连续新鲜样本，恢复斜率不引入反冲。 */
            .fast_gain=.5f,                      /* 无单位，实测功率超限量到负向快修正的增益。 */
            .release_w_per_s=60,                 /* W/s，负向快修正释放速度。 */
            .release_confirm_ms=10U,             /* ms，解除超功率前低于门槛的连续确认时间。 */
            .stable_run_ms=100U,                 /* ms，驱动稳定/持续超限后允许慢模型学习的时间。 */

            /* [沿用] 下列为共用旧快保护结构的兼容字段，力控不走旧启动峰值或轮速分配器。
             * 力控实际电流、速度及转速预测限制由上方车辆参数决定，不通过此处放宽。 */
            .eta=.95f,                          /* 无单位，旧接口初值；校验时用 algorithm.safety_factor 替换。 */
            .stale_release_w_per_s=20,           /* W/s，旧失联恢复速度；力控失联时不释放负向快修正。 */
            .meter_fresh_ms=20U,                 /* ms，旧功率计新鲜门槛；力控快路径固定检查 <=20 ms。 */
            .feedback_fresh_ms=5U,               /* ms，旧轮反馈门槛；力控快路径固定检查 <=5 ms。 */
            .max_current_command=15000,          /* CAN 命令幅值，旧限流；校验时使用 algorithm.max_current_command。 */
            .acceleration_tau_s=.010f,           /* s，旧转速增长滤波常数，力控使用实测 speed_prediction_rpm。 */
            .prediction_s=.008f,                 /* s，旧转速预测窗口，复核增长裕量时可参考其时长。 */
            .growth_time_s=.005f,                /* s，旧增长兜底时间，力控不使用。 */
            .peak_ratio=1.30f,                   /* 无单位，旧启动峰值比例，力控不开放该额外额度。 */
            .low_rpm=100,                        /* 转子 rpm，旧低速判据，力控不使用。 */
            .low_current=500,                    /* CAN 命令幅值，旧低需求判据，力控不使用。 */
            .target_step_rpm=300,                /* 转子 rpm，旧目标阶跃门槛，力控没有目标轮速。 */
            .zero_enter_rpm=50,                  /* 转子 rpm，旧近零速进入门槛，力控不使用。 */
            .zero_exit_rpm=100,                  /* 转子 rpm，旧近零速退出门槛，力控不使用。 */
            .demand_enter_current=200,           /* CAN 命令幅值，旧需求进入门槛，力控不使用。 */
            .demand_exit_current=100,            /* CAN 命令幅值，旧需求退出门槛，力控不使用。 */
            .low_hold_ms=100U,                   /* ms，旧低需求保持时间，力控不使用。 */
            .startup_window_ms=30U,              /* ms，旧启动峰值窗口，力控不使用。 */
            .startup_cooldown_ms=500U,           /* ms，旧启动峰值冷却时间，力控不使用。 */
            .guard_search_nodes=256U,            /* 搜索节点数，旧 guard 工作量上限，力控使用自己的有界分配器。 */
        },
        .outer={
            .kp=1,                              /* [沿用] 无单位，功率模型慢修正比例增益；功率阶跃整定。 */
            .ki=1.5f,                           /* [沿用] 1/s，慢修正积分增益；观察稳态误差与恢复。 */
            .measurement_lpf_tau_s=.030f,        /* [沿用] s，慢环功率滤波时间常数；依据采样噪声复核。 */
            .activation_margin_w=5,              /* [沿用] W，接近目标功率时的激活裕量。 */
            .error_deadband_w=.3f,               /* [沿用] W，慢修正误差死区；与负载功率计噪声比较。 */
            .max_positive_correction_w=8,        /* [沿用] W，模型正修正上限，不是额外物理能源额度。 */
            .positive_slew_rate_w_per_s=60,      /* [沿用] W/s，正修正恢复斜率；预算阶跃时检查冲击。 */
            .budget_drop_reset_w=1,              /* [沿用] W，预算下降复位阈值；验证下降时不释放旧积分。 */
            .antiwindup_gain_per_s=10,           /* [沿用] 1/s，慢修正抗饱和增益；检查限幅解除后的恢复。 */
            .low_demand_delay_s=.5f,             /* [沿用] s，低需求保持时间，超时后衰减历史修正。 */
            .low_demand_decay_w_per_s=1,         /* [沿用] W/s，低需求修正衰减速度。 */
        },
        .budget={
            .empty_energy_ratio=.1f,             /* [沿用] 无单位，0~1，电容额外负载能力的空电门槛。 */
            .full_energy_ratio=.3f,              /* [沿用] 无单位，0~1，电容增补全部开放门槛；用充放电日志复核。 */
            .max_boost_w=100,                    /* [沿用] W，电容负载侧增补上限；始终受 051 总能力封顶。 */
            .rise_rate_w_per_s=200,              /* [沿用] W/s，负载额度上升斜率，下降不等待斜率。 */
            .demand_margin_w=CHASSIS_POWER_DEMAND_MARGIN_W, /* [沿用] W，真实需求申请裕量，验证零需求不申请。 */
            .buffer={
                .max_energy_j=CHASSIS_BUFFER_MAX_ENERGY_J,                  /* [沿用] J，裁判缓冲最大有效值。 */
                .recovery_enter_j=CHASSIS_BUFFER_RECOVERY_ENTER_J,          /* [沿用] J，进入恢复阈值；低缓冲试验验证。 */
                .recovery_exit_j=CHASSIS_BUFFER_RECOVERY_EXIT_J,            /* [沿用] J，退出恢复阈值，应高于进入值。 */
                .hard_floor_j=CHASSIS_BUFFER_HARD_FLOOR_J,                  /* [沿用] J，预测缓冲硬底座。 */
                .prediction_window_s=CHASSIS_BUFFER_PREDICTION_WINDOW_S,    /* [沿用] s，输入增补能量预测窗口。 */
                .guard_window_s=CHASSIS_BUFFER_GUARD_WINDOW_S,              /* [沿用] s，输入超功率预测保护窗口。 */
                .settlement_delay_s=CHASSIS_BUFFER_SETTLEMENT_DELAY_S,      /* [沿用] s，裁判结算延迟裕量，按实测消息时序复核。 */
                .energy_margin_j=CHASSIS_BUFFER_ENERGY_MARGIN_J,           /* [沿用] J，缓冲估计能量裕量。 */
                .power_margin_w=CHASSIS_BUFFER_POWER_MARGIN_W,             /* [沿用] W，输入功率测量/调度裕量。 */
                .max_boost_w=CHASSIS_BUFFER_BOOST_MAX_W,                    /* [沿用] W，裁判缓冲允许的输入侧增补上限。 */
                .recovery_power_w=CHASSIS_BUFFER_RECOVERY_POWER_W,          /* [沿用] W，恢复时最大输入功率削减量。 */
                .recovery_target_time_s=CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S, /* [沿用] s，缓冲恢复目标时间。 */
                .observer_max_step_s=CHASSIS_BUFFER_OBSERVER_MAX_STEP_S,    /* [沿用] s，输入侧能量观测最大有效步长。 */
                .recovery_hold_s=CHASSIS_BUFFER_RECOVERY_HOLD_S,            /* [沿用] s，退出恢复前连续保持时间。 */
                .rise_rate_w_per_s=CHASSIS_BUFFER_RISE_RATE_W_PER_S,        /* [沿用] W/s，输入额度上升斜率。 */
                .safety_factor=.98f,                                       /* [沿用] 无单位，与功率算法使用同一换算系数。 */
                .correction_rise_j_per_s=CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S, /* [沿用] J/s，缓冲估计向上修正速率。 */
            },
        },
    };
    /* [沿用] 原底盘功率模型系数，不是输出转矩常数。
     * 重新标定：同步记录各轮 CAN 电流命令、转子 rpm 和底盘负载功率；覆盖不同转速/电流，
     * 按原模型联合拟合并用独立数据验证误差。缩放量 x=abs(command/current_conversion)，
     * n=abs(转子 rpm)，P=k0+k1*x+k2*n+k3*x*n+k4*x*x+k5*n*n。
     * x 是原拟合使用的电流缩放量，不是按 20/16384 换算后的物理 A；系数与缩放必须成组使用。 */
    const MotorPowerModelConfig_s m={
        .k0=1.3123816f,       /* [沿用] W，常数损耗项。 */
        .k1=-.25836727f,      /* [沿用] W/缩放电流单位，一次电流项。 */
        .k2=.0007424524f,     /* [沿用] W/rpm，一次转速项。 */
        .k3=.0024271158f,     /* [沿用] W/(缩放电流单位*rpm)，电流转速交叉项。 */
        .k4=.081198552f,      /* [沿用] W/(缩放电流单位^2)，二次电流项。 */
        .k5=3.890596e-8f,     /* [沿用] W/rpm^2，二次转速项。 */
        .current_conversion=1000.0f, /* [沿用] 每个缩放电流单位对应的 CAN 命令值，不能单改而保留系数。 */
    };
    for(unsigned w=0;w<4;++w) c.models[w]=m;
    return c;
}
