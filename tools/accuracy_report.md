# Functional Accuracy Assessment Report
**Generated on**: 2026-06-08 15:29:19  
**Academic Section Reference**: Section 3 - Functional Accuracy Assessment

## 1. Confusion Matrix
| Metric | Value | Interpretation |
|---|---|---|
| **True Positives (TP)** | 3 | Correctly identified occupied slots |
| **True Negatives (TN)** | 3 | Correctly identified vacant slots |
| **False Positives (FP)** | 1 | Vacant slots incorrectly flagged as occupied |
| **False Negatives (FN)** | 0 | Occupied slots incorrectly flagged as vacant |

## 2. Mechatronics Performance Metrics
Calculated from 7 experimental trials:

* **Accuracy**: `85.71%` (Target: > 97.00%) - status: **⚠️ MARGINAL**
* **Precision**: `75.00%` (Target: > 95.00%) - status: **⚠️ MARGINAL**
* **Recall (Sensitivity)**: `100.00%` (Target: > 95.00%) - status: **✅ PASSED**
* **F1-Score**: `0.8571` (Target: > 0.9500) - status: **⚠️ MARGINAL**

## 3. Trial-by-Trial Log
| Trial # | Slot | Ground Truth | System Detection | Result |
|---|---|---|---|---|
| 1 | L4 | Vacant | Occupied | FP |
| 2 | L1 | Vacant | Vacant | TN |
| 3 | R1 | Occupied | Occupied | TP |
| 4 | R2 | Occupied | Occupied | TP |
| 5 | R3 | Vacant | Vacant | TN |
| 6 | L4 | Occupied | Occupied | TP |
| 7 | L5 | Vacant | Vacant | TN |

---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
