#include "chassis_force_vehicle_config.h"

/* [沿用] config/01_pid/pid_config.c 的 pid_chassis_speed_config.MaxOut 和原功率模块上限均为 15000。
 * 这是已用的 CAN 命令幅值上限，不等于新完成的持续电流/温升标定；不据此放宽其他保护。 */
#define BOARD_CHASSIS_LEGACY_CURRENT_LIMIT 15000.0f

/*
 * 车辆力控参数填写入口：只在本函数填写车辆参数，不修改 modules/chassis_force 内部算法。
 * 方案/启用：docs/chassis-direct-force.md；实测/无测力计替代方法/记录表：docs/chassis-force-calibration.md。
 *
 * 标记约定：
 *   [沿用] 原工程已有值，注明来源；装配、轮胎或重心改变后需要复核。
 *   [待测] 无法从现有工程确定的物理标定量，继续留 0；测量并验收后填写。
 *   [初值] 操作幅度、斜率及调节参数，已经给出调试起点，需通过台架整定。
 *   [调试限值] 人为设定的较低调试边界，不是实测机械极限或已验收的安全能力。
 *
 * 当前仍缺三组实测数据：四轮 k_out_nm_a、四轮 force_limit_n、speed_prediction_rpm。
 * 填入调试初值不会解除锁定；这些待测项为 0 或有效标志为 0 时仍然禁止驱动。
 * 速度/温度/堵转调试限值需要上板核对反馈、检测延迟及温升，不能据此宣称已完成验收。
 *
 * 单位约定：车体 x 向前、y 向左，正转矩为俯视逆时针；位置相对车辆质心。
 * 四轮顺序固定为 LF 左前、RF 右前、LB 左后、RB 右后，对应 CAN1 电调 ID 1、4、2、3。
 * CAN 电流命令不是 A：I_cmd_A = command * 20 / 16384；反馈电流字段的标度需另外核对。
 *
 * 标定顺序：核对轮序/方向/几何 -> 受限单轮电流台架测 K_out -> 测轮力/预测裕量并复核电流/保护设置
 *           -> 低力平移与固定转矩 -> 功率自适应 -> 可选退出制动。
 * 当前没有独立单轮标定入口；不要通过伪填参数来绕过未标定锁定。
 * 全部必填项验收后，再通过构建参数设置 CHASSIS_FORCE_CALIBRATION_VALID=1。
 * 只改有效标志仍不能通过参数检查；修改任意参数后，必须在函数末尾重新计算 checksum。
 */
