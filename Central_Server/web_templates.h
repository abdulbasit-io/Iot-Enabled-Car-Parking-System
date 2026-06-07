#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <pgmspace.h>

// ==================== PUBLIC DASHBOARD TEMPLATE ====================
const char PUBLIC_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Parking Dashboard</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-grad: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
            --glass-bg: rgba(30, 41, 59, 0.45);
            --glass-border: rgba(255, 255, 255, 0.08);
            --accent: #6366f1;
            --emerald: #10b981;
            --emerald-glow: rgba(16, 185, 129, 0.15);
            --crimson: #ef4444;
            --crimson-glow: rgba(239, 68, 68, 0.15);
            --text-main: #f8fafc;
            --text-mute: #94a3b8;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Outfit', sans-serif;
            background: var(--bg-grad);
            color: var(--text-main);
            min-height: 100vh;
            padding: 30px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .container {
            width: 100%;
            max-width: 1100px;
            display: flex;
            flex-direction: column;
            gap: 25px;
        }

        /* Header */
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0 10px;
        }

        h1 {
            font-size: 1.8rem;
            font-weight: 800;
            letter-spacing: -0.05em;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        h1 span {
            background: linear-gradient(to right, #818cf8, #a78bfa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .badge {
            font-size: 0.75rem;
            font-weight: 600;
            padding: 5px 12px;
            border-radius: 20px;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .badge.public {
            background: rgba(99, 102, 241, 0.15);
            color: #a5b4fc;
            border-color: rgba(99, 102, 241, 0.3);
        }

        .badge.online {
            background: var(--emerald-glow);
            color: #34d399;
            border-color: rgba(16, 185, 129, 0.3);
        }

        .badge.offline {
            background: var(--crimson-glow);
            color: #f87171;
            border-color: rgba(239, 68, 68, 0.3);
        }

        /* Top Grid */
        .top-row {
            display: grid;
            grid-template-columns: 1.5fr 1fr;
            gap: 20px;
        }

        @media (max-width: 768px) {
            .top-row {
                grid-template-columns: 1fr;
            }
        }

        /* Frosted Cards */
        .card {
            background: var(--glass-bg);
            border: 1px solid var(--glass-border);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border-radius: 24px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }

        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.3);
        }

        .card-title {
            font-size: 0.9rem;
            font-weight: 600;
            color: var(--text-mute);
            text-transform: uppercase;
            letter-spacing: 0.05em;
            margin-bottom: 20px;
        }

        /* Occupancy Gauge Widget */
        .occupancy-body {
            display: flex;
            align-items: center;
            justify-content: space-around;
            gap: 20px;
            padding: 10px 0;
        }

        .gauge-container {
            position: relative;
            width: 140px;
            height: 140px;
        }

        .gauge-svg {
            transform: rotate(-90deg);
        }

        .gauge-bg {
            fill: none;
            stroke: rgba(255, 255, 255, 0.05);
            stroke-width: 12;
        }

        .gauge-fill {
            fill: none;
            stroke: var(--accent);
            stroke-width: 12;
            stroke-linecap: round;
            stroke-dasharray: 439.8;
            stroke-dashoffset: 439.8;
            transition: stroke-dashoffset 0.8s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .gauge-text {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            text-align: center;
        }

        .gauge-number {
            font-size: 2rem;
            font-weight: 800;
            color: var(--text-main);
            line-height: 1;
        }

        .gauge-label {
            font-size: 0.75rem;
            color: var(--text-mute);
            font-weight: 600;
            text-transform: uppercase;
            margin-top: 4px;
        }

        .stat-details {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }

        .stat-item {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .stat-color {
            width: 12px;
            height: 12px;
            border-radius: 4px;
        }

        .stat-color.occupied { background-color: var(--crimson); }
        .stat-color.available { background-color: var(--emerald); }

        .stat-val {
            font-weight: 600;
            font-size: 1.1rem;
        }

        .stat-desc {
            font-size: 0.8rem;
            color: var(--text-mute);
        }

        /* Gate Status Widget */
        .gate-body {
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            height: 100%;
            gap: 15px;
            min-height: 140px;
        }

        .gate-beacon {
            position: relative;
            width: 60px;
            height: 60px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            background: rgba(255, 255, 255, 0.03);
            border: 2px solid rgba(255, 255, 255, 0.1);
            font-size: 1.5rem;
            transition: all 0.5s ease;
        }

        .gate-beacon.open {
            border-color: var(--emerald);
            box-shadow: 0 0 20px var(--emerald-glow);
            animation: pulse-green 2s infinite;
        }

        .gate-beacon.closed {
            border-color: var(--crimson);
            box-shadow: 0 0 20px var(--crimson-glow);
        }

        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.4); }
            70% { box-shadow: 0 0 0 15px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }

        .gate-label-status {
            font-size: 1.4rem;
            font-weight: 800;
            letter-spacing: 0.05em;
        }

        .gate-label-status.open { color: var(--emerald); }
        .gate-label-status.closed { color: var(--crimson); }

        .full-warning {
            color: var(--crimson);
            background: rgba(239, 110, 110, 0.1);
            border: 1px solid rgba(239, 68, 68, 0.2);
            padding: 8px 15px;
            border-radius: 12px;
            font-weight: 600;
            font-size: 0.85rem;
            animation: flash 1.5s infinite;
        }

        @keyframes flash {
            0% { opacity: 0.5; }
            50% { opacity: 1; }
            100% { opacity: 0.5; }
        }

        /* Parking Layout Maps */
        .layout-row {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }

        @media (max-width: 900px) {
            .layout-row {
                grid-template-columns: 1fr;
            }
        }

        .section-title {
            font-size: 1rem;
            font-weight: 700;
            margin-bottom: 15px;
            color: var(--text-main);
            display: flex;
            align-items: center;
            justify-content: space-between;
        }

        .section-title span {
            font-size: 0.8rem;
            font-weight: 400;
            color: var(--text-mute);
        }

        .slots-grid {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
        }

        .slot-card {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 16px;
            padding: 15px 10px;
            text-align: center;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 8px;
            transition: all 0.3s ease;
        }

        .slot-card.occupied {
            background: rgba(239, 68, 68, 0.04);
            border-color: rgba(239, 68, 68, 0.2);
        }

        .slot-card.available {
            background: rgba(16, 185, 129, 0.04);
            border-color: rgba(16, 185, 129, 0.2);
        }

        .slot-id {
            font-size: 0.75rem;
            font-weight: 600;
            color: var(--text-mute);
        }

        .slot-icon {
            font-size: 1.2rem;
            filter: grayscale(1);
            opacity: 0.3;
            transition: all 0.3s ease;
        }

        .slot-card.occupied .slot-icon {
            filter: none;
            opacity: 1;
        }

        .slot-state {
            font-size: 0.7rem;
            font-weight: 800;
            letter-spacing: 0.05em;
            text-transform: uppercase;
        }

        .slot-card.occupied .slot-state { color: var(--crimson); }
        .slot-card.available .slot-state { color: var(--emerald); }

        /* Footer Link */
        footer {
            display: flex;
            justify-content: center;
            margin-top: 10px;
        }

        .admin-link {
            color: var(--text-mute);
            text-decoration: none;
            font-size: 0.85rem;
            font-weight: 600;
            display: flex;
            align-items: center;
            gap: 5px;
            padding: 8px 18px;
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 0.05);
            background: rgba(255, 255, 255, 0.02);
            transition: all 0.3s ease;
        }

        .admin-link:hover {
            color: var(--text-main);
            border-color: var(--accent);
            background: rgba(99, 102, 241, 0.05);
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <header>
            <h1>Smart <span>Parking</span></h1>
            <div class="badge public">Public View</div>
        </header>

        <!-- Top Widgets Row -->
        <div class="top-row">
            <!-- Occupancy -->
            <div class="card">
                <div class="card-title">Real-Time Occupancy</div>
                <div class="occupancy-body">
                    <div class="gauge-container">
                        <svg class="gauge-svg" width="140" height="140">
                            <circle class="gauge-bg" cx="70" cy="70" r="60"></circle>
                            <circle class="gauge-fill" id="occupancyRing" cx="70" cy="70" r="60"></circle>
                        </svg>
                        <div class="gauge-text">
                            <div class="gauge-number" id="occCount">0</div>
                            <div class="gauge-label">Occupied</div>
                        </div>
                    </div>
                    <div class="stat-details">
                        <div class="stat-item">
                            <div class="stat-color occupied"></div>
                            <div>
                                <div class="stat-val" id="lblOccupied">0</div>
                                <div class="stat-desc">Full Slots</div>
                            </div>
                        </div>
                        <div class="stat-item">
                            <div class="stat-color available"></div>
                            <div>
                                <div class="stat-val" id="lblAvailable">10</div>
                                <div class="stat-desc">Empty Slots</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Gate Status -->
            <div class="card" style="display: flex; flex-direction: column;">
                <div class="card-title">Gate Security</div>
                <div class="gate-body">
                    <div class="gate-beacon" id="gateBeacon">🔒</div>
                    <div class="gate-label-status" id="gateStatusText">CLOSED</div>
                    <div class="full-warning" id="fullWarning" style="display: none;">PARKING LOT FULL</div>
                </div>
            </div>
        </div>

        <!-- Parking Maps Grid -->
        <div class="layout-row">
            <!-- Left Side -->
            <div class="card">
                <div class="section-title">
                    <span>⬅️ Left Section</span>
                    <span id="leftNodeStatus">Disconn.</span>
                </div>
                <div class="slots-grid" id="leftGrid">
                    <!-- Left slots auto generated -->
                </div>
            </div>

            <!-- Right Side -->
            <div class="card">
                <div class="section-title">
                    <span>➡️ Right Section</span>
                    <span id="rightNodeStatus">Disconn.</span>
                </div>
                <div class="slots-grid" id="rightGrid">
                    <!-- Right slots auto generated -->
                </div>
            </div>
        </div>

        <!-- Admin Navigation -->
        <footer>
            <a href="/admin" class="admin-link">🛡️ Admin Control Panel</a>
        </footer>
    </div>

    <!-- Frontend Live Polling Logic -->
    <script>
        const maxSlots = 10;
        const circumference = 2 * Math.PI * 60; // 376.99

        function updateGauge(occupied) {
            const circle = document.getElementById('occupancyRing');
            const percent = (occupied / maxSlots) * 100;
            const offset = circumference - (percent / 100) * circumference;
            circle.style.strokeDashoffset = offset;
            document.getElementById('occCount').innerText = occupied;
            document.getElementById('lblOccupied').innerText = occupied;
            document.getElementById('lblAvailable').innerText = maxSlots - occupied;
        }

        function buildGrids() {
            // Generate Left Slots
            let leftHtml = '';
            for (let i = 1; i <= 5; i++) {
                leftHtml += `
                    <div class="slot-card available" id="slot-L${i}">
                        <div class="slot-id">L${i}</div>
                        <div class="slot-icon">🚗</div>
                        <div class="slot-state">Empty</div>
                    </div>
                `;
            }
            document.getElementById('leftGrid').innerHTML = leftHtml;

            // Generate Right Slots
            let rightHtml = '';
            for (let i = 1; i <= 5; i++) {
                rightHtml += `
                    <div class="slot-card available" id="slot-R${i}">
                        <div class="slot-id">R${i}</div>
                        <div class="slot-icon">🚗</div>
                        <div class="slot-state">Empty</div>
                    </div>
                `;
            }
            document.getElementById('rightGrid').innerHTML = rightHtml;
        }

        async function pollData() {
            try {
                // Fetch stats and slots
                const response = await fetch('/api/data');
                const data = await response.json();

                updateGauge(data.occupied);

                // Gate updates
                const beacon = document.getElementById('gateBeacon');
                const text = document.getElementById('gateStatusText');
                const warning = document.getElementById('fullWarning');

                if (data.gateOpen) {
                    beacon.innerText = '🔓';
                    beacon.className = 'gate-beacon open';
                    text.innerText = 'OPEN';
                    text.className = 'gate-label-status open';
                } else {
                    beacon.innerText = '🔒';
                    beacon.className = 'gate-beacon closed';
                    text.innerText = 'CLOSED';
                    text.className = 'gate-label-status closed';
                }

                warning.style.display = (data.occupied >= maxSlots && !data.gateOpen) ? 'block' : 'none';

                // Map updates - Left
                data.left.forEach((val, index) => {
                    const card = document.getElementById(`slot-L${index + 1}`);
                    if (card) {
                        card.className = `slot-card ${val ? 'occupied' : 'available'}`;
                        card.querySelector('.slot-state').innerText = val ? 'Occupied' : 'Empty';
                    }
                });

                // Map updates - Right
                data.right.forEach((val, index) => {
                    const card = document.getElementById(`slot-R${index + 1}`);
                    if (card) {
                        card.className = `slot-card ${val ? 'occupied' : 'available'}`;
                        card.querySelector('.slot-state').innerText = val ? 'Occupied' : 'Empty';
                    }
                });

                // Fetch Node status info
                const nodeRes = await fetch('/api/nodes');
                const nodeData = await nodeRes.json();
                
                let leftOnline = 'Offline';
                let rightOnline = 'Offline';
                
                nodeData.connectedNodes.forEach(n => {
                    if (n.side === 0) leftOnline = n.lastMessageStr;
                    if (n.side === 1) rightOnline = n.lastMessageStr;
                });

                document.getElementById('leftNodeStatus').innerText = leftOnline;
                document.getElementById('rightNodeStatus').innerText = rightOnline;

            } catch (err) {
                console.error("Poll error:", err);
            }
        }

        // Start Up
        buildGrids();
        pollData();
        setInterval(pollData, 2000);
    </script>
</body>
</html>
)rawliteral";

