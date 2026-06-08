#!/usr/bin/env python3
"""
IoT-Enabled Smart Car Parking System - QoS & Network Analyzer
Department of Mechatronics Engineering, FUNAAB
"""

import os
import re
import json
import urllib.request
import subprocess
import platform
from datetime import datetime

def parse_credentials():
    # Defaults
    host = "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app"
    auth = "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW"
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    data_js_path = os.path.join(script_dir, "..", "Vercel_Mirror", "api", "data.js")
    
    if os.path.exists(data_js_path):
        try:
            with open(data_js_path, "r", encoding="utf-8") as f:
                content = f.read()
                # Extract firebaseAuth
                auth_match = re.search(r'firebaseAuth\s*=\s*process\.env\.FIREBASE_AUTH\s*\|\|\s*["\']([^"\']+)["\']', content)
                if auth_match:
                    auth = auth_match.group(1)
                # Extract firebaseHost
                host_match = re.search(r'firebaseHost\s*=\s*["\']([^"\']+)["\']', content)
                if host_match:
                    host = host_match.group(1)
        except Exception as e:
            print(f"[*] Warning: Could not parse Vercel data.js file: {e}")
            print("[*] Falling back to default credentials.")
            
    return host, auth

def ping_host(host):
    print(f"[*] Measuring Network RTT (pinging {host})...")
    # Clean host from protocol or port
    clean_host = host.replace("https://", "").replace("http://", "").split("/")[0]
    
    # Determine OS flags
    param = "-n" if platform.system().lower() == "windows" else "-c"
    command = ["ping", param, "5", clean_host]
    
    try:
        res = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=10)
        output = res.stdout
        
        # Parse output for average RTT
        if platform.system().lower() == "windows":
            match = re.findall(r"Average = (\d+)ms", output)
            if match:
                return float(match[0])
        else:
            match = re.findall(r"min/avg/max/mdev = [\d\.]+/(?P<avg>[\d\.]+)/", output)
            if match:
                return float(match[0])
            # Fallback regex
            match_fallback = re.findall(r"rtt min/avg/max/mdev = [\d\.]+/(?P<avg>[\d\.]+)/", output)
            if match_fallback:
                return float(match_fallback[0])
            
        print("[!] Could not parse ping response details. Raw output:")
        print(output[:200])
    except Exception as e:
        print(f"[!] Ping execution failed: {e}")
    return None