ChassisForceConfig_s ChassisForceVehicleConfig(void)
{
    ChassisForceConfig_s c={
        .backend=CHASSIS_CONTROL_FORCE, /* 协议后端标识，无单位；固定为 FORCE，不是调参项。 */
        .schema_version=1U,             /* 参数结构版本，无单位；修改结构时单独升级。 */
        .calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0, /* 标定验收标志；默认宏值为 0。 */

        /* 四轮逐轮填写。共同标定方法：
         * k_out_nm_a：输出轴接已知力臂 L(m)，测力 F(N)，tau=F*L；多个低电流正/反向
         *             点拟合 tau-I 的斜率，单位 N*m/A。它是减速器输出端常数，不再乘减速比。
         *             电源输入电流不能代替电机转矩电流；检查摩擦、偏置及正反向拟合误差。
         * current_limit：沿用原速控/功率配置的 15000；更换硬件或调整持续负载时复核电流/温升，
         *                不得超过 16384；起步、堵转和温度保护仍需单独填写。
         * force_limit_n：用测力装置和低力加载验证可重复传递的轮缘力，考虑接地滑移后留裕量；
         *                架空轮速或输出轴转矩不能证明真实接地力。
         * current_slew：初值按原电流幅值上限除以 1 s，表示 0 到旧上限至少约 1 s；
         *               单轮低电流斜坡记录功率/冲击后调整，不是已测得的电流响应斜率。
         */
        .wheel={
            [0]={ /* LF 左前，CAN1 ID 1。几何在下方从原车配置装配。 */
                .k_out_nm_a=0.0f,   /* [待测] N*m/A，输出轴转矩常数，按上述力臂法拟合。 */
                .current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */
                .force_limit_n=0.0f, /* [待测] N，保守可传递轮缘力，低力加载验证。 */
                .current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */
            },
            [1]={ /* RF 右前，CAN1 ID 4。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右前轮组。 */
                .current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */
            },
            [2]={ /* LB 左后，CAN1 ID 2。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量左后轮组。 */
                .current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */
            },
            [3]={ /* RB 右后，CAN1 ID 3。 */
                .k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右后轮组。 */
                .current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */
                .force_limit_n=0.0f, /* [待测] N。 */
                .current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */
            },
        },

        /* 车体力/转矩是可选择的操作幅度，不是电机或轮地能力的标定常数。
         * 以下采用低幅起点；物理输出仍受逐轮力/电流、速度及功率限制，不从旧目标轮速换算。 */
        .force_max_n=10.0f,            /* [初值] N，满输入合力；低力加载/加速试验后调整。 */
        .spin_torque_nm=0.3f,          /* [初值] N*m，固定旋转请求；用电流/负载功率日志复核。 */
        .spin_start_torque_nm=0.1f,    /* [初值] N*m，自适应起步请求；不保证起转，也不强制最小输出。 */
        .spin_torque_max_nm=1.0f,      /* [调试限值] N*m，自适应请求上限；不是最大物理转矩的测量值。 */
        .force_rise_n_s=20.0f,         /* [初值] N/s，0 到 10 N 约 0.5 s；阶跃试验核对冲击。 */
        .force_fall_n_s=40.0f,         /* [初值] N/s，10 N 撤至 0 约 0.25 s；松杆试验调整。 */
        .torque_rise_nm_s=0.5f,        /* [初值] N*m/s，0 到固定 0.3 N*m 约 0.6 s。 */
        .torque_fall_nm_s=1.0f,        /* [初值] N*m/s，固定 0.3 N*m 撤至 0 约 0.3 s。 */

        /* 速度只作保护，不是正常驱动力的目标值。轮级 rpm 一律指电机转子。
         * 原工程未配置经过验证的机械硬上限；此处人为限制低速调试，仍需确认估计方向和实际峰值。 */
        .velocity_limit_mps=0.3f,      /* [调试限值] m/s，平移保护；外部测速核对，不是车体最高速度。 */
        .omega_limit_rad_s=1.0f,       /* [调试限值] rad/s，旋转保护；独立测速核对撤力延迟。 */
        .wheel_limit_rpm=1000.0f,      /* [调试限值] 转子 rpm，轮级保护；不是轮子输出轴 rpm。 */
        .resume_ratio=0.9f,            /* [初值] 无单位，(0,1)；降至上限乘此比例后恢复，检查有无抖动。 */
        .startup_current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT*0.1f, /* [调试限值] CAN 命令幅值，旧上限的 10%；验证起步。 */
        .speed_prediction_rpm=0.0f,    /* [待测] 转子 rpm，功率预测增长裕量；取实测预测窗口内最大增长并留裕量。 */

        /* 堵转/温度：不以长时间硬堵转测试阈值；使用受限短脉冲或负载台架。 */
        .stall_current_command=1000.0f, /* [调试限值] CAN 命令幅值；低于起步上限 1500，使近零速堵转检测可触发。 */
        .stall_rpm=20.0f,              /* [初值] 转子 rpm，低速判据；记录静止噪声后复核，可能误触发。 */
        .stall_timeout_ms=100U,        /* [调试限值] ms，达到上述电流且低速时的连续确认时间；不是允许堵转时长的实测结果。 */
        .temp_derate_c=50.0f,          /* [调试限值] 摄氏度，提前降额；先核对温度反馈与环境温度。 */
        .temp_stop_c=60.0f,            /* [调试限值] 摄氏度，停机；不是电机额定耐温，按温升试验复核。 */

        /* 退出制动：只预填参数，不改变默认 EXIT_MODE=0（滑行）。启用制动后须验证不反向持续驱动。 */
        .brake_torque_nm=0.2f,         /* [初值] N*m，单次反向请求；低转速核对制动距离、电流与功率。 */
        .brake_stop_rad_s=0.1f,        /* [初值] rad/s，停止死区；静止噪声与撤销制动试验后调整。 */
        .brake_timeout_ms=300U,        /* [调试限值] ms，单次制动最多持续时间；不承诺在此时间内停稳。 */

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

    /* [沿用] chassis_config.h 的 OMNI_X_*_DIRECTION；单轮短时低电流核对正命令、反馈正转及轮缘方向。
     * 下列 dx/dy 是 X 型轮布局的理论方向；符号必须与最终 CAN 电流一致，不能再由驱动反向一次。 */
    const float direction[4]={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION}; /* 四轮正命令方向校正，无单位，顺序 LF/RF/LB/RB。 */
    const float dx[4]={-.7071067812f,.7071067812f,-.7071067812f,.7071067812f}; /* X 型轮理论方向的前向分量，无单位。 */
    const float dy[4]={.7071067812f,.7071067812f,-.7071067812f,-.7071067812f}; /* X 型轮理论方向的左向分量，无单位。 */
    for(unsigned w=0;w<4;++w) {
        /* [沿用] m；WHEEL_BASE/TRACK_WIDTH 原单位为 mm。测轮心位置并用称重/支撑法确认质心；
         * 当前假定质心与几何中心重合。若偏移，改为各轮相对实测质心的 x_m/y_m。 */
        c.wheel[w].x_m=(w<2 ? 1 : -1)*WHEEL_BASE*.0005f; /* 前后轮的纵向轮心坐标，单位 m。 */
        c.wheel[w].y_m=(w%2==0 ? 1 : -1)*TRACK_WIDTH*.0005f; /* 左右轮的横向轮心坐标，单位 m。 */
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
ChassisPower_Init_Config_s ChassisForceVehiclePowerConfig(void)
{
    ChassisPower_Init_Config_s c={
        .stage=3, /* 功控阶段：固定为 3 完整慢环；直接力控不支持较早阶段。 */
        .force_control=true, /* 后端标志：true 使用直接力控快路径，不是构建后端选择开关。 */
        .meter_enabled=POWER_METER_ENABLE!=0, /* 负载功率计是否启用；由宏控制，默认启用。 */
        .buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0, /* 裁判缓冲增补开关：0 关闭、1 启用，默认 1。 */
        .use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP, /* 是否按超级电容供电策略处理；默认直供为 false。 */
        .algorithm={ /* 共用功率分配接口的数值配置。 */
            .safety_factor=.98f,                 /* [沿用] 无单位，物理功率安全系数；按模型误差复核裕量。 */
            .small_error_threshold_rpm=500,      /* [沿用] rpm，旧速控接口参数，力控不以它生成速度误差。 */
            .reserved_power_threshold_w=54,      /* [沿用] W，旧分配器参数，力控不走旧轮速分配路径。 */
            .per_motor_reserved_power_w=8,       /* [沿用] W，同上，保留共用配置结构兼容性。 */
            .max_current_command=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，与四轮原限流保持一致。 */
        },
        .transient={ /* 快速超功率保护及旧速度后端兼容参数。 */
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
            .max_current_command=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* CAN 命令幅值，旧限流；校验时使用 algorithm 值。 */
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
        .outer={ /* 模型误差的慢速预算修正参数。 */
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
        .budget={ /* 负载预算与输入侧缓冲预算；两层功率分别限幅。 */
            .empty_energy_ratio=.1f,             /* [沿用] 无单位，0~1，电容额外负载能力的空电门槛。 */
            .full_energy_ratio=.3f,              /* [沿用] 无单位，0~1，电容增补全部开放门槛；用充放电日志复核。 */
            .max_boost_w=100,                    /* [沿用] W，电容负载侧增补上限；始终受 051 总能力封顶。 */
            .rise_rate_w_per_s=200,              /* [沿用] W/s，负载额度上升斜率，下降不等待斜率。 */
            .demand_margin_w=CHASSIS_POWER_DEMAND_MARGIN_W, /* [沿用] W，真实需求申请裕量，验证零需求不申请。 */
            .buffer={ /* 裁判缓冲能量观测和恢复策略。 */
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