// ==================== ADMIN CONTROL PANEL TEMPLATE ====================
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Control Center</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
    <!-- Authenticate immediately on page load -->
    <script>
        if (localStorage.getItem("admin_auth") !== "true") {
            let pw = prompt("Enter Admin Password:");
            if (pw === "admin123") {
                localStorage.setItem("admin_auth", "true");
            } else {
                alert("Access Denied!");
                window.location.href = "/";
            }
        }
    </script>
    <style>
        :root {
            --bg-grad: linear-gradient(135deg, #090d16 0%, #111026 100%);
            --glass-bg: rgba(15, 23, 42, 0.65);
            --glass-border: rgba(255, 255, 255, 0.05);
            --accent: #6366f1;
            --emerald: #10b981;
            --emerald-glow: rgba(16, 185, 129, 0.15);
            --crimson: #ef4444;
            --crimson-glow: rgba(239, 68, 68, 0.15);
            --text-main: #f8fafc;
            --text-mute: #94a3b8;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Outfit', sans-serif;
            background: var(--bg-grad);
            color: var(--text-main);
            min-height: 100vh;
            padding: 30px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .container {
            width: 100%;
            max-width: 1100px;
            display: flex;
            flex-direction: column;
            gap: 25px;
        }

        /* Header */
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0 10px;
        }

        h1 {
            font-size: 1.8rem;
            font-weight: 800;
            letter-spacing: -0.05em;
        }

        h1 span {
            color: #f43f5e; /* Rose admin accent */
        }

        .header-actions {
            display: flex;
            align-items: center;
            gap: 15px;
        }

        .btn {
            font-family: 'Outfit', sans-serif;
            font-weight: 600;
            font-size: 0.85rem;
            padding: 8px 16px;
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 0.1);
            background: rgba(255, 255, 255, 0.03);
            color: var(--text-main);
            cursor: pointer;
            transition: all 0.3s ease;
        }

        .btn:hover {
            background: rgba(255, 255, 255, 0.08);
            border-color: rgba(255, 255, 255, 0.2);
        }

        .btn.logout {
            border-color: rgba(244, 63, 94, 0.3);
            color: #fb7185;
            background: rgba(244, 63, 94, 0.05);
        }

        .btn.logout:hover {
            background: rgba(244, 63, 94, 0.15);
            border-color: #f43f5e;
        }

        /* Cards Layout */
        .grid-3 {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
        }

        .grid-2 {
            display: grid;
            grid-template-columns: 1.5fr 1fr;
            gap: 20px;
        }

        @media (max-width: 900px) {
            .grid-3, .grid-2 {
                grid-template-columns: 1fr;
            }
        }

        .card {
            background: var(--glass-bg);
            border: 1px solid var(--glass-border);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border-radius: 24px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
        }

        .card-title {
            font-size: 0.9rem;
            font-weight: 600;
            color: var(--text-mute);
            text-transform: uppercase;
            letter-spacing: 0.05em;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        /* System Operations Card */
        .mode-row {
            display: flex;
            gap: 10px;
            margin-top: 15px;
        }

        .mode-btn {
            flex: 1;
            padding: 15px;
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 0.05);
            background: rgba(255, 255, 255, 0.01);
            color: var(--text-mute);
            font-family: 'Outfit', sans-serif;
            font-weight: 700;
            font-size: 0.9rem;
            cursor: pointer;
            transition: all 0.3s ease;
            text-align: center;
        }

        .mode-btn:hover {
            background: rgba(255, 255, 255, 0.04);
            color: var(--text-main);
        }

        .mode-btn.active {
            color: var(--text-main);
            border-color: rgba(99, 102, 241, 0.4);
            background: rgba(99, 102, 241, 0.1);
            box-shadow: 0 0 15px rgba(99, 102, 241, 0.15);
        }

        .sys-state {
            margin-top: 20px;
            padding: 12px 15px;
            background: rgba(255, 255, 255, 0.02);
            border-radius: 12px;
            font-size: 0.85rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .state-val {
            font-weight: 600;
            color: var(--emerald);
        }

        /* Occupancy Gauge Widget */
        .occupancy-body {
            display: flex;
            align-items: center;
            justify-content: space-around;
            gap: 20px;
            padding: 10px 0;
        }

        .gauge-container {
            position: relative;
            width: 140px;
            height: 140px;
        }

        .gauge-svg {
            transform: rotate(-90deg);
        }

        .gauge-bg {
            fill: none;
            stroke: rgba(255, 255, 255, 0.05);
            stroke-width: 12;
        }

        .gauge-fill {
            fill: none;
            stroke: var(--accent);
            stroke-width: 12;
            stroke-linecap: round;
            stroke-dasharray: 439.8;
            stroke-dashoffset: 439.8;
            transition: stroke-dashoffset 0.8s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .gauge-text {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            text-align: center;
        }

        .gauge-number {
            font-size: 2rem;
            font-weight: 800;
            color: var(--text-main);
            line-height: 1;
        }

        .gauge-label {
            font-size: 0.75rem;
            color: var(--text-mute);
            font-weight: 600;
            text-transform: uppercase;
            margin-top: 4px;
        }

        .stat-details {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }

        .stat-item {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .stat-color {
            width: 12px;
            height: 12px;
            border-radius: 4px;
        }

        .stat-color.occupied { background-color: var(--crimson); }
        .stat-color.available { background-color: var(--emerald); }

        .stat-val {
            font-weight: 600;
            font-size: 1.1rem;
        }

        .stat-desc {
            font-size: 0.8rem;
            color: var(--text-mute);
        }

        /* Gate Status Widget */
        .gate-body {
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            height: 100%;
            gap: 15px;
            min-height: 140px;
        }

        .gate-beacon {
            position: relative;
            width: 60px;
            height: 60px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            background: rgba(255, 255, 255, 0.03);
            border: 2px solid rgba(255, 255, 255, 0.1);
            font-size: 1.5rem;
            transition: all 0.5s ease;
        }

        .gate-beacon.open {
            border-color: var(--emerald);
            box-shadow: 0 0 20px var(--emerald-glow);
            animation: pulse-green 2s infinite;
        }

        .gate-beacon.closed {
            border-color: var(--crimson);
            box-shadow: 0 0 20px var(--crimson-glow);
        }

        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.4); }
            70% { box-shadow: 0 0 0 15px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }

        .gate-label-status {
            font-size: 1.4rem;
            font-weight: 800;
            letter-spacing: 0.05em;
        }

        .gate-label-status.open { color: var(--emerald); }
        .gate-label-status.closed { color: var(--crimson); }

        .full-warning {
            color: var(--crimson);
            background: rgba(239, 110, 110, 0.1);
            border: 1px solid rgba(239, 68, 68, 0.2);
            padding: 8px 15px;
            border-radius: 12px;
            font-weight: 600;
            font-size: 0.85rem;
            animation: flash 1.5s infinite;
        }

        @keyframes flash {
            0% { opacity: 0.5; }
            50% { opacity: 1; }
            100% { opacity: 0.5; }
        }

        /* Power Stats Card */
        .power-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }

        .power-item {
            background: rgba(255, 255, 255, 0.02);
            border-radius: 16px;
            padding: 15px;
            border: 1px solid rgba(255, 255, 255, 0.03);
            text-align: center;
        }

        .power-lbl {
            font-size: 0.75rem;
            color: var(--text-mute);
            font-weight: 600;
            margin-bottom: 5px;
        }

        .power-val {
            font-size: 1.4rem;
            font-weight: 800;
            color: #38bdf8; /* Cyan telemetry */
        }

        /* Node Status Card */
        .nodes-container {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }

        .node-row-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 15px;
            background: rgba(255, 255, 255, 0.02);
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 0.03);
        }

        .node-meta {
            display: flex;
            flex-direction: column;
            gap: 2px;
        }

        .node-name {
            font-size: 0.85rem;
            font-weight: 600;
        }

        .node-mac-val {
            font-size: 0.7rem;
            font-family: monospace;
            color: var(--text-mute);
        }

        .node-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: var(--crimson);
            box-shadow: 0 0 10px rgba(239, 68, 68, 0.2);
            transition: all 0.3s ease;
        }

        .node-dot.online {
            background-color: var(--emerald);
            box-shadow: 0 0 10px rgba(16, 185, 129, 0.4);
        }

        /* Logs Console */
        .console-card {
            display: flex;
            flex-direction: column;
        }

        .terminal {
            background: #020617;
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 16px;
            padding: 15px;
            font-family: 'Courier New', monospace;
            font-size: 0.8rem;
            height: 250px;
            overflow-y: auto;
            display: flex;
            flex-direction: column;
            gap: 6px;
            color: #a7f3d0; /* Soft green terminals */
        }

        .log-line {
            line-height: 1.4;
            display: flex;
            gap: 10px;
        }

        .log-tstamp {
            color: #6366f1;
            flex-shrink: 0;
            font-weight: bold;
        }

        .log-msg {
            word-break: break-all;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <header>
            <h1>Admin <span>Control Panel</span></h1>
            <div class="header-actions">
                <button class="btn" onclick="goHome()">🏠 Public View</button>
                <button class="btn logout" onclick="logout()">🔒 Lock</button>
            </div>
        </header>

        <!-- Top Widgets Row (Occupancy & Gate) -->
        <div class="grid-2">
            <!-- Occupancy -->
            <div class="card">
                <div class="card-title">Real-Time Occupancy</div>
                <div class="occupancy-body">
                    <div class="gauge-container">
                        <svg class="gauge-svg" width="140" height="140">
                            <circle class="gauge-bg" cx="70" cy="70" r="60"></circle>
                            <circle class="gauge-fill" id="occupancyRing" cx="70" cy="70" r="60"></circle>
                        </svg>
                        <div class="gauge-text">
                            <div class="gauge-number" id="occCount">0</div>
                            <div class="gauge-label">Occupied</div>
                        </div>
                    </div>
                    <div class="stat-details">
                        <div class="stat-item">
                            <div class="stat-color occupied"></div>
                            <div>
                                <div class="stat-val" id="lblOccupied">0</div>
                                <div class="stat-desc">Full Slots</div>
                            </div>
                        </div>
                        <div class="stat-item">
                            <div class="stat-color available"></div>
                            <div>
                                <div class="stat-val" id="lblAvailable">10</div>
                                <div class="stat-desc">Empty Slots</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Gate Status -->
            <div class="card" style="display: flex; flex-direction: column;">
                <div class="card-title">Gate Security</div>
                <div class="gate-body">
                    <div class="gate-beacon" id="gateBeacon">🔒</div>
                    <div class="gate-label-status" id="gateStatusText">CLOSED</div>
                    <div class="full-warning" id="fullWarning" style="display: none;">PARKING LOT FULL</div>
                </div>
            </div>
        </div>

        <!-- System Controls row -->
        <div class="grid-2">
            <!-- Mode Config -->
            <div class="card">
                <div class="card-title">System Operations Mode</div>
                <div class="mode-row">
                    <button class="mode-btn" id="btn-auto" onclick="changeMode('auto')">🤖 AUTO</button>
                    <button class="mode-btn" id="btn-online" onclick="changeMode('online')">🌐 FORCE ONLINE</button>
                    <button class="mode-btn" id="btn-offline" onclick="changeMode('offline')">📡 FORCE OFFLINE</button>
                </div>
                <div class="sys-state">
                    <span>Active Network Mode:</span>
                    <span class="state-val" id="activeModeText">LOADING...</span>
                </div>
            </div>

            <!-- ESP-NOW Nodes Monitor -->
            <div class="card">
                <div class="card-title">ESP-NOW Sensor Health</div>
                <div class="nodes-container">
                    <div class="node-row-item">
                        <div class="node-meta">
                            <span class="node-name">⬅️ Left Side Node (L1-L5)</span>
                            <span class="node-mac-val" id="leftMac">--:--:--:--:--:--</span>
                        </div>
                        <div class="node-dot" id="leftDot"></div>
                    </div>
                    <div class="node-row-item">
                        <div class="node-meta">
                            <span class="node-name">➡️ Right Side Node (R1-R5)</span>
                            <span class="node-mac-val" id="rightMac">--:--:--:--:--:--</span>
                        </div>
                        <div class="node-dot" id="rightDot"></div>
                    </div>
                </div>
            </div>
        </div>

        <!-- Telemetry and Logs row -->
        <div class="grid-2">
            <!-- Logs Console -->
            <div class="card console-card">
                <div class="card-title">
                    <span>📋 Live System Logs</span>
                    <button class="btn" style="padding: 3px 10px; font-size: 0.75rem;" onclick="clearLogs()">Clear Screen</button>
                </div>
                <div class="terminal" id="terminalConsole">
                    <!-- Logs generated here -->
                </div>
            </div>

            <!-- Power Telemetry -->
            <div class="card">
                <div class="card-title">⚡ Power monitoring</div>
                <div class="power-grid">
                    <div class="power-item">
                        <div class="power-lbl">Current</div>
                        <div class="power-val" id="p-curr">0.00 A</div>
                    </div>
                    <div class="power-item">
                        <div class="power-lbl">Voltage</div>
                        <div class="power-val" id="p-volt">0.00 V</div>
                    </div>
                    <div class="power-item">
                        <div class="power-lbl">Power</div>
                        <div class="power-val" id="p-pow">0.00 W</div>
                    </div>
                    <div class="power-item">
                        <div class="power-lbl">Est. Daily Energy</div>
                        <div class="power-val" id="p-energy">0.00 kWh</div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- AJAX Scripts for admin functionality -->
    <script>
        let localLoggedTimeStrings = new Set();

        function goHome() {
            window.location.href = "/";
        }

        function logout() {
            localStorage.removeItem("admin_auth");
            window.location.href = "/";
        }

        async function changeMode(mode) {
            try {
                const res = await fetch(`/api/mode?mode=${mode}`);
                if(res.ok) {
                    // Quick state update visual
                    updateModeButtons(mode.toUpperCase());
                }
            } catch (err) {
                console.error("Failed to change mode:", err);
            }
        }

        function updateModeButtons(activeMode) {
            document.getElementById('btn-auto').className = 'mode-btn' + (activeMode === 'AUTO' ? ' active' : '');
            document.getElementById('btn-online').className = 'mode-btn' + (activeMode === 'ONLINE' ? ' active' : '');
            document.getElementById('btn-offline').className = 'mode-btn' + (activeMode === 'OFFLINE' ? ' active' : '');
        }

        function clearLogs() {
            document.getElementById('terminalConsole').innerHTML = '';
            localLoggedTimeStrings.clear();
        }

        async function pollAdminData() {
            try {
                // Get basic data
                const dataRes = await fetch('/api/data');
                const data = await dataRes.json();
                
                // Update Occupancy Gauge
                const circle = document.getElementById('occupancyRing');
                const percent = (data.occupied / 10) * 100;
                const circumference = 2 * Math.PI * 60; // 376.99
                const offset = circumference - (percent / 100) * circumference;
                circle.style.strokeDashoffset = offset;
                document.getElementById('occCount').innerText = data.occupied;
                document.getElementById('lblOccupied').innerText = data.occupied;
                document.getElementById('lblAvailable').innerText = 10 - data.occupied;

                // Update Gate Status
                const beacon = document.getElementById('gateBeacon');
                const gateText = document.getElementById('gateStatusText');
                const warning = document.getElementById('fullWarning');

                if (data.gateOpen) {
                    beacon.innerText = '🔓';
                    beacon.className = 'gate-beacon open';
                    gateText.innerText = 'OPEN';
                    gateText.className = 'gate-label-status open';
                } else {
                    beacon.innerText = '🔒';
                    beacon.className = 'gate-beacon closed';
                    gateText.innerText = 'CLOSED';
                    gateText.className = 'gate-label-status closed';
                }

                warning.style.display = (data.occupied >= 10 && !data.gateOpen) ? 'block' : 'none';

                // Set power telemetry
                document.getElementById('p-curr').innerText = data.current.toFixed(2) + ' A';
                document.getElementById('p-volt').style.color = '#38bdf8'; // Force colour
                document.getElementById('p-volt').innerText = '5.00 V'; 
                document.getElementById('p-pow').innerText = data.power.toFixed(2) + ' W';
                document.getElementById('p-energy').innerText = (data.power * 24 / 1000).toFixed(3) + ' kWh';
                
                // Get operating mode configuration
                let configStr = 'AUTO';
                if (data.sysMode === 1) configStr = 'FORCE ONLINE';
                if (data.sysMode === 2) configStr = 'FORCE OFFLINE';
                
                document.getElementById('activeModeText').innerText = `${data.mode} (${configStr})`;
                
                document.getElementById('btn-auto').className = 'mode-btn' + (data.sysMode === 0 ? ' active' : '');
                document.getElementById('btn-online').className = 'mode-btn' + (data.sysMode === 1 ? ' active' : '');
                document.getElementById('btn-offline').className = 'mode-btn' + (data.sysMode === 2 ? ' active' : '');

                // Get Node MAC addresses
                const nodeRes = await fetch('/api/nodes');
                const nodeData = await nodeRes.json();
                
                let leftOn = false;
                let rightOn = false;
                
                nodeData.connectedNodes.forEach(node => {
                    if (node.side === 0) {
                        document.getElementById('leftMac').innerText = node.mac;
                        leftOn = (node.lastMessageStr !== 'Offline');
                    }
                    if (node.side === 1) {
                        document.getElementById('rightMac').innerText = node.mac;
                        rightOn = (node.lastMessageStr !== 'Offline');
                    }
                });

                document.getElementById('leftDot').className = `node-dot ${leftOn ? 'online' : ''}`;
                document.getElementById('rightDot').className = `node-dot ${rightOn ? 'online' : ''}`;

                // Fetch System Logs
                const logRes = await fetch('/api/logs');
                const infoText = await logRes.text();
                
                // Parse the plain text logs from /api/info
                // Expected format: line-by-line of "timestamp: message"
                const lines = infoText.split('\n');
                const terminal = document.getElementById('terminalConsole');
                let newLinesAdded = false;

                lines.forEach(line => {
                    if(line.trim() && line.includes(':')) {
                        const colonIndex = line.indexOf(':');
                        const tstamp = line.substring(0, colonIndex);
                        const msg = line.substring(colonIndex + 1);
                        
                        const uniqueKey = `${tstamp}-${msg}`;
                        if (!localLoggedTimeStrings.has(uniqueKey)) {
                            localLoggedTimeStrings.add(uniqueKey);
                            
                            const lineDiv = document.createElement('div');
                            lineDiv.className = 'log-line';
                            lineDiv.innerHTML = `<span class="log-tstamp">[${tstamp}]</span><span class="log-msg">${msg}</span>`;
                            terminal.appendChild(lineDiv);
                            newLinesAdded = true;
                        }
                    }
                });

                if (newLinesAdded) {
                    terminal.scrollTop = terminal.scrollHeight;
                }

            } catch (err) {
                console.error("Poll Admin error:", err);
            }
        }

        // Start Up
        pollAdminData();
        setInterval(pollAdminData, 2000);
    </script>
</body>
</html>
)rawliteral";

#endif
