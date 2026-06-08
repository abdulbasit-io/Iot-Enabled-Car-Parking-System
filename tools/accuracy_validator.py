#!/usr/bin/env python3
"""
IoT-Enabled Smart Car Parking System - Functional Accuracy Validator
Department of Mechatronics Engineering, FUNAAB
"""

import os
import re
import json
import urllib.request
import time
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
                # Extract credentials
                auth_match = re.search(r'firebaseAuth\s*=\s*process\.env\.FIREBASE_AUTH\s*\|\|\s*["\']([^"\']+)["\']', content)
                if auth_match:
                    auth = auth_match.group(1)
                host_match = re.search(r'firebaseHost\s*=\s*["\']([^"\']+)["\']', content)
                if host_match:
                    host = host_match.group(1)
        except Exception as e:
            print(f"[*] Warning: Could not parse Vercel data.js: {e}")
            
    return host, auth

def fetch_current_slots(host, auth):
    url = f"https://{host}/parking/current.json?auth={auth}"
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode('utf-8'))
            return data
    except Exception as e:
        print(f"[x] Error reaching Firebase: {e}")
        return None

def main():
    print("=" * 65)
    print("      IoT SMART PARKING SYSTEM - FUNCTIONAL ACCURACY VALIDATOR   ")
    print("=" * 65)
    
    host, auth = parse_credentials()
    
    # Validation session stats
    tp, fp, tn, fn = 0, 0, 0, 0
    trials = []
    
    print("[*] Welcome to the Academic Accuracy Validation Wizard.")
    print("[*] You will place obstacle cards or vehicles at designated slots")
    print("[*] and the wizard will cross-reference the physical ground truth with Firebase.")
    print("-" * 65)
    
    while True:
        print("\nSelect test category:")
        print("1. Add a VACANT Slot Trial (TN / FP Test)")
        print("2. Add an OCCUPIED Slot Trial (TP / FN Test)")
        print("3. Print Performance Matrix & Save Thesis Report")
        print("4. Exit")
        
        choice = input("Enter choice (1-4): ").strip()
        
        if choice not in ["1", "2", "3", "4"]:
            print("[!] Invalid option. Try again.")
            continue
            
        if choice == "4":
            break
            
        if choice in ["1", "2"]:
            slot = input("Enter slot ID to test (e.g., L1, L2, R3, R5): ").strip().upper()
            if not re.match(r"^[LR][1-5]$", slot):
                print("[!] Invalid slot format. Must be L1-L5 or R1-R5.")
                continue
                
            side = slot[0] # 'L' or 'R'
            idx = int(slot[1]) - 1 # 0-indexed
            
            ground_truth = True if choice == "2" else False
            ground_truth_str = "OCCUPIED" if ground_truth else "VACANT"
            
            print(f"\n[ACTION] Please ensure physical slot {slot} is: **{ground_truth_str}**")
            input("Press [Enter] once the physical setup is ready...")
            
            print("[*] Fetching state from cloud database...")
            data = fetch_current_slots(host, auth)
            if not data:
                print("[!] Error: Could not read slot state from Firebase.")
                continue
                
            # Parse state
            is_occupied = False
            if side == 'L':
                slots = data.get("leftSlots", [])
                if idx < len(slots):
                    is_occupied = bool(slots[idx])
            else:
                slots = data.get("rightSlots", [])
                if idx < len(slots):
                    is_occupied = bool(slots[idx])
                    
            detected_str = "OCCUPIED" if is_occupied else "VACANT"
            print(f"[*] Firebase reported slot {slot} as: **{detected_str}**")
            
            # Record outcome
            if ground_truth == True:
                if is_occupied == True:
                    print("✅ RESULT: TRUE POSITIVE (TP)")
                    tp += 1
                    trials.append({"slot": slot, "truth": "Occupied", "detected": "Occupied", "result": "TP"})
                else:
                    print("❌ RESULT: FALSE NEGATIVE (FN)")
                    fn += 1
                    trials.append({"slot": slot, "truth": "Occupied", "detected": "Vacant", "result": "FN"})
            else:
                if is_occupied == False:
                    print("✅ RESULT: TRUE NEGATIVE (TN)")
                    tn += 1
                    trials.append({"slot": slot, "truth": "Vacant", "detected": "Vacant", "result": "TN"})
                else:
                    print("❌ RESULT: FALSE POSITIVE (FP)")
                    fp += 1
                    trials.append({"slot": slot, "truth": "Vacant", "detected": "Occupied", "result": "FP"})
            
        elif choice == "3":
            total = tp + fp + tn + fn
            if total == 0:
                print("[!] No trials recorded yet. Add some trials first!")
                continue
                
            # Calculate metrics
            accuracy = (tp + tn) / total
            precision = tp / (tp + fp) if (tp + fp) > 0 else 0
            recall = tp / (tp + fn) if (tp + fn) > 0 else 0
            f1 = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
            
            print("\n" + "=" * 65)
            print("                FUNCTIONAL ACCURACY METRICS REPORT               ")
            print("=" * 65)
            print(f"Total Trials Conducted: {total}")
            print(f"True Positives (TP)   : {tp}")
            print(f"True Negatives (TN)   : {tn}")
            print(f"False Positives (FP)  : {fp}")
            print(f"False Negatives (FN)  : {fn}")
            print("-" * 65)
            print(f"Accuracy              : {accuracy * 100:.2f} %  (Target: > 97.00%)")
            print(f"Precision             : {precision * 100:.2f} %  (Target: > 95.00%)")
            print(f"Recall                : {recall * 100:.2f} %  (Target: > 95.00%)")
            print(f"F1-Score              : {f1:.4f}       (Target: > 0.9500)")
            print("=" * 65)
            
            # Save report
            script_dir = os.path.dirname(os.path.abspath(__file__))
            report_path = os.path.join(script_dir, "accuracy_report.md")
            
            with open(report_path, "w", encoding="utf-8") as f:
                f.write(f"""# Functional Accuracy Assessment Report
**Generated on**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  
**Academic Section Reference**: Section 3 - Functional Accuracy Assessment

## 1. Confusion Matrix
| Metric | Value | Interpretation |
|---|---|---|
| **True Positives (TP)** | {tp} | Correctly identified occupied slots |
| **True Negatives (TN)** | {tn} | Correctly identified vacant slots |
| **False Positives (FP)** | {fp} | Vacant slots incorrectly flagged as occupied |
| **False Negatives (FN)** | {fn} | Occupied slots incorrectly flagged as vacant |

## 2. Mechatronics Performance Metrics
Calculated from {total} experimental trials:

* **Accuracy**: `{accuracy * 100:.2f}%` (Target: > 97.00%) - status: **{"✅ PASSED" if accuracy >= 0.97 else "⚠️ MARGINAL"}**
* **Precision**: `{precision * 100:.2f}%` (Target: > 95.00%) - status: **{"✅ PASSED" if precision >= 0.95 else "⚠️ MARGINAL"}**
* **Recall (Sensitivity)**: `{recall * 100:.2f}%` (Target: > 95.00%) - status: **{"✅ PASSED" if recall >= 0.95 else "⚠️ MARGINAL"}**
* **F1-Score**: `{f1:.4f}` (Target: > 0.9500) - status: **{"✅ PASSED" if f1 >= 0.95 else "⚠️ MARGINAL"}**

## 3. Trial-by-Trial Log
| Trial # | Slot | Ground Truth | System Detection | Result |
|---|---|---|---|---|
""")
                for idx, t in enumerate(trials):
                    f.write(f"| {idx + 1} | {t['slot']} | {t['truth']} | {t['detected']} | {t['result']} |\n")
                    
                f.write(f"""
---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
""")
            print(f"[+] Saved detailed academic validation report to: {report_path}")

if __name__ == "__main__":
    main()
