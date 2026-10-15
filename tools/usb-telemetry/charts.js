(function (root) {
  'use strict';
  function drawChart(canvas, samples, traces, {now, unit = ''} = {}) {
    const bounds = canvas.getBoundingClientRect();
    const width = bounds.width, height = bounds.height;
    if (!width || !height) return;
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    if (canvas.width !== Math.round(width * ratio) || canvas.height !== Math.round(height * ratio)) {canvas.width = Math.round(width * ratio); canvas.height = Math.round(height * ratio);}
    const ctx = canvas.getContext('2d');
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    ctx.clearRect(0, 0, width, height);
    const pad = {left: 42, right: 12, top: 12, bottom: 25}, plotWidth = width - pad.left - pad.right, plotHeight = height - pad.top - pad.bottom;
    const end = now, start = end - 60000;
    const visible = samples.filter(sample => sample.arrivalMs >= start && sample.arrivalMs <= end);
    let low = 0, high = 1;
    for (const sample of visible) for (const trace of traces) {
      const value = sample.normalized.values[trace.key];
      if (value !== null && Number.isFinite(value)) {low = Math.min(low, value); high = Math.max(high, value);}
    }
    const spread = high - low || 1;
    if (low < 0) low -= spread * .09;
    high += spread * .12;
    const y = value => pad.top + plotHeight * (1 - (value - low) / (high - low));
    const x = time => pad.left + (time - start) / 60000 * plotWidth;
    ctx.font = '9px "Segoe UI", sans-serif';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 4; i++) {
      const lineY = pad.top + i / 4 * plotHeight;
      ctx.strokeStyle = '#edf1f6';
      ctx.beginPath();ctx.moveTo(pad.left, lineY);ctx.lineTo(width - pad.right, lineY);ctx.stroke();
      ctx.fillStyle = '#a4b0c1';ctx.textAlign = 'right';ctx.textBaseline = 'middle';
      const value = high - (high - low) * i / 4;
      ctx.fillText(Math.abs(high) < 10 ? value.toFixed(1) : String(Math.round(value)), pad.left - 8, lineY);
    }
    for (let i = 0; i <= 4; i++) {
      const lineX = pad.left + i / 4 * plotWidth;
      ctx.strokeStyle = '#f3f5f9';ctx.beginPath();ctx.moveTo(lineX, pad.top);ctx.lineTo(lineX, height - pad.bottom);ctx.stroke();
      ctx.fillStyle = '#a4b0c1';ctx.textAlign = i === 0 ? 'left' : i === 4 ? 'right' : 'center';ctx.textBaseline = 'bottom';ctx.fillText(i === 4 ? '现在' : `−${60 - i * 15}s`, lineX, height - 3);
    }
    ctx.save();ctx.beginPath();ctx.rect(pad.left, pad.top, plotWidth, plotHeight);ctx.clip();
    for (const trace of traces) {
      ctx.beginPath();ctx.strokeStyle = trace.color;ctx.lineWidth = trace.dash ? 1.2 : 1.6;ctx.setLineDash(trace.dash ? [4, 4] : []);
      let previous = null;
      for (const sample of visible) {
        const value = sample.normalized.values[trace.key];
        if (value === null || !Number.isFinite(value)) {previous = null; continue;}
        const pointX = x(sample.arrivalMs), pointY = y(value);
        if (!previous || previous.segment !== sample.segment || sample.arrivalMs - previous.arrivalMs > 500) ctx.moveTo(pointX, pointY);
        else ctx.lineTo(pointX, pointY);
        previous = sample;
      }
      ctx.stroke();
      if (previous && end - previous.arrivalMs <= 500) {
        ctx.setLineDash([]);ctx.beginPath();ctx.arc(x(previous.arrivalMs), y(previous.normalized.values[trace.key]), 2.2, 0, Math.PI * 2);ctx.fillStyle = trace.color;ctx.fill();
      }
    }
    ctx.restore();ctx.setLineDash([]);
    canvas.setAttribute('aria-label', `${unit} 趋势，保留最近 60 秒，${visible.length} 个样本。`);
  }
  root.TelemetryCharts = {drawChart};
})(globalThis);
