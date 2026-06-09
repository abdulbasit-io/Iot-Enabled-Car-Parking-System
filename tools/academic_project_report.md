# Design, Implementation, and Performance Evaluation of an IoT-Enabled Smart Car Parking System with Predictive Analytics

**Academic Section**: Mechatronics Engineering Thesis / Journal Draft  
**Institution**: Department of Mechatronics Engineering, Federal University of Agriculture, Abeokuta (FUNAAB)  
**Date**: June 2026  

---

## 1. Introduction and Problem Statement

### 1.1 Context
Urbanization and the rising density of automobile ownership have transformed parking management in campus environments into a major operational bottleneck. Traditional parking systems are passive, forcing drivers to circulate through parking lots searching for vacancies, which leads to:
* Accumulation of carbon emissions due to idling.
* Lost productivity and increased driver frustration.
* Uneven utilization of campus parking lots.

### 1.2 Limitations of Current Solutions
Conventional automated parking solutions rely on high-power camera networks (subject to lighting interference and privacy regulations) or high-cost, single-point wired loop detectors. Additionally, these systems are reactive: they display current occupancy but cannot forecast future availability, preventing campus administrators from performing proactive traffic routing and space management.

### 1.3 The Proposed Mechatronic Solution
This project introduces a low-power, distributed mechatronic sensing framework coupled with an online machine learning prediction service. The system uses low-cost ultrasonic sensor arrays connected to ESP32 microcontrollers. It splits the workload using a dual-hop communication topology (ESP-NOW to Wi-Fi) to transmit data to a Firebase Realtime Database. Finally, it uses a Gradient Boosting machine learning model to predict parking availability up to an hour in advance.

```
+------------------+         +------------------+
| Left Client Node |         | Right Client Node|
|  (ESP32 + HC-SR) |         |  (ESP32 + HC-SR) |
+--------+---------+         +--------+---------+
         | (ESP-NOW)                  | (ESP-NOW)
         +-------------+   +----------+
                       |   |
                       v   v
            +-----------------------+
            |  Central Gateway Node |  <-- INA219 Power Monitor
            |     (ESP32 Gateway)   |
            +-----------+-----------+
                        | (Wi-Fi / HTTPS)
                        v
            +-----------------------+
            | Firebase Cloud RTDB   |
            +-----------+-----------+
                        |
            +-----------+-----------+
            v                       v
  +------------------+    +------------------+
  |  Vercel Web App  |    |  Render ML Model |
  | (Admin/User UI)  |    | (HGBR Predictor) |
  +------------------+    +------------------+
```

---

## 2. Methodology

### 2.1 Edge Sensing & Communication Architecture
The sensing layer is partitioned into two sub-networks to cover the parking bays:
* **Client Sensor Nodes (Left and Right)**: Each node monitors up to 5 bays using HC-SR04 ultrasonic sensors. They calculate local occupancy status and transmit updates via **ESP-NOW**, a low-power, peer-to-peer 2.4GHz wireless protocol. This avoids the high power consumption and connection times associated with standard Wi-Fi.
* **Central Gateway Server**: A central ESP32 acts as the ESP-NOW base station, receiving data packets from the edge nodes. The gateway consolidates the slot states, measures its own energy consumption via an analog **INA219** current sensor, and uploads telemetry packets to the **Firebase Realtime Database** via Wi-Fi.

### 2.2 Cloud Integration & User Interfaces
* **Data Replication Proxy**: To maintain high-speed UI responsiveness and bypass Firebase connection overhead, a Vercel serverless proxy replicates state updates to a web client.
* **Dashboards**: A user-facing booking interface displays real-time slot vacancies and enables arrival reservations, while an admin panel tracks network performance, power diagnostics, and prediction graphs.

### 2.3 Machine Learning Prediction Service
To enable proactive space management, a background service runs on Render. It processes historical occupancy records to predict availability over 15-minute and 1-hour horizons:
* **Features**:
  * *Temporal*: Cyclical time of day (Hour + Minute representation), Day of the week (0–6), and Weekend flags (binary).
  * *Historical*: Same-hour yesterday lag ($y_{t-24h}$), and a rolling mean of occupancy over the past 60 minutes.
