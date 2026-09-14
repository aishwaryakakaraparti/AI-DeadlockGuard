/**
 * dashboard.js — Phase 14
 *
 * Vanilla JS live engine for AI-DeadlockGuard dashboard.
 * Four self-contained modules:
 *   1. ThemeManager  — dark/light toggle, persisted to localStorage
 *   2. RiskGauge     — animated SVG arc, colour transitions
 *   3. RiskChart     — scrolling Canvas area chart, no dependencies
 *   4. SSEClient     — EventSource('/stream') driving all live UI updates
 */

'use strict';

/* ═══════════════════════════════════════════════════════════════════════════
   1. THEME MANAGER
   ═══════════════════════════════════════════════════════════════════════════ */
const ThemeManager = (() => {
  const KEY = 'adg-theme';
  let current = 'dark';

  function apply(theme) {
    current = theme;
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem(KEY, theme);
    // Update toggle buttons
    document.querySelectorAll('.theme-btn').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.theme === theme);
    });
  }

  function init() {
    const saved = localStorage.getItem(KEY) || 'dark';
    apply(saved);
    document.querySelectorAll('.theme-btn').forEach(btn => {
      btn.addEventListener('click', () => apply(btn.dataset.theme));
    });
  }

  return { init, apply, get: () => current };
})();


/* ═══════════════════════════════════════════════════════════════════════════
   2. RISK GAUGE
   SVG arc: circumference = 2π × r  (r = 90)
   We use 75% of the full circle so it looks like a classic speedometer.
   ═══════════════════════════════════════════════════════════════════════════ */
const RiskGauge = (() => {
  const RADIUS        = 90;
  const CIRCUMFERENCE = 2 * Math.PI * RADIUS;
  const ARC_FRACTION  = 0.75; // use 75% of full circle
  const ARC_LENGTH    = CIRCUMFERENCE * ARC_FRACTION;

  let arcEl, scoreEl, statusEl;

  function _colorForRisk(score) {
    if (score < 0.4) return '#10b981';          // green
    if (score < 0.65) return '#f59e0b';          // amber
    if (score < 0.8) return '#f97316';           // orange
    return '#ef4444';                             // red
  }

  function _statusForRisk(score) {
    if (score < 0.4) return { label: 'SAFE',     cls: 'safe'    };
    if (score < 0.75) return { label: 'WARNING',  cls: 'warning' };
    return                   { label: 'DEADLOCK', cls: 'danger'  };
  }

  function init() {
    arcEl    = document.getElementById('gaugeArc');
    scoreEl  = document.getElementById('gaugeScore');
    statusEl = document.getElementById('gaugeStatus');

    // Set the arc total dasharray
    arcEl.setAttribute('stroke-dasharray', `${ARC_LENGTH} ${CIRCUMFERENCE - ARC_LENGTH}`);
    // Start rotated so arc begins at bottom-left (220°)
    arcEl.parentElement.style.transform = 'rotate(135deg)';

    update(0);
  }

  function update(score) {
    // score: 0..1 float
    const pct    = Math.min(Math.max(score, 0), 1);
    const offset = ARC_LENGTH * (1 - pct);
    const color  = _colorForRisk(pct);
    const status = _statusForRisk(pct);

    arcEl.style.strokeDashoffset = offset;
    arcEl.style.stroke = color;

    scoreEl.textContent = `${Math.round(pct * 100)}%`;
    scoreEl.style.color = color;

    statusEl.textContent = status.label;
    statusEl.className = `gauge-status ${status.cls}`;
  }

  return { init, update };
})();


/* ═══════════════════════════════════════════════════════════════════════════
   3. RISK CHART  —  Canvas-based scrolling area chart
   Stores the last MAX_POINTS ticks and redraws on every update.
   ═══════════════════════════════════════════════════════════════════════════ */
