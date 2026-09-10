#pragma once

#include <string_view>

// Single-page dashboard with Chart.js, dark theme, and real-time polling.
// Embedded directly into the binary as a C++ raw string literal (zero disk dependencies).
inline constexpr std::string_view DASHBOARD_HTML = R"html(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kronos TSDB — Dashboard</title>
    <!-- Chart.js from CDN -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: #0b0f19;
            color: #e2e8f0;
            padding: 24px;
        }
        .header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 24px;
            padding-bottom: 16px;
            border-bottom: 1px solid #1e293b;
        }
        .title-group { display: flex; align-items: center; gap: 12px; }
        h1 { font-size: 22px; font-weight: 700; color: #f8fafc; }
        .live-badge {
            background: rgba(16, 185, 129, 0.15);
            color: #10b981;
            border: 1px solid #10b981;
            font-size: 11px;
            font-weight: 600;
            padding: 2px 8px;
            border-radius: 9999px;
            letter-spacing: 0.05em;
        }
        .controls { display: flex; align-items: center; gap: 12px; }
        select {
            background: #1e293b;
            color: #f8fafc;
            border: 1px solid #334155;
            padding: 8px 14px;
            border-radius: 8px;
            font-size: 14px;
            outline: none;
            cursor: pointer;
        }
        select:focus { border-color: #38bdf8; }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }
        .card {
            background: #131b2e;
            border: 1px solid #1e293b;
            padding: 16px 20px;
            border-radius: 12px;
        }
        .card-label { font-size: 12px; color: #94a3b8; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.05em; }
        .card-value { font-size: 24px; font-weight: 700; color: #38bdf8; }
        .chart-card {
            background: #131b2e;
            border: 1px solid #1e293b;
            padding: 24px;
            border-radius: 12px;
            height: 480px;
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="title-group">
            <h1>⏱️ Kronos TSDB</h1>
            <span class="live-badge">● LIVE (2s)</span>
        </div>
        <div class="controls">
            <label for="metricSelect" style="font-size: 13px; color: #94a3b8;">Metric:</label>
            <select id="metricSelect"></select>
        </div>
    </div>

    <div class="stats-grid">
        <div class="card">
            <div class="card-label">Latest Value</div>
            <div class="card-value" id="statLatest">—</div>
        </div>
        <div class="card">
            <div class="card-label">Average (Buffer)</div>
            <div class="card-value" id="statAvg">—</div>
        </div>
        <div class="card">
            <div class="card-label">Buffered Points</div>
            <div class="card-value" id="statPoints">—</div>
        </div>
    </div>

    <div class="chart-card">
        <canvas id="chartCanvas"></canvas>
    </div>

    <script>
        let chart = null;
        let selectedMetric = '';

        // Initialize Chart.js with dark-theme styling
        function initChart() {
            const ctx = document.getElementById('chartCanvas').getContext('2d');
            
            // Gradient fill
            const gradient = ctx.createLinearGradient(0, 0, 0, 400);
            gradient.addColorStop(0, 'rgba(56, 189, 248, 0.35)');
            gradient.addColorStop(1, 'rgba(56, 189, 248, 0.0)');

            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Value',
                        data: [],
                        borderColor: '#38bdf8',
                        backgroundColor: gradient,
                        borderWidth: 2,
                        pointRadius: 3,
                        pointBackgroundColor: '#38bdf8',
                        fill: true,
                        tension: 0.25
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: { duration: 300 },
                    scales: {
                        x: {
                            grid: { color: 'rgba(255, 255, 255, 0.04)' },
                            ticks: { color: '#64748b', maxTicksLimit: 10 }
                        },
                        y: {
                            grid: { color: 'rgba(255, 255, 255, 0.06)' },
                            ticks: { color: '#94a3b8' }
                        }
                    },
                    plugins: {
                        legend: { display: false },
                        tooltip: {
                            backgroundColor: '#1e293b',
                            titleColor: '#94a3b8',
                            bodyColor: '#f8fafc',
                            borderColor: '#334155',
                            borderWidth: 1
                        }
                    }
                }
            });
        }

        // Fetch metric names and populate dropdown
        async function loadMetricsList() {
            try {
                const res = await fetch('/api/v1/metrics_list');
                const names = await res.json();
                const select = document.getElementById('metricSelect');
                select.innerHTML = '';

                names.forEach(name => {
                    const opt = document.createElement('option');
                    opt.value = name;
                    opt.textContent = name;
                    select.appendChild(opt);
                });

                if (names.length > 0) {
                    selectedMetric = names[0];
                    pollMetric();
                }
            } catch (err) {
                console.error('Failed to load metrics list:', err);
            }
        }

        // Poll query endpoint and update chart + stats cards
        async function pollMetric() {
            if (!selectedMetric) return;
            try {
                const res = await fetch(`/api/v1/query?name=${encodeURIComponent(selectedMetric)}`);
                if (!res.ok) return;
                const data = await res.json();

                // Update stat cards
                document.getElementById('statLatest').textContent = data.latest.toLocaleString();
                document.getElementById('statAvg').textContent = data.average.toLocaleString();
                document.getElementById('statPoints').textContent = (data.points ? data.points.length : 0);

                // Update chart points
                if (data.points && chart) {
                    chart.data.labels = data.points.map((p, idx) => {
                        if (p.timestamp && p.timestamp > 0) {
                            return new Date(p.timestamp * 1000).toLocaleTimeString();
                        }
                        return `#${idx + 1}`;
                    });
                    chart.data.datasets[0].data = data.points.map(p => p.value);
                    chart.update();
                }
            } catch (err) {
                console.error('Polling error:', err);
            }
        }

        document.getElementById('metricSelect').addEventListener('change', (e) => {
            selectedMetric = e.target.value;
            pollMetric();
        });

        initChart();
        loadMetricsList();
        setInterval(pollMetric, 2000);
    </script>
</body>
</html>)html";