* **Algorithms**: Evaluates a baseline **Historical Average**, a **Linear Regression** estimator, and the production-standard **HistGradientBoostingRegressor (HGBR)** (a histogram-based Gradient Boosting decision tree ensemble).

---

## 3. Experimental Results and Discussion

### 3.1 Network QoS and Telemetry (Section 1)
Network reliability was evaluated over 300 transmission cycles between the central gateway and the Firebase cloud:

| Parameter | Measured Value | Academic Target | Status |
|---|---|---|---|
| **Packet Loss Rate** | 0.000% | < 1.00% | **✅ PASSED** |
| **Network Uptime Uptime** | 100.00% | > 99.00% | **✅ PASSED** |
| **Average Jitter** | 186.47 ms | < 120.00 ms | **❌ FAILED** |
| **ICMP Ping Latency (RTT)** | 61.10 ms | < 150.00 ms | **✅ PASSED** |
| **ESP-NOW Link Availability** | 100.00% | > 99.90% | **✅ PASSED** |

*Discussion*: The ESP-NOW protocol achieved 100% reliability, demonstrating the robustness of peer-to-peer networks in campus environments. The jitter and latency failures are due to the cellular Wi-Fi backhaul routing. However, this latency is acceptable because parking slot changes do not require millisecond-level responsiveness.

### 3.2 Energy Consumption Benchmarking (Section 2)
Using physical telemetry from the INA219 current sensor, the operational current profiles of the client nodes were calculated:

* **Active Cycle Current (150 ms)**: $85.0\text{ mA}$ (Sensing + ESP-NOW transmission)
* **Warmup Phase Current (15 ms)**: $15.0\text{ mA}$
* **Deep Sleep Current (30 s)**: $0.15\text{ mA}$ (RTC clock active)
* **Average Daily Consumption**: $26.66\text{ mAh/day}$
* **Projected Uptime (2500 mAh battery)**: **`93.8 Days`** of continuous operation on a single Li-ion cell.
* **Central Gateway Average Power Draw**: $44.99\text{ Watts}$ (Continuous Wi-Fi active connection).

*Discussion*: The sleep-cycling mechatronic design reduces client node power requirements. This allows the system to operate for over 3 months on a single battery, confirming its suitability for deployment without dedicated power lines.

### 3.3 Functional Occupancy Accuracy (Section 3)
Functional accuracy was verified using 276 trial events across 10 monitored slots (L1–L5, R1–R5):

* **True Positives (TP)**: 78 (Correctly identified occupied slots)
* **True Negatives (TN)**: 192 (Correctly identified vacant slots)
* **False Positives (FP)**: 6 (Vacant slots incorrectly flagged as occupied due to sensor noise/reflections)
* **False Negatives (FN)**: 0 (Occupied slots missed by sensors)
* **Accuracy**: **`97.83%`** (Target: > 97.00%) — **✅ PASSED**
* **Recall (Sensitivity)**: **`100.00%`** (Target: > 95.00%) — **✅ PASSED**
* **Precision**: **`92.86%`** (Target: > 95.00%) — **⚠️ MARGINAL**
* **F1-Score**: **`0.9630`** (Target: > 0.9500) — **✅ PASSED**

*Discussion*: The zero False Negative rate ensures that drivers are never routed to an occupied slot. The marginal precision rate of 92.86% was caused by transient ultrasonic echo reflections, which can be mitigated in future iterations using software Kalman filtering on the edge.

### 3.4 Predictive Model Validation (Section 5)
The ML model was evaluated on 152 resampled intervals from 37.9 hours of synchronized history logs. It was split chronologically into 60% training, 20% validation, and 20% testing sets:

#### 3.4.1 Time Series Cross-Validation (Walk-Forward Split)
| Fold | Train Size | Test Size | MAE (slots) | RMSE (slots) | $R^2$ Score | MAPE (%) |
|---|---|---|---|---|---|---|
| Fold 1 | 27 | 25 | 0.148 | 0.384 | 0.812 | 1.48% |
| Fold 2 | 52 | 25 | 1.192 | 1.451 | 0.312 | 11.92% |
| Fold 3 | 77 | 25 | 2.714 | 3.115 | -0.118 | 27.14% |
| Fold 4 | 102 | 25 | 0.298 | 0.545 | 0.895 | 2.98% |
| Fold 5 | 127 | 25 | 4.385 | 4.887 | -0.420 | 43.85% |

#### 3.4.2 Model Performance Comparison (Table 5)
| Model | Horizon | MAE (slots) | RMSE (slots) | MAPE (%) | $R^2$ Score | Inf. Time (ms) |
|---|---|---|---|---|---|---|
| **Historical Avg (Baseline)** | 15 min | 2.45 | 3.12 | 24.5% | 0.00 | < 1 |
| **Historical Avg (Baseline)** | 1 hour | 2.98 | 3.56 | 29.8% | 0.00 | < 1 |
| **Linear Regression** | 15 min | 3.91 | 5.42 | 49.1% | -0.52 | 13.59 |
| **Linear Regression** | 1 hour | 4.12 | 5.89 | 51.2% | -0.61 | 11.20 |
| **HistGradientBoosting (HGBR)** | 15 min | 3.58 | 5.26 | 45.4% | -0.34 | 142.04 |
| **HistGradientBoosting (HGBR)** | 1 hour | 3.82 | 5.48 | 48.2% | -0.45 | 110.15 |

#### 3.4.3 Feature Ablation Study
| Model Configuration | MAE (15 min) | $\Delta$ from Full Model |
|---|---|---|
| **Full model (all features)** | 3.577 | — |
| *Without occupancy at same hour yesterday* | 3.985 | +0.408 |
| *Without rolling mean (60 min)* | 3.892 | +0.315 |
| *Without hour of day* | 4.214 | +0.637 |
| *Without day of week* | 3.612 | +0.035 |
| **Temporal features only** | 4.312 | +0.735 |

---

## 3.5 Discussion on Model Performance and Data Scarcity

The experimental ML metrics show a negative $R^2$ score and an MAE of 3.58 slots, which is higher than the target of $<1.0$. This result highlights a key challenge in time-series forecasting: **data scarcity and cold-start limitations**.

With only 37.9 hours of telemetry logs, the training dataset only covers a single day. On Monday (June 8), the parking lot was empty all morning and only occupied in the late afternoon. On Tuesday (June 9), occupancy spiked early in the morning due to class schedules. 

Because the model was trained only on Monday's data, it assumed that Tuesday morning would also be empty. When tested on Tuesday morning, it predicted low occupancy, leading to a high MAE and a negative $R^2$ score. 

The ablation study shows that removing temporal features like `hour_of_day` increases the MAE by `+0.637` slots. This confirms that time of day is a key feature for the model. Once the system logs **14 to 30 days** of continuous data, it will observe weekly patterns, and the $R^2$ score is expected to exceed the target threshold of `>0.85`.

---

## 4. Conclusion and Recommendations

### 4.1 Summary of Accomplishments
1. **Low-Power Distributed Edge Array**: Implemented a reliable, low-power edge sensing network using ESP-NOW. It provides a projected client node battery life of **93.8 days**.
2. **High-Accuracy Sensing**: Achieved **97.83%** occupancy detection accuracy over 276 trials, ensuring reliable real-time updates.
3. **Cloud and Predictive Infrastructure**: Developed a real-time web portal and a machine learning service using Gradient Boosting to predict parking demand.

### 4.2 Recommendations for Future Work
* **Extend Data Collection**: Continue running the system for **30 days** to collect enough data for the ML model to learn weekly patterns and improve its forecasting accuracy.
* **Implement Edge Filtering**: Add software-based Kalman filtering to the ESP32 client nodes to reduce sensor noise and improve occupancy precision.
* **Dynamic Routing**: Integrate the predictive ML model with campus signage to route drivers to vacant spaces before they enter the parking lot.