const RiskChart = (() => {
  const MAX_POINTS = 60;  // 60 × 0.5s = 30 second window
  let canvas, ctx;
  let history = [];

  function _draw() {
    const W = canvas.width;
    const H = canvas.height;
    const theme = ThemeManager.get();

    ctx.clearRect(0, 0, W, H);

    // Background
    ctx.fillStyle = theme === 'dark' ? '#010409' : '#f0f2f4';
    ctx.fillRect(0, 0, W, H);

    // Grid lines (horizontal at 25%, 50%, 75%, 100%)
    const gridColor = theme === 'dark' ? 'rgba(255,255,255,0.06)' : 'rgba(0,0,0,0.06)';
    const labelColor = theme === 'dark' ? '#4b5563' : '#9ca3af';
    ctx.strokeStyle = gridColor;
    ctx.lineWidth = 1;
    ctx.font = '10px system-ui, sans-serif';
    ctx.fillStyle = labelColor;
    ctx.textAlign = 'right';
    [0, 25, 50, 75, 100].forEach(pct => {
      const y = H - (pct / 100) * (H - 20) - 4;
      ctx.beginPath();
      ctx.moveTo(32, y);
      ctx.lineTo(W, y);
      ctx.stroke();
      ctx.fillText(`${pct}%`, 28, y + 4);
    });

    if (history.length < 2) return;

    const pts = history.slice(-MAX_POINTS);
    const stepX = (W - 32) / (MAX_POINTS - 1);

    const toXY = (i, val) => ({
      x: 32 + i * stepX,
      y: H - (val * (H - 20)) - 4
    });

    // Latest risk drives the gradient colour
    const latest = pts[pts.length - 1];
    let lineColor = '#10b981';
    if (latest >= 0.75) lineColor = '#ef4444';
    else if (latest >= 0.4) lineColor = '#f59e0b';

    // Area fill with vertical gradient
    const grad = ctx.createLinearGradient(0, 0, 0, H);
    grad.addColorStop(0, lineColor + 'aa');
    grad.addColorStop(1, lineColor + '08');

    ctx.beginPath();
    const first = toXY(0, pts[0]);
    ctx.moveTo(first.x, H - 4);
    ctx.lineTo(first.x, first.y);

    for (let i = 1; i < pts.length; i++) {
      const prev = toXY(i - 1, pts[i - 1]);
      const curr = toXY(i, pts[i]);
      const cpx  = (prev.x + curr.x) / 2;
      ctx.bezierCurveTo(cpx, prev.y, cpx, curr.y, curr.x, curr.y);
    }

    const last = toXY(pts.length - 1, pts[pts.length - 1]);
    ctx.lineTo(last.x, H - 4);
    ctx.closePath();
    ctx.fillStyle = grad;
    ctx.fill();

    // Line on top
    ctx.beginPath();
    ctx.moveTo(first.x, first.y);
    for (let i = 1; i < pts.length; i++) {
      const prev = toXY(i - 1, pts[i - 1]);
      const curr = toXY(i, pts[i]);
      const cpx  = (prev.x + curr.x) / 2;
      ctx.bezierCurveTo(cpx, prev.y, cpx, curr.y, curr.x, curr.y);
    }
    ctx.strokeStyle = lineColor;
    ctx.lineWidth   = 2.5;
    ctx.stroke();

    // Current value dot
    ctx.beginPath();
    ctx.arc(last.x, last.y, 4.5, 0, Math.PI * 2);
    ctx.fillStyle = lineColor;
    ctx.fill();
    ctx.strokeStyle = theme === 'dark' ? '#010409' : '#ffffff';
    ctx.lineWidth = 2;
    ctx.stroke();
  }

  function init() {
    canvas = document.getElementById('riskCanvas');
    ctx    = canvas.getContext('2d');

    function resize() {
      const rect = canvas.parentElement.getBoundingClientRect();
      canvas.width  = rect.width;
      canvas.height = 180;
      _draw();
    }
    window.addEventListener('resize', resize);
    resize();
  }

  function push(score) {
    history.push(score);
    if (history.length > MAX_POINTS * 2) history = history.slice(-MAX_POINTS);
    _draw();
  }

  // Redraw on theme change so colours update
  function redraw() { _draw(); }

  return { init, push, redraw };
})();


/* ═══════════════════════════════════════════════════════════════════════════
   4. SSE CLIENT  —  drives all live UI updates
   ═══════════════════════════════════════════════════════════════════════════ */
