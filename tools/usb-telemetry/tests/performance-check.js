// Run with Playwright CLI run-code --filename against the local telemetry page.
async (page) => {
  const check = (condition, message) => {if (!condition) throw new Error(message);};
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.goto('http://127.0.0.1:4173');
  await page.setViewportSize({width: 1440, height: 1050});
  await page.locator('#demo-button').click();
  await page.waitForFunction(() => telemetryApp.store.samples.length > 29000);
  const settle = () => page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
  const activity = () => page.evaluate(() => new Promise(resolve => {
    const original = TelemetryCharts.drawChart, counts = {};
    TelemetryCharts.drawChart = (canvas, ...args) => {
      counts[canvas.id] = (counts[canvas.id] || 0) + 1;
      return original(canvas, ...args);
    };
    let frames = 0;
    const start = performance.now();
    function observe(now) {
      frames++;
      if (now - start < 500) requestAnimationFrame(observe);
      else {TelemetryCharts.drawChart = original;resolve({counts, frames, elapsed: now - start});}
    }
    requestAnimationFrame(observe);
  }));
  await settle();
  const live = await activity();
  const targetFrames = Math.min(live.frames, live.elapsed * 60 / 1000);
  check(live.counts['power-chart'] >= targetFrames * .75, 'Visible waveform still refreshes below the display cadence: ' + JSON.stringify(live));
  const offscreen = await page.locator('#imu-accel-chart').evaluate(node => node.getBoundingClientRect().top >= innerHeight);
  check(offscreen && !live.counts['imu-accel-chart'], 'Offscreen waveform was redrawn');

  await page.getByRole('button', {name: '放大功率观测', exact: true}).click();
  await settle();
  const expanded = await activity();
  check(expanded.counts['expanded-chart'] > 0 && Object.keys(expanded.counts).length === 1, 'Modal must draw only its visible waveform');
  await page.keyboard.press('Escape');
  await page.locator('#pause-button').click();
  await settle();
  const paused = await activity();
  check(Object.keys(paused.counts).length === 0, 'Paused waveforms redraw without an interaction');
  await page.locator('#imu-accel-chart').scrollIntoViewIfNeeded();
  await page.waitForFunction(() => document.querySelector('#imu-accel-chart').getAttribute('aria-label')?.includes('个样本'));
  await page.locator('#pause-button').click();
  await page.locator('#tuning-tab').click();
  await settle();
  check(Object.keys((await activity()).counts).length === 0, 'Hidden monitor workspace keeps drawing');
  // Replay the actual app loop on displays whose refresh rate is not a multiple of 60.
  const cadencePage = await page.context().newPage();
  cadencePage.on('pageerror', error => errors.push(error.message));
  await cadencePage.addInitScript(() => {
    let callbacks = [];
    window.requestAnimationFrame = callback => {callbacks.push(callback);return callbacks.length;};
    window.__stepWaveformFrame = now => {
      const pending = callbacks;callbacks = [];
      pending.forEach(callback => callback(now));
    };
  });
  await cadencePage.goto(page.url());
  await cadencePage.locator('#demo-button').click();
  await cadencePage.getByRole('button', {name: '放大功率观测', exact: true}).click();
  const cadence = await cadencePage.evaluate(() => {
    const original = TelemetryCharts.drawChart, results = {};
    let count = 0, start = performance.now();
    TelemetryCharts.drawChart = () => {count++;};
    for (const hz of [60, 75, 90, 120, 144, 240]) {
      __stepWaveformFrame(start);count = 0;
      for (let frame = 1; frame <= hz * 2; frame++) __stepWaveformFrame(start + frame * 1000 / hz);
      results[hz] = count / 2;start += 3000;
    }
    TelemetryCharts.drawChart = original;
    return results;
  });
  await cadencePage.close();
  for (const [hz, fps] of Object.entries(cadence)) check(fps >= 57 && fps <= 62, `${hz}Hz display renders at ${fps}fps`);
  check(errors.length === 0, errors.join('; '));
  return {passed: true, live, cadence, cases: ['display cadence', 'offscreen charts', 'modal isolation', 'pause', 'scroll while paused', 'hidden workspace', '60/75/90/120/144/240Hz displays']};
}
