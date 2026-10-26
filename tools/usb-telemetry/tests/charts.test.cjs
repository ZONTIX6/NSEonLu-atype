'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

function fixture(width = 180) {
  class Element {
    constructor() {this.children = [];this.style = {};this.listeners = {};}
    append(...children) {this.children.push(...children);}
    replaceChildren(...children) {this.children = children;}
    setAttribute() {}
    addEventListener(name, handler) {this.listeners[name] = handler;}
  }
  const paths = [], frames = [];
  let path;
  const context = new Proxy({
    beginPath() {path = [];},
    moveTo(x, y) {path.push({move: true, x, y});},
    lineTo(x, y) {path.push({move: false, x, y});},
    stroke() {paths.push({color: context.strokeStyle, points: path});}
  }, {get: (target, key) => target[key] ?? (() => {})});
  const canvas = new Element();
  canvas.parentElement = new Element();
  canvas.getBoundingClientRect = () => ({left: 0, top: 0, width, height: 240});
  canvas.getContext = () => context;
  const root = {window: {devicePixelRatio: 1}, document: {
    createElement: () => new Element(), createTextNode: text => text
  }, requestAnimationFrame: callback => {frames.push(callback);return frames.length;}};
  vm.runInNewContext(fs.readFileSync(require.resolve('../charts.js'), 'utf8'), root);
  const draw = samples => root.TelemetryCharts.drawChart(canvas, samples,
    [{key: 'value', name: 'Value', color: 'trace'}], {now: 10000, range: [-10000, 0]});
  const points = () => paths.filter(path => path.color === 'trace').at(-1).points;
  return {canvas, draw, points, frames};
}
const sample = (arrivalMs, value = 0, segment = 0) => ({arrivalMs, segment, tick: arrivalMs,
  normalized: {values: {value}}});

test('small visible windows do not scan the entire ten-minute history', () => {
  const {draw} = fixture();
  let reads = 0;
  const samples = Array.from({length: 30000}, (_, i) => ({...sample(i * 20),
    get arrivalMs() {reads++;return i * 20;}}));
  draw(samples);
  assert.ok(reads < 5000, `Read ${reads} timestamps for only 501 visible samples`);
});

test('dense paths retain first, last and extrema within a pixel', () => {
  const {draw, points} = fixture();
  const samples = Array.from({length: 10001}, (_, i) => sample(i));
  samples[5001] = sample(5001, 100);
  samples[5002] = sample(5002, -100);
  draw(samples);
  const path = points();
  assert.ok(path.length <= 4 * 127, `Drew ${path.length} points for a 126px plot`);
  assert.equal(path[0].x, 42);
  assert.equal(path.at(-1).x, 168);
  assert.ok(path.some(point => point.y < 50), 'Positive spike disappeared');
  assert.ok(path.some(point => point.y > 180), 'Negative spike disappeared');
  assert.equal(samples.length, 10001);
  assert.equal(samples[5001].normalized.values.value, 100);
});

test('pixel reduction preserves invalid values, segment boundaries and time gaps', () => {
  const {draw, points} = fixture();
  draw([sample(100), sample(101, 10), sample(102, null), sample(103, 30),
    sample(104, 40, 1), sample(900, 50, 1)]);
  assert.equal(points().filter(point => point.move).length, 4);
});

test('empty windows and duplicate timestamps remain valid', () => {
  const {draw, points} = fixture();
  draw([]);
  assert.equal(points().length, 0);
  draw([sample(0, 2), sample(0, 5), sample(10000, 4)]);
  assert.equal(points().length, 3);
});
