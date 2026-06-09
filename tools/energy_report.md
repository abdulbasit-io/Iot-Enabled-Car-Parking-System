# Energy Consumption Benchmarking & Battery Life Report
**Generated on**: 2026-06-09 17:03:15  
**Academic Section Reference**: Section 2 - Energy Consumption Benchmarking

## 1. Experimental Methodology
To evaluate the energy performance of the smart parking system's components, we model the operational cycle of the wireless ESP32 sensor nodes. The device cycles through five distinct states, using deep sleep modes to preserve power.

## 2. Energy Consumption Profile (per 30s cycle)
| Operational Phase | Duration (ms) | Avg Current (mA) | Power Draw (mW) | Energy (mJ) | % of Total |
|---|---|---|---|---|---|
| Deep Sleep | 30000.0 | 0.015 | 0.06 | 1.67 | 1.3% |
| Sensor Warmup & Trigger | 15.0 | 20.200 | 74.74 | 1.12 | 0.9% |
| Distance Calculation | 50.0 | 44.600 | 165.02 | 8.25 | 6.6% |
| Wi-Fi Initialization | 350.0 | 59.500 | 220.15 | 77.05 | 61.3% |
| Data Transmission (ESP-NOW) | 150.0 | 67.600 | 250.12 | 37.52 | 29.9% |
| **TOTAL CYCLE** | **30565.0** | **—** | **—** | **125.61 mJ** | **100.0%** |

## 3. Battery Lifetime Projection
Calculations are based on a standard **2500.0 mAh Li-ion cell** (3.7V nominal):

* **Charge per Transmission Cycle**: `0.009430 mAh`
* **Cycles per 24 Hours**: `2827`
* **Daily Power Consumption**: `26.656 mAh/day`
* **Estimated Sensor Battery Life**: **`93.8 Days` (0.26 Years)**

## 4. Central Gateway Power Benchmark
* **Average Operational Power (Active Link)**: `44.79 W` (derived from integrated ADC current telemetry logs).

---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
