// Run with Playwright CLI run-code --filename against port 4173.
async (page) => {
  const check = (condition, message) => {if (!condition) throw new Error(message);};
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.goto('http://127.0.0.1:4173');
  check(await page.locator('#imu-status').innerText() === '等待数据', 'IMU needs an empty state');
  await page.evaluate(() => {
    window.__imuRow = (version = 2, extras = {}) => {
      const P = TelemetryProtocol, fields = version === 1 ? P.V1_FIELDS : P.FIELDS;
      const row = {...Object.fromEntries(fields.map(key => [key, 0])), version, seq: 1, tick: 1000, chassis_valid: 1, chassis_tick_ms: 1000, chassis_mode: 3, imu_valid: 1, imu_age_ms: 2, imu_yaw_mdeg: -12345, imu_pitch_mdeg: 1250, imu_roll_mdeg: -2500, imu_yaw_total_mdeg: 721234, imu_gx_mdps: -180500, imu_gy_mdps: 250, imu_gz_mdps: 90000, imu_ax_mmps2: -1500, imu_ay_mmps2: 2000, imu_az_mmps2: 9807, chassis_offset_mdeg: -30500, wz_cmd_milli: 1525000, ...extras};
      telemetryApp.parser.push(fields.join(',') + '\r\n' + fields.map(key => row[key]).join(',') + '\r\n');
    };
    __imuRow(1);
  });
  await page.waitForFunction(() => document.querySelector('#imu-status').textContent.includes('未提供'));
  check(await page.locator('#imu-yaw').innerText() === '—', 'Legacy firmware must not show a fabricated IMU zero');
  await page.evaluate(() => __imuRow(2, {seq: 2}));
  await page.waitForFunction(() => document.querySelector('#imu-yaw').textContent === '-12.35');
  check(await page.locator('#imu-gx').innerText() === '-180.50', 'Gyro units incorrect');
  check(await page.locator('#imu-az').innerText() === '9.807', 'Acceleration units incorrect');
  check(await page.locator('#follow-offset').innerText() === '-30.50', 'Follow input not rendered');
  check((await page.locator('#follow-diagnostic').innerText()).includes('Yaw 电机编码器'), 'Follow diagnosis must name the actual input');
  await page.locator('#pause-button').click();
  await page.evaluate(() => __imuRow(2, {seq: 3, imu_yaw_mdeg: 45000}));
  check(await page.locator('#imu-yaw').innerText() === '-12.35', 'Pause failed to freeze IMU');
  await page.locator('#pause-button').click();
  await page.waitForFunction(() => document.querySelector('#imu-yaw').textContent === '45.00');
  await page.evaluate(() => __imuRow(2, {seq: 4, imu_age_ms: 11}));
  await page.waitForFunction(() => document.querySelector('#imu-yaw').textContent === '—');
  check((await page.locator('#imu-status').innerText()).includes('过期'), 'IMU invalid needs a status');
  await page.locator('#demo-button').click();
  await page.waitForFunction(() => document.querySelector('#imu-status').textContent === '在线');
  await page.locator('#pause-button').click();
  for (const name of ['IMU 姿态', 'IMU 角速度', 'IMU 加速度']) {
    await page.getByRole('button', {name: `放大${name}`, exact: true}).click();
    check(await page.getByRole('dialog', {name}).isVisible(), `${name} did not expand`);
    await page.keyboard.press('Escape');
  }
  const downloadEvent = page.waitForEvent('download');
  await page.locator('#export-button').click();
  const stream = await (await downloadEvent).createReadStream();
  let csv = '';for await (const chunk of stream) csv += chunk.toString();
  check(csv.split('\r\n')[0].includes('imu_az_mmps2,chassis_offset_mdeg'), 'IMU export fields missing');
  await page.setViewportSize({width: 1440, height: 1050});
  await page.locator('#imu-card').screenshot({path: 'output/playwright/imu-readouts.png'});
  await page.screenshot({path: 'output/playwright/imu-dashboard-desktop.png', fullPage: true});
  for (const width of [768, 375, 320]) {
    await page.setViewportSize({width, height: 900});
    check(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth), `Layout overflows at ${width}px`);
  }
  await page.screenshot({path: 'output/playwright/imu-dashboard-mobile.png', fullPage: true});
  check(errors.length === 0, errors.join('; '));
  return {passed: true, cases: ['empty', 'v1', 'v2 signed units', 'follow input', 'pause', 'IMU stale', 'demo', 'three expanded charts', 'CSV export', '320–1440px']};
}
