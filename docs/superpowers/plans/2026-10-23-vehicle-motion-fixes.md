# 整车运动控制缺口修复与验证

用户已授权修复前次检查确认的五项问题，验证后本地提交。保留现有底盘 Kp=0.75 和 CHASSIS_POWER_BENCH_TEST=1；这两项已有改动不纳入本次提交。

## 实施范围

- [x] 新增 `tests/vehicle_motion` 主机集成测试，直接包含真实 CMD、云台和发射应用；仅替换硬件与消息传输。五项原始缺陷均先观察到断言失败，再修改实现。
- [x] 云台原子复制 Yaw 编码器采样状态与时间戳；CMD 将未采样或超过 20 ms 的反馈纳入整车安全锁存。恢复反馈后仍需中立复位。
- [x] 图传 C 键四档对平移速度施加 40/60/80/100% 系数；遥控器摇杆比例不变，旋转目标不受档位影响。
- [x] 视觉中档暂时继续使用人工摇杆，不让未实现的视觉分支屏蔽控制。
- [x] 单发/三连发要求有效热量、足够整组热量及原有七发储备；所有拨弹要求摩擦轮和拨盘反馈新鲜、摩擦轮实际转速连续 100 ms 处于目标的 ±10%。
- [x] 热量、反馈、摩擦轮开关或急停条件失效时取消批次，重新满足条件不会恢复已取消的按键请求。单/三发需要松开后再次按下。
- [x] 离散批次结束后，下一批次或连发必须等待超过 300 ms，且热量源时间戳晚于上次拨弹 300 ms；连发切离散批次同样等待。防止在热量尚未上报时重复使用额度。此等待可能增加操作延迟，是本次采用的保守策略。
- [x] 通过 10 项应用集成场景及仓库 40 组主机测试；独立只读代码审查未发现 P1/P2 缺陷。
- [x] 整车、台架、直供三种配置均在新目录完整构建成功，无编译 warning/error。

## 验证命令

```powershell
make -B -C tests/vehicle_motion test CC=gcc
# 对 tests 下全部 40 个 Makefile 执行：make -B -C <测试目录> test CC=gcc
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/motion-review-vehicle
make -j8 PROFILE=atype-bench BUILD_DIR=build/motion-review-atype-bench
make -j8 PROFILE=atype-direct BUILD_DIR=build/motion-review-atype-direct
```

主机测试记录位于 `build/motion-test-*.log`，固件构建记录位于 `build/motion-review-*.log`。生成产物不入库。

本次未烧录或操控实物。摩擦轮速度容差与稳定等待、反馈方向、机械限位、实际热量延迟仍需要空载和实机验收；主机测试不证明机械系统已验收。