const SSEClient = (() => {
  let evtSource  = null;
  let logEl      = null;
  let bannerEl   = null;
  let bannerTimer = null;
  let lastDeadlockState = false;

  const MAX_LOG_LINES = 200;

  function _el(id) { return document.getElementById(id); }

  function _updateStatCard(id, value, thresholds) {
    const el = _el(id);
    if (!el) return;
    el.textContent = value;
    // optional colour thresholds: { warn, danger }
    if (thresholds) {
      el.classList.remove('highlight', 'warn', 'danger');
      if      (value >= thresholds.danger) el.classList.add('danger');
      else if (value >= thresholds.warn)   el.classList.add('warn');
      else                                 el.classList.add('highlight');
    }
  }

  function _renderLog(lines) {
    if (!logEl) return;
    if (!lines || lines.length === 0) {
      logEl.innerHTML = '<div class="empty-log">Waiting for monitor events…</div>';
      return;
    }

    const fragment = document.createDocumentFragment();
    lines.slice(-MAX_LOG_LINES).forEach(line => {
      const div = document.createElement('div');
      div.className = 'log-entry';

      // Parse:  [HH:MM:SS] TYPE pid=X resource=Y
      const m = line.match(/^(\[[\d:]+\])\s+(\w+)\s+(.*)$/);
      if (m) {
        const [, ts, tag, rest] = m;
        const tagClass = {
          WAIT:    'tag-wait',
          HOLD:    'tag-hold',
          RELEASE: 'tag-release',
          DEADLOCK:'tag-deadlock',
          RESOLVE: 'tag-resolve',
        }[tag] || 'tag-monitor';
        div.innerHTML =
          `<span class="log-ts">${ts}</span>` +
          `<span class="log-tag ${tagClass}">${tag}</span> ${_esc(rest)}`;
      } else {
        div.textContent = line;
      }
      fragment.appendChild(div);
    });

    logEl.innerHTML = '';
    logEl.appendChild(fragment);
    logEl.scrollTop = logEl.scrollHeight;
  }

  function _esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  }

  function _flashBanner(show) {
    if (!bannerEl) return;
    if (show) {
      bannerEl.classList.add('show');
      clearTimeout(bannerTimer);
      // Auto-hide after 8 seconds if no longer in deadlock
    } else {
      bannerTimer = setTimeout(() => bannerEl.classList.remove('show'), 3000);
    }
  }

  function _updateStatus(state) {
    const badge   = _el('statusBadge');
    const monitor = state.monitor_status;
    const risk    = state.risk_score;

    if (!badge) return;
    badge.className = 'status-badge';
    if (monitor === 'deadlock' || risk >= 0.75) {
      badge.classList.add('deadlock');
      badge.querySelector('.status-dot').style.display = '';
      badge.querySelector('.status-text').textContent = 'DEADLOCK';
    } else if (monitor === 'running') {
      badge.classList.add('active');
      badge.querySelector('.status-text').textContent = 'MONITOR ACTIVE';
    } else {
      badge.classList.add('stopped');
      badge.querySelector('.status-text').textContent = 'STOPPED';
    }
  }

  function _onMessage(evt) {
    let state;
    try { state = JSON.parse(evt.data); } catch { return; }

    const risk = state.risk_score || 0;
    const f    = state.features   || {};

    // Gauge
    RiskGauge.update(risk);
    // Chart
    RiskChart.push(risk);

    // Stat cards
    _updateStatCard('valBlocked',  f.blocked_count   ?? 0, { warn: 2, danger: 4 });
    _updateStatCard('valEdges',    f.edge_count       ?? 0, { warn: 8, danger: 16 });
    _updateStatCard('valGrowth',   (f.wait_time_growth ?? 0).toFixed(3), null);
    _updateStatCard('valDensity',  (f.graph_density    ?? 0).toFixed(5), null);
    _updateStatCard('valRisk',     `${Math.round(risk * 100)}%`, null);

    // Log
    _renderLog(state.log_lines);

    // Status badge
    _updateStatus(state);

    // Deadlock banner
    const isDeadlock = state.monitor_status === 'deadlock' || risk >= 0.75;
    if (isDeadlock !== lastDeadlockState) {
      _flashBanner(isDeadlock);
      lastDeadlockState = isDeadlock;
    }
  }

  function init() {
    logEl    = document.getElementById('logTerminal');
    bannerEl = document.getElementById('deadlockBanner');

    evtSource = new EventSource('/stream');
    evtSource.onmessage = _onMessage;
    evtSource.onerror   = () => {
      const badge = _el('statusBadge');
      if (badge) {
        badge.className = 'status-badge stopped';
        badge.querySelector('.status-text').textContent = 'DISCONNECTED';
      }
    };
  }

  return { init };
})();


/* ═══════════════════════════════════════════════════════════════════════════
   5. RESOLVE BUTTON
   ═══════════════════════════════════════════════════════════════════════════ */
function initResolveButton() {
  const btn      = document.getElementById('resolveBtn');
  const feedback = document.getElementById('resolveFeedback');

  if (!btn) return;

  btn.addEventListener('click', async () => {
    btn.disabled = true;
    feedback.textContent = 'Sending resolve signal…';
    feedback.className   = 'resolve-feedback';
    try {
      const res  = await fetch('/api/resolve', { method: 'POST' });
      const data = await res.json();
      if (data.ok) {
        feedback.textContent = '✓ Resolve signal sent.';
        feedback.className   = 'resolve-feedback ok';
      } else {
        feedback.textContent = `✗ ${data.error}`;
        feedback.className   = 'resolve-feedback err';
      }
    } catch (e) {
      feedback.textContent = '✗ Network error.';
      feedback.className   = 'resolve-feedback err';
    }
    setTimeout(() => {
      btn.disabled = false;
      feedback.textContent = '';
      feedback.className   = 'resolve-feedback';
    }, 3500);
  });
}


/* ═══════════════════════════════════════════════════════════════════════════
   INIT
   ═══════════════════════════════════════════════════════════════════════════ */
document.addEventListener('DOMContentLoaded', () => {
  ThemeManager.init();
  RiskGauge.init();
  RiskChart.init();
  SSEClient.init();
  initResolveButton();

  // Redraw chart on theme change so gradient colours update
  document.querySelectorAll('.theme-btn').forEach(btn => {
    btn.addEventListener('click', () => setTimeout(RiskChart.redraw, 50));
  });
});