def main():
    print("=" * 65)
    print("      IoT SMART PARKING SYSTEM - ARCHITECTURE & QoS ANALYZER     ")
    print("=" * 65)
    
    host, auth = parse_credentials()
    print(f"[*] Firebase Host: {host}")
    
    # Fetch data
    print("[*] Downloading history telemetry logs from Firebase...")
    url = f"https://{host}/parking/history.json?auth={auth}&orderBy=\"$key\"&limitToLast=300"
    
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode('utf-8'))
    except Exception as e:
        print(f"[x] Error: Failed to fetch telemetry from Firebase: {e}")
        return

    if not data:
        print("[x] Error: No history data found in Firebase database.")
        return
    
    # Sort logs chronologically
    records = []
    for key, val in data.items():
        if isinstance(val, dict) and "timestamp" in val and "uptime" in val:
            records.append(val)
            
    records.sort(key=lambda x: x["timestamp"])
    
    total_received = len(records)
    if total_received < 2:
        print(f"[!] Warning: Only {total_received} records found. At least 2 are needed for delta QoS analysis.")
        return
        
    print(f"[*] Successfully parsed {total_received} history records.")
    
    # Metrics Variables
    expected_interval_ms = 30000 # 30 seconds
    total_lost = 0
    jitter_samples = []
    left_online_count = 0
    right_online_count = 0
    power_samples = []
    
    first_time = records[0]["timestamp"]
    last_time = records[-1]["timestamp"]
    total_elapsed_ms = last_time - first_time
    
    for i in range(total_received):
        rec = records[i]
        
        # Node online counts
        if rec.get("leftNodeOnline", False):
            left_online_count += 1
        if rec.get("rightNodeOnline", False):
            right_online_count += 1
            
        if rec.get("powerUsage") is not None:
            power_samples.append(rec["powerUsage"])
            
        if i > 0:
            prev = records[i-1]
            dt_server = rec["timestamp"] - prev["timestamp"]
            dt_uptime = rec["uptime"] - prev["uptime"]
            
            # Packet loss estimation
            if dt_server > (expected_interval_ms * 1.5):
                missed = round(dt_server / expected_interval_ms) - 1
                if missed > 0:
                    total_lost += missed
            
            # Jitter calculation: variation in latency transmission steps
            step_jitter = abs(dt_server - dt_uptime)
            jitter_samples.append(step_jitter)
            
    # Computations
    total_sent = total_received + total_lost
    packet_loss_pct = (total_lost / total_sent * 100) if total_sent > 0 else 0.0
    reliability_pct = ((total_sent - total_lost) / total_sent * 100) if total_sent > 0 else 100.0
    
    avg_jitter = sum(jitter_samples) / len(jitter_samples) if jitter_samples else 0.0
    max_jitter = max(jitter_samples) if jitter_samples else 0.0
    
    avg_power = sum(power_samples) / len(power_samples) if power_samples else 0.0
    
    # Node Availability (ESP-NOW status)
    left_node_availability = (left_online_count / total_received * 100)
    right_node_availability = (right_online_count / total_received * 100)
    
    # Ping RTT
    ping_rtt = ping_host(host)
    
    # Output Terminal Summary
    print("\n" + "=" * 65)
    print("                      QoS PERFORMANCE REPORT                     ")
    print("=" * 65)
    print(f"Test Window Start      : {datetime.fromtimestamp(first_time/1000).strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Test Window End        : {datetime.fromtimestamp(last_time/1000).strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Total Duration         : {total_elapsed_ms / 3600000:.2f} hours")
    print("-" * 65)
    print(f"Packets Transmitted    : {total_sent}")
    print(f"Packets Received (Rx)  : {total_received}")
    print(f"Packets Lost (Tx Error): {total_lost}")
    print(f"Packet Loss Rate       : {packet_loss_pct:.3f} %  (Target: < 1.0%)")
    print(f"Network Reliability    : {reliability_pct:.2f} % (Target: > 99.0%)")
    print("-" * 65)
    print(f"Average Packet Jitter  : {avg_jitter:.1f} ms  (Target: < 120 ms)")
    print(f"Maximum Packet Jitter  : {max_jitter:.1f} ms")
    if ping_rtt is not None:
        print(f"Network RTT Latency    : {ping_rtt:.1f} ms  (Target: < 150 ms)")
    else:
        print(f"Network RTT Latency    : N/A (ICMP ping blocked)")
    print("-" * 65)
    print(f"Left ESP-NOW Node Link : {left_node_availability:.2f} % Uptime")
    print(f"Right ESP-NOW Node Link: {right_node_availability:.2f} % Uptime")
    print(f"Average Power Draw     : {avg_power:.2f} W")
    print("=" * 65)
    
    # Save markdown report for Thesis/Academic use
    script_dir = os.path.dirname(os.path.abspath(__file__))
    report_path = os.path.join(script_dir, "qos_report.md")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(f"""# QoS Performance Telemetry Report
**Generated on**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  
**Academic Section Reference**: Section 1 - Sensing and Communication Architecture Testing

## 1. Network Metrics Summary
| Parameter | Measured Value | Academic Target | Status |
|---|---|---|---|
| **Packets Sent** | {total_sent} | - | N/A |
| **Packets Received** | {total_received} | - | N/A |
| **Packet Loss Rate** | {packet_loss_pct:.3f}% | < 1.00% | {"✅ PASSED" if packet_loss_pct < 1.0 else "❌ FAILED"} |
| **Network Reliability** | {reliability_pct:.2f}% | > 99.00% | {"✅ PASSED" if reliability_pct > 99.0 else "❌ FAILED"} |
| **Average Jitter** | {avg_jitter:.2f} ms | < 120.00 ms | {"✅ PASSED" if avg_jitter < 120.0 else "❌ FAILED"} |
| **Ping Latency (RTT)** | {f"{ping_rtt:.1f} ms" if ping_rtt else "N/A"} | < 150.00 ms | {("✅ PASSED" if ping_rtt < 150.0 else "❌ FAILED") if ping_rtt else "Unknown"} |

## 2. ESP-NOW Client Node Link Availability
* **Left Side Sensor Node (L1-L5)**: `{left_node_availability:.2f}%` Link Uptime Availability
* **Right Side Sensor Node (R1-R5)**: `{right_node_availability:.2f}%` Link Uptime Availability

## 3. Power Analysis
* **Mean Gateway Power Consumption**: `{avg_power:.3f} Watts`

---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
""")
    print(f"\n[+] Saved detailed academic markdown report to: {report_path}")

if __name__ == "__main__":
    main()